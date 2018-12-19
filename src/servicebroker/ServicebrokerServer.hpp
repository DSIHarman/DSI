/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_SERVICEBROKERSERVER_HPP
#define DSI_SERVICEBROKER_SERVICEBROKERSERVER_HPP

#include <tr1/type_traits>

#include "dsi/private/CNonCopyable.hpp"

#include "Slist.hpp"
#include "bind.hpp"
#include "io.hpp"

#include "SocketConnectionContext.hpp"
#include "ClientSpecificData.hpp"
#include "AutoBuffer.hpp"
#include "WorkerThread.hpp"


class MasterAdapter;
class ServicebrokerServer;


#ifdef HAVE_HTTP_SERVICE

/**
 * 'Connection: close' HTTP server handle.
 */
class HTTPClient : public DSI::Private::CNonCopyable
{
public:

   HTTPClient(const DSI::IPv4::StreamSocket& sock);

   // start statemachine: read data -> eval -> write data
   void start();

private:

   void eval();
   void arm();
   const char* parseRequestLine();
   int parseContentLength();

   bool handleRead(size_t amount, DSI::io::error_code err);
   bool handleWrite(size_t amount, DSI::io::error_code err);

   DSI::IPv4::StreamSocket mSock;

   // read buffer
   char buf_[512];
   char* body_begin_;
   int total_;

   enum State { State_SENDHEADER = 0, State_SENDBODY };

   State mState;

   std::string mHeader;
   std::string mBody;
};

#endif   // HAVE_HTTP_SERVICE


/**
 * A client handle base class - either from a slave servicebroker or a real DSI application.
 */
class ServicebrokerClientBase : public DSI::Private::CNonCopyable, public DSI::intrusive::SlistBaseHook
{
   friend class SocketMessageContext;
public:

   enum State
   {
      State_READING_HEADER = 0,
      State_READING_BODY,
      State_WRITING,
      State_DEFERRED
   };

   ServicebrokerClientBase(ServicebrokerServer& server, uint32_t nodeAddress, uint32_t localAddress);
   virtual ~ServicebrokerClientBase();

   virtual void start() = 0;

   /**
    * Helper function to be efficiently used with the intrusive::Slist.
    */
   static inline
   void dispose(ServicebrokerClientBase* data)
   {
      delete data;
   }

   inline
   const SocketConnectionContext& getContext() const
   {
      return mContext;
   }

private:

   inline
   ServicebrokerClientBase& self()
   {
      return *this;
   }

protected:

   /**
    * Handle new request frame -> dispatch request in Servicebroker singleton.
    */
   void eval(struct sb_request_frame& f);

   virtual void arm() = 0;
   virtual void evaluateWriteResult() = 0;

   /**
    * Send a deferred response back to the client.
    *
    * @param status OS return value from the IO operation to master.
    * @param retval Response code from SB master.
    * @param data Data of the response (body only, no header data).
    * @param len Length of data.
    */
   void sendDeferredResponse(int status, int retval, void* data, size_t len);

   bool handleRead(size_t amount, DSI::io::error_code err);
   bool handleWrite(size_t amount, DSI::io::error_code err);

   ClientSpecificData mData;
   SocketConnectionContext mContext;

   ServicebrokerServer& mServer;

   // input and output buffers - fixed size, more will be allocated automatically if needed
   tAutoBufferType mReadBuffer;
   tAutoBufferType mWriteBuffer;
   State mState;
};


template<typename IOCompletionRoutineT, typename DeviceT>
struct CredentialsEvent : public DSI::EventBase
{
   typedef DSI::ReadEventTraits<IOCompletionRoutineT> traits_type;
   typedef typename DeviceT::fd_type fd_type;

   CredentialsEvent(IOCompletionRoutineT func)
      : func_(func)
   {
      // NOOP
   }

   // FIXME check for events POLLHUP before POLLIN since both are marked
   bool eval(fd_type fd, short revents)
   {
      static struct ucred _credentials = { -1, SB_UNKNOWN_USER_ID, SB_UNKNOWN_GROUP_ID };
      struct ucred* credentials = &_credentials;

      if (revents & POLLIN)
      {
         struct auth
         {
            char magic[4];    // buffer to receive 'A' 'U' 'T' 'H'
            int32_t pid;        // only good for TCP authentication requests
         } auth_request;

         iovec io[1];
         io[0].iov_base = &auth_request;
         io[0].iov_len = sizeof(auth_request);

         struct msghdr hdr;
         ::memset(&hdr, 0, sizeof(hdr));
         hdr.msg_iov = io;
         hdr.msg_iovlen = 1;

         // receive Unix credentials as ancillary data
         if (std::tr1::is_same<DeviceT, DSI::Unix::StreamSocket>::value)
         {
            char buf[CMSG_SPACE(sizeof(struct ucred))];

            hdr.msg_control = buf;
            hdr.msg_controllen = sizeof(buf);
         }

         ssize_t len = 0;
         do
         {
            len = ::recvmsg(fd, &hdr, MSG_NOSIGNAL);
         }
         while (len < 0 && errno == EINTR);

         if (std::tr1::is_same<DeviceT, DSI::Unix::StreamSocket>::value)
         {
            struct cmsghdr* cmsg = CMSG_FIRSTHDR(&hdr);
            credentials = (struct ucred *)CMSG_DATA(cmsg);
         }
         else
         {
            _credentials.pid = auth_request.pid;
         }

         // FIXME better return enums (errnos) here instead of bool
         func_(*credentials,
               len == sizeof(auth_request)
               && auth_request.magic[0] == 'A'
               && auth_request.magic[3] == 'H' ? DSI::io::ok : DSI::io::unspecified);
      }
      else
      {
         _credentials.pid = -1;
         func_(*credentials, DSI::io::unspecified);
      }

      return false;  // no auto rearm
   }

   IOCompletionRoutineT func_;
};


#if defined(HAVE_TCP_CLIENT_TIMEOUT_SUPERVISION)

typedef DSI::Dispatcher tDispatcherType;
typedef DSI::NativeTimer tTimerType;

#else
typedef Dispatcher tDispatcherType;
#endif   // HAVE_TCP_CLIENT_TIMEOUT_SUPERVISION


class TCPMixin : public DSI::Private::CNonCopyable, public DSI::intrusive::SlistBaseHook
{
public:
   inline explicit TCPMixin(tDispatcherType& service)
      : mTcpAcceptor(service)
      , mNextTcp(service)
   {
      // NOOP
   }

   /**
    * Helper function to be efficiently used with the intrusive::Slist.
    */
   static inline
   void dispose(TCPMixin* data)
   {
      delete data;
   }

   DSI::IPv4::Acceptor mTcpAcceptor;
   DSI::IPv4::StreamSocket mNextTcp;
};


#ifdef HAVE_HTTP_SERVICE

struct HTTPMixin
{
   inline explicit
   HTTPMixin(tDispatcherType& service)
      : mHTTPAcceptor(service)
      , mNextHTTP(service)
   {
      // NOOP
   }

   bool init();
   bool handleNewClient(DSI::IPv4::Endpoint& address, DSI::io::error_code err);

   /// close HTTP sockets
   void close();

   DSI::IPv4::Acceptor mHTTPAcceptor;
   DSI::IPv4::StreamSocket mNextHTTP;
};

#else

struct HTTPMixin
{
   inline explicit
   HTTPMixin(tDispatcherType& /*service*/)
   {
      // NOOP
   }

   inline
   bool init()
   {
      return true;
   }

   inline
   void close()
   {
      // NOOP
   }
};

#endif   // HAVE_HTTP_SERVICE


struct NotificationMixin
{
   inline explicit
   NotificationMixin(tDispatcherType& service)
      : mNotificationAcceptor(service)
      , mNextNotification(service)
   {
      // NOOP
   }

   bool init();
   bool handleNewClient(DSI::IPv4::Endpoint& address, DSI::io::error_code err);

   DSI::IPv4::Acceptor mNotificationAcceptor;
   DSI::IPv4::StreamSocket mNextNotification;
};


/**
 * The connection acceptor.
 */
class ServicebrokerServer
   : public DSI::Private::CNonCopyable
   , private HTTPMixin
   , private NotificationMixin
{
public:
   enum
   {
      eEnableTcpMaster   = (1<<0),
      eEnableTcpSlave    = (1<<1),
      eEnableHTTP        = (1<<2),

      eEnableAll         = 0xFF
   };

   ServicebrokerServer();
   ~ServicebrokerServer();

   /// Add this new client to list of all connected TCP clients
   void addClient(ServicebrokerClientBase& client);

   /// Removes this new client from list of all connected TCP clients (if any)
   void removeClient(ServicebrokerClientBase& client);

   /// Remove TCP client with given extendedID (if any)
   void removeClient(int32_t extendedID);

   bool init(const char* mountpoint, MasterAdapter* master, unsigned int flags, const char* const bindIPs);

   int run();
   void stop();

   /**
    * Stop the internal workerthread - if any.
    */
   void stopWorkerThread();

   inline
   tDispatcherType& dispatcher()
   {
      return mService;
   }


private:

   bool handleNewTcpClient(DSI::IPv4::Endpoint& address, DSI::io::error_code err);
   TCPMixin* getServerSocket(DSI::IPv4::Endpoint& address);

   bool handleNewLocalClient(DSI::Unix::Endpoint& address, DSI::io::error_code err);
   bool handleInternalNotification(size_t amount, DSI::io::error_code err);

   MasterAdapter* mMaster;

   // sockets stuff
   tDispatcherType mService;

   DSI::Unix::Acceptor mUnixAcceptor;
   DSI::Unix::StreamSocket mNextUnix;

   // slave-to-master communication goes here
   WorkerThread mWorker;

   DSI::DescriptorDevice mPipeDevice;
   InternalNotification mCurrentNotification;

   /// List of all connected TCP clients (other servicebrokers)
   DSI::intrusive::Slist<ServicebrokerClientBase> mClients;
   DSI::intrusive::Slist<TCPMixin> mServerSockets;

   unsigned int mFlags;
   int mStopped;
   const char * mBindIPs;
   bool mBindFeatureActive;
};


inline
void ServicebrokerServer::stopWorkerThread()
{
   if (mWorker)
      mWorker.stop();
}


// ----------------------------------------------------------------------------------


template<typename SocketT>
struct TimerMixin
{
   inline
   TimerMixin(tDispatcherType&)
   {
      // NOOP
   }

   virtual ~TimerMixin()
   {
      // NOOP
   }

   inline
   void tick()
   {
      // NOOP
   }
};


// ----------------------------------------------------------------------------------


#ifdef HAVE_TCP_CLIENT_TIMEOUT_SUPERVISION

// only senseful for ipv4 connections (this is a slave servicebroker)
template<>
struct TimerMixin<DSI::IPv4::StreamSocket>
{
   TimerMixin(tDispatcherType& dispatcher)
      : t_(dispatcher)
      , operations_(0)
   {
      t_.start(4000, bind2(&TimerMixin<DSI::IPv4::StreamSocket>::handleTimeout, std::tr1::ref(*this), std::tr1::placeholders::_1));
   }


   virtual ~TimerMixin()
   {
      // NOOP
   }


   bool handleTimeout(DSI::io::error_code err)
   {
      Log::message(5, "Having timer timeout");
      if (err == DSI::io::ok)
      {
         if (operations_ > 0)
         {
            operations_ = 0;
            return true;    // rearm...
         }
         else
            delete this;    // timeout -> close connection
      }

      return false;
   }

   inline
   void tick()
   {
      ++operations_;
   }

   tTimerType t_;
   unsigned int operations_;
};

#endif   // HAVE_TCP_CLIENT_TIMEOUT_SUPERVISION


template<typename SocketT>
class ServicebrokerClient : private ServicebrokerClientBase, private TimerMixin<SocketT>
{
public:

   /**
    * @param nodeAddress node address in host byte order.
    */
   ServicebrokerClient(SocketT& sock, ServicebrokerServer& server,
                       uint32_t nodeAddress = SB_LOCAL_NODE_ADDRESS, uint32_t localAddress = SB_LOCAL_NODE_ADDRESS)
      : ServicebrokerClientBase(server, nodeAddress, localAddress)
      , TimerMixin<SocketT>(server.dispatcher())
      , mSock(sock)
   {
      // NOOP
   }

   void start()
   {
      (void)mSock.fileControl(DSI::NonBlockingFileControl());
      armCredentials(bind3(&ServicebrokerClient<SocketT>::handleCredentials, std::tr1::ref(*this), std::tr1::placeholders::_1, std::tr1::placeholders::_2));
   }

private:

   bool handleRead(size_t amount, DSI::io::error_code err)
   {
      TimerMixin<SocketT>::tick();
      return ServicebrokerClientBase::handleRead(amount, err);
   }

   bool handleWrite(size_t amount, DSI::io::error_code err)
   {
      return ServicebrokerClientBase::handleWrite(amount, err);
   }

   bool handleCredentials(const struct ucred& credentials, DSI::io::error_code err)
   {
      if (std::tr1::is_same<typename SocketT::endpoint_type, DSI::Unix::Endpoint>::value)
      {
         int on = 0;
         (void)setsockopt(mSock.fd(), SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));  // switch off again
      }

      if (err == DSI::io::ok)
      {
         mContext.init(credentials.pid, credentials.uid);

         arm();    // ... for normal operation
      }
      else
         delete this;

      return false;
   }

   template<typename IOCompletionRoutineT>
   void armCredentials(IOCompletionRoutineT func)
   {
      if (std::tr1::is_same<typename SocketT::endpoint_type, DSI::Unix::Endpoint>::value)
      {
         int on = 1;
         (void)setsockopt(mSock.fd(), SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));   // switch it on
      }

      mSock.dispatcher()->enqueueEvent(mSock.fd(), new CredentialsEvent<IOCompletionRoutineT, SocketT>(func));
   }


   void evaluateWriteResult()
   {
      // try to write buffer directly
      DSI::io::error_code ec = DSI::io::unspecified;
      ssize_t len = mWriteBuffer.response().fr_envelope.fr_size + sizeof(sb_response_frame_t);


      ssize_t rc = mSock.write_some(mWriteBuffer.ptr(), len, ec);

      if (ec == DSI::io::ok)
      {
         if (rc < len)
         {
            mWriteBuffer.bump(rc);
            mState = State_WRITING;
         }
         else
         {
            mWriteBuffer.reset();
            mState = State_READING_HEADER;
         }

         arm();
      }
      else if(ec == DSI::io::would_block)
      {
         mState = State_WRITING;
         arm();
      }
      else
         delete this;
   }


   void arm()
   {
      if (mState == State_READING_HEADER || mState == State_READING_BODY)
      {
         if (mReadBuffer.capacity() - mReadBuffer.pos() > 0)
         {
            mSock.async_read_some(mReadBuffer.ptr(), mReadBuffer.capacity() - mReadBuffer.pos(), bind3(&ServicebrokerClient<SocketT>::handleRead, std::tr1::ref(*this), std::tr1::placeholders::_1, std::tr1::placeholders::_2));
         }
         else
         {
            Log::error("Strange socket state detected. Closing connection.");
            delete this;
         }
      }
      else
      {
         mSock.async_write_some(mWriteBuffer.ptr(), mWriteBuffer.response().fr_envelope.fr_size + sizeof(sb_response_frame_t) - mWriteBuffer.pos(), bind3(&ServicebrokerClient<SocketT>::handleWrite, std::tr1::ref(*this), std::tr1::placeholders::_1, std::tr1::placeholders::_2));
      }
   }


private:

   SocketT mSock;
};

typedef ServicebrokerClient<DSI::Unix::StreamSocket> ServicebrokerClientUnix;

typedef ServicebrokerClient<DSI::IPv4::StreamSocket> ServicebrokerClientTcp;

#endif   // DSI_SERVICEBROKER_SERVICEBROKERSERVER_HPP
