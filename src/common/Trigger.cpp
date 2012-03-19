/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "Trigger.hpp"


#include <time.h>
#include <errno.h>


/// duration in milliseconds
bool DSI::Trigger::timed_wait(unsigned int duration)
{
   struct timespec ts;
   (void)::clock_gettime(CLOCK_REALTIME, &ts);

   ts.tv_sec += duration / 1000;
   ts.tv_nsec += 1000000ll * (duration % 1000);

   if (ts.tv_nsec / 1000000000 > 0)
   {
      ts.tv_sec += ts.tv_nsec / 1000000000ll;
      ts.tv_nsec -= (ts.tv_nsec / 1000000000ll) * 1000000000ll;
   }

   int err = ::sem_timedwait(&mHandle, &ts);

   assert(err == 0 || (err < 0 && (errno == ETIMEDOUT || errno == EINTR)));

   return err == 0;
}


bool DSI::Trigger::signal()
{
   int v = 0;
   int err = ::sem_getvalue(&mHandle, &v);

   if (err == 0 && v < 1)
      err = ::sem_post(&mHandle);

   return err == 0;
}

