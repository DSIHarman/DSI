/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_TIMER_HPP
#define DSI_TIMER_HPP


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
#include "CDevices.hpp"



namespace DSI
{
   /**
    * A native multiplexer dispatchable timer.
    */
   struct NativeTimer : public Device<NormalDescriptorTraits>, public DSI::Private::CNonCopyable
   {
   public:

      explicit
      NativeTimer(Dispatcher& dispatcher);

      ~NativeTimer();

      template<typename TimerTimeoutRoutineT>
      io::error_code start(unsigned int timeout_ms, TimerTimeoutRoutineT func)
      {
         io::error_code rc = io::unspecified;
         NativeTimerEvent<TimerTimeoutRoutineT>* e = new NativeTimerEvent<TimerTimeoutRoutineT>(timeout_ms, func);

         if (e->arm(fd_))
         {
            this->dispatcher_->enqueueEvent(fd_, e);
            rc = io::ok;
         }
         else
            delete e;

         return rc;
      }
   };
}//namespace DSI

#endif   // DSI_TIMER_HPP
