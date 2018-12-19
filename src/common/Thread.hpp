/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_COMMON_THREAD_HPP
#define DSI_COMMON_THREAD_HPP


#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


namespace DSI
{

   class Thread
   {
      struct ArgumentHolder
      {
         void* runnable_;
         volatile int finished_;
      };

      template<typename RunnableT>
      static
      void* run_it(void* arg)
      {
         ArgumentHolder* arg_ = (ArgumentHolder*)arg;
         RunnableT r = *(RunnableT*)arg_->runnable_;
         arg_->finished_ = 1;
         r();
         return nullptr;
      }


   public:

      /// create a dead thread
      inline
      Thread()
         : mTid(0)
      {
         // NOOP
      }


      /// create a new thread from runnable
      template<typename RunnableT>
      explicit inline
      Thread(RunnableT runnable)
      {
         ArgumentHolder holder = { &runnable, 0 };
         int rc = ::pthread_create(&mTid, nullptr, run_it<RunnableT>, &holder);
         assert(rc == 0);
         (void)rc;
         while(!holder.finished_);    // FIXME remove busy looping here?
      }


      /// join the thread
      inline
      ~Thread()
      {
         join();
      }


      inline
      operator const void*() const
      {
         return mTid ? this : nullptr;
      }


      /// assignment operator with move semantics
      /// so don't try to write myThread = Thread(runner) since this will not compile (Thread is an rvalue here)
      inline
      Thread& operator=(Thread& rhs)
      {
         if (this != &rhs)
         {
            mTid = rhs.mTid;
            rhs.mTid = 0;    // make a move
         }

         return *this;
      }


      /// join the thread - if possible
      inline
      bool join()
      {
         bool rc = true;

         if (mTid != 0)
         {
            rc = (::pthread_join(mTid, nullptr) == 0);
            mTid = 0;
         }

         return rc;
      }


      /// join the thread - if possible
      // FIXME implement this
      inline
      bool timed_join(unsigned int /*timeoutMs*/)
      {
         return join();
      }


   private:

      pthread_t mTid;
   };
}//namespace DSI

#endif   // DSI_COMMON_THREAD_HPP
