/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_COMMON_TRIGGER_HPP
#define DSI_COMMON_TRIGGER_HPP


#include <semaphore.h>
#include <cassert>

#include "dsi/private/CNonCopyable.hpp"


namespace DSI
{
   class Trigger : public DSI::Private::CNonCopyable
   {
   public:

      inline
      Trigger()
      {
         int rc = ::sem_init(&mHandle, 0, 0);
         (void)rc;
         assert(rc != -1);
      }


      inline
      ~Trigger()
      {
         int rc = ::sem_destroy(&mHandle);
         (void)rc;
         assert(rc != -1);
      }


      /// duration in milliseconds
      bool timed_wait(unsigned int duration);

      /// signal (wake up) waiter
      bool signal();


   private:

      sem_t mHandle;
   };
}//namespace DSI

#endif   // DSI_COMMON_TRIGGER_HPP
