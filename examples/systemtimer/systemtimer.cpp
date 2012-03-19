/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/CCommEngine.hpp"

#include <sys/timerfd.h>
#include <errno.h>

#include <iostream>
#include <cassert>
#include <cstdlib>

using namespace DSI;

/**
 * An alarm clock that ticks @c seconds seconds before saying "Beep!".
 */
class CAlarmClock
{
public:

   CAlarmClock(CCommEngine& commEngine, int seconds = 10)
    : mFd(-1)
    , mCommEngine(commEngine)
    , mSeconds(seconds)
    , mElapsed(0)
   {
      mFd = timerfd_create(CLOCK_MONOTONIC, 0);
      assert(mFd > 0);

      // create interval timer for one-seconds intervals
      struct itimerspec spec = {
         { 1, 0 },
         { 1, 0 }
      };

      int rc = timerfd_settime(mFd, 0, &spec, 0);
      assert(rc == 0);
      (void)rc;

      // make the timers file descriptor be known by the DSI event loop
      mCommEngine.addGenericDevice(mFd, CCommEngine::In, std::tr1::bind(&CAlarmClock::timerPing, this, std::tr1::placeholders::_1));
   }


   inline
   ~CAlarmClock()
   {
      mCommEngine.removeGenericDevice(mFd);

      while(close(mFd) && errno == EINTR);
   }


private:

   bool timerPing(CCommEngine::IOResult /*unused here*/)
   {
      // we ignore expirations so far...

      uint64_t expirations;
      ssize_t rc = ::read(mFd, &expirations, sizeof(expirations));

      assert(rc == sizeof(expirations));

      std::cout << (++mElapsed % 2 == 1 ? "Tick" : "Tack") << std::endl;

      if (mElapsed == mSeconds)
      {
         std::cout << "Beep!!!" << std::endl;
         mCommEngine.stop();
      }

      return true;
   }


   int mFd;
   CCommEngine& mCommEngine;

   int mSeconds;
   int mElapsed;
};


/**
 * This example shows how to integrate any other multiplexable devices to the main DSI event loop.
 * Here we just take a simple timer fd, but any other objects that can be polled on are allowed,
 * e.g. sockets, pipes, message queues, signal fds, ...
 */
int main(int argc, char** argv)
{
   int seconds = 10;

   if (argc > 1)
      seconds = atoi(argv[1]);

   // the DSI communication engine to work on
   CCommEngine commEngine;

   CAlarmClock clock(commEngine, seconds);

   // you should also add your servers and clients to the communication engine before calling run.
   return commEngine.run();
}
