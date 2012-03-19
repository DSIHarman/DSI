/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "PulseChannelManager.hpp"

#include "io.hpp"
#include "clientIo.h"
#include "config.h"
#include "Log.hpp"
#include "Notification.hpp"

using namespace DSI;

namespace /*anonymous*/
{

inline
void sendSocketNotification(int notification_id, int fd, int code, int value)
{
   sb_pulse_t tmp = { code, value };
   size_t total = 0;
   ssize_t rc;
   do
   {
      rc = ::send(fd, ((char*)&tmp) + total, sizeof(tmp) - total, MSG_NOSIGNAL);

      if (rc > 0)
         total += rc;
   }
   while((rc < 0 && errno == EINTR) || (rc > 0 && total < sizeof(tmp)));

   if (rc < 0)
   {
      int error = errno;
      Log::warning(0, " Error sending notification -%d- (TCP send errno: %d)", notification_id, error);
   }
}


struct CompFd
{
   CompFd(int fd)
    : fd_(fd)
   {
      // NOOP
   }

   bool operator()(PulseChannel& channel) const
   {
      return channel.fd == fd_;
   }

private:
   int fd_;
};


struct CompNidPidChid
{
   CompNidPidChid(int nid, int pid, int chid)
    : nid_(nid)
    , pid_(pid)
    , chid_(chid)
   {
      // NOOP
   }

   bool operator()(PulseChannel& channel) const
   {
      return channel.nid == nid_ && channel.pid == pid_ && channel.chid == chid_;
   }

private:
   int nid_;
   int pid_;
   int chid_;
};


/**
 * @return a pointer to a given pulse channel from the given @c container
 *         or 0 if nothing was found.
 */
template<typename ComparatorT>
int find(PulseChannelManager::tPulseChannelListType& container, const ComparatorT& comp)
{
   // first search in list...
   for (size_t i=0; i<container.size(); ++i)
   {
      PulseChannel& chnl = container[i];

      if (comp(chnl))
         return i;
   }

   return -1;
}


/**
 * Increments the refcount of the PulseChannel if available.
 *
 * @return -1 if given nid/pid/chid triple could not be found, else the associated file descriptor.
 */
int tryAddRef(PulseChannelManager::tPulseChannelListType& container, int nid, pid_t pid, int chid)
{
   int idx = find(container, CompNidPidChid(nid, pid, chid));

   if (idx >= 0)
   {
      ++container[idx].refCount;
      return container[idx].fd;
   }

   return -1;
}


struct SocketDestructor
{
   static inline
   void eval(int handle)
   {
      while(::close(handle) && errno == EINTR);
   }
};


/**
 * Decrements the refcount of the PulseChannel if available.
 */
template<typename DestructorT>
void release(PulseChannelManager::tPulseChannelListType& container, int fd)
{
   int idx = find(container, CompFd(fd));
   if (idx >= 0)
   {
      if (--(container[idx].refCount) == 0)
      {
         DestructorT::eval(fd);
         container.erase(container.begin()+idx);
      }
   }
}


inline
unsigned long long getMonotonicTime()
{
   struct timespec ts;
   assert(::clock_gettime(CLOCK_MONOTONIC, &ts) == 0);

   return  1000000000LL * ts.tv_sec + ts.tv_nsec;
}


inline
unsigned int timeDiffMonotonicMs(unsigned long long diff_to)
{
   long long rc = getMonotonicTime() - diff_to;
   assert(rc > 0);

   return rc / 1000000;
}


/**
 * Wait for asynchronous connection establishment (allow timeout).
 */
io::error_code waitForConnectionEstablished(IPv4::StreamSocket& sock, unsigned int timeoutMs)
{
   io::error_code rc = io::unspecified;

   Log::message(4, "Waiting for establishing a notification connection...");

   pollfd fds[1] = { { sock.fd(), POLLOUT, 0 } };
   bool doItAgain;

   do
   {
      doItAgain = false;

      unsigned long long lastCall = getMonotonicTime();

      int ret = ::poll(fds, 1, timeoutMs);
      if (ret == 0)
      {
         // normal timeout, exit
         rc = io::would_block;
      }
      else if (ret > 0)
      {
         // socket connected?
         int result = 0;
         socklen_t len = sizeof(result);

         if (::getsockopt(sock.fd(), SOL_SOCKET, SO_ERROR, &result, &len) == 0)
         {
            if (result == 0)
            {
               rc = io::ok;
            }
            else
               rc = io::to_error_code(result);
         }
      }
      else
      {
         if (errno == EINTR)
         {
            unsigned int diff = timeDiffMonotonicMs(lastCall);
            if (diff < timeoutMs)
            {
               timeoutMs -= diff;
               doItAgain = true;
            }
            else
            {
               rc = io::would_block;
            }
         }
         else
         {
            rc = io::to_error_code(io::getLastError());
         }
      }
   }
   while(doItAgain);

   return rc;
}

}   // namespace anonymous


// -------------------------------------------------------------------------------------------------


PulseChannelManager::PulseChannelManager()
{
   // NOOP
}


/*static*/
PulseChannelManager& PulseChannelManager::getInstance()
{
   static PulseChannelManager manager;
   return manager;
}


int PulseChannelManager::attach(int nid, int pid, int chid)
{
   int rc = -1;

   // first search in list...
   int fd = tryAddRef(mSockets, nid, pid, chid);
   if (fd != -1)
      return fd;

   // not available - create entry
   if (nid == SB_LOCAL_NODE_ADDRESS)
   {
      // is unix socket
      // create path via pid and chid

      Unix::StreamSocket sock;

      if (sock.open() == io::ok)
      {
         char buf[128];
         Unix::Endpoint dest(make_unix_path(buf, sizeof(buf), pid, chid));
         io::error_code ec = io::unset;

         do
         {
            ec = sock.connect(dest);
         }
         while(ec == io::interrupted);

         if (ec == io::ok)
         {
            (void)sock.fileControl(NonBlockingFileControl());
            rc = sock.release();
         }
      }
   }
   else
   {
      // is tcp socket (other servicebroker)
      short port = chid;

      IPv4::StreamSocket sock;

      if (sock.open() == io::ok)
      {
         (void)sock.fileControl(NonBlockingFileControl());

         // extract the pid from the masters peer socket if not given by the slave servicebroker
         IPv4::Endpoint dest((pid != SB_UNKNOWN_IP_ADDRESS ? ntohl(pid) : nid), ntohs(port));
         io::error_code ec = io::unset;

         do
         {
            ec = sock.connect(dest);
         }
         while(ec == io::interrupted);

         if (ec != io::ok && ec != io::in_progress && pid != SB_UNKNOWN_IP_ADDRESS)
         {
            // at least QNX does want this from us here...
            (void)sock.close();
            if (sock.open() == io::ok)
            {
               (void)sock.fileControl(NonBlockingFileControl());
               dest.set(nid, ntohs(port));

               do
               {
                  ec = sock.connect(dest);
               }
               while(ec == io::interrupted);
            }
         }

         if (ec == io::in_progress)
         {
            ec = waitForConnectionEstablished(sock, SB_MASTERADAPTER_SENDTIMEO);
         }

         if (ec == io::ok)
         {
            (void)sock.setSocketOption(SocketTcpNoDelayOption<true>());
            rc = sock.release();
         }
         else
         {
            char tmp[32];
            Log::warning(0, "Cannot connect to %s: errorcode=%d", dest.toString(tmp), static_cast<int>(ec));
         }
      }
   }

   if (rc >= 0)
   {
      PulseChannel chl(rc, nid, pid, chid);
      mSockets.push_back(chl);
   }

   return rc;
}


void PulseChannelManager::detach(int handle)
{
   release<SocketDestructor>(mSockets, handle);
}


void PulseChannelManager::sendPulse(int handle, notificationid_t notificationID, int code, int value)
{
   (void)sendSocketNotification(notificationID, handle, code, value);
}
