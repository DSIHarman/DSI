/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_DISPATCHER_HPP
#define DSI_DISPATCHER_HPP


#include "dsi/private/CNonCopyable.hpp"

/*
  Policies: do not loop on EINTR on blocking _some functions, but loop on asynchronous function calls since there data
  should be ready to read so we do not block infinitely if we return to the action due to an EINTR.

  Desired behaviour of the Dispatcher (not yet fully implemented):

  It should be possible to arm for read and write simultaneously.
  When arming a device multiple times with the same policy the last wins, all previous armed io events will be deleted.
  When arming a device during a callback on the same fd that is currently under action a callback return value of 'true'
  should remove the user set callback and restore the original one.
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/timerfd.h>

// unix sockets
#include <sys/un.h>

// ip sockets
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "CHandler.hpp"

namespace DSI
{
   template<typename InheriterT>
   class DispatcherBase :  public DSI::Private::CNonCopyable
   {
   public:

      DispatcherBase()
         : finished_(false)
         , rc_(0)
      {
         // NOOP
      }

      template<typename DeviceT>
      inline
      void removeAll(DeviceT& device)
      {
         removeAll(device.fd());
      }

      /// This interface method is ment to be an extension point.
      inline
      void removeAll(int fd)
      {
         static_cast<InheriterT*>(this)->doRemoveAll(fd);
      }

      // FIXME allow read and write simultaneously on same fdset entry, but no multiple reads/writes
      // FIXME therefore a fd should always exist only once in the fdset
      // FIXME use internal pool for events, which could perhaps be rebalanced if a large event is added.
      template<typename EventT>
      inline
      void enqueueEvent(typename EventT::fd_type fd, EventT* evt)
      {
         enqueueEvent(fd, evt, EventT::traits_type::poll_mask);
      }

      // FIXME make sure a certain fd can only be attached once, maybe we should just replace any existing registration by the new one?
      /// This interface method is ment to be an extension point.
      inline
      void enqueueEvent(int fd, EventBase* e, short direction)
      {
         static_cast<InheriterT*>(this)->doEnqueueEvent(fd, e, direction);
      }


      /**
       * Run the main event loop.
       *
       * @return -1 on error during poll (errno maybe evaluated) or any other value given as argument to stop().
       */
      int run();


      void stop(int rc = 0)
      {
         // FIXME maybe send signal to myself to get out of 'poll' here?
         finished_ = true;
         rc_ = rc;
      }


   protected:

      ~DispatcherBase()
      {
         // NOOP
      }

      volatile bool finished_;
      int rc_;
   };

   /**
    * The device event dispatcher (poll)
    */
   class Dispatcher : public DispatcherBase<Dispatcher>
   {

      friend class DispatcherBase<Dispatcher>;

   public:
      Dispatcher(size_t capa = 128);


      ~Dispatcher();


      int poll(int timeout_ms);


      // remove event in concern of read/write status
      template<typename EventTraitsT>
      void removeEvent(int fd)
      {
         for (unsigned int i=0; i<max_; ++i)
         {
            if (fds_table_[i].fd == fd && (fds_table_[i].events & EventTraitsT::poll_mask))
            {
               clearSlot(i);
               break;
            }
         }
      }

   private:

      void doEnqueueEvent(int fd, EventBase* evt, short pollmask);


      // remove all events to the given file descriptor
      void doRemoveAll(int fd);


      void clearSlot(int idx);


      size_t max_;
      size_t capa_;

      pollfd* fds_table_;
      EventBase** events_table_;
   };
}//namespace DSI

#include "CDispatcherT.hpp"

#endif   // DSI_DISPATCHER_HPP
