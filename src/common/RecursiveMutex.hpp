/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_COMMON_RECURSIVE_MUTEX
#define DSI_COMMON_RECURSIVE_MUTEX


#include <pthread.h>

#include "dsi/private/CNonCopyable.hpp"


namespace DSI
{

   /// FIXME is this really recursive?
   class RecursiveMutex : public Private::CNonCopyable
   {
   public:

      inline
      RecursiveMutex()
      {
         (void)::pthread_mutex_init(&mMutex, nullptr);
      }


      inline
      ~RecursiveMutex()
      {
         (void)::pthread_mutex_destroy(&mMutex);
      }


      inline
      void lock()
      {
         (void)::pthread_mutex_lock(&mMutex);
      }


      inline
      void unlock()
      {
         (void)::pthread_mutex_unlock(&mMutex);
      }


   private:

      pthread_mutex_t mMutex;
   };
}//namespace DSI

#endif   // DSI_COMMON_RECURSIVE_MUTEX
