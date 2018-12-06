/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "WorkerThread.hpp"

#include <pthread.h>
#include <signal.h>
#include <sys/poll.h>
#include <cassert>

#include <tr1/functional>

#include "Log.hpp"
#include "Notifier.hpp"
#include "MasterAdapter.hpp"

#define sleepMs(timeout) (void)::poll(nullptr, 0, timeout)

using namespace DSI;


WorkerThread::WorkerThread()
 : mActive(false)
 , mThread()
 , mAster(nullptr)
 , mNotifier(nullptr)
{
   // NOOP
}


WorkerThread::~WorkerThread()
{
   stop();
   (void)mThread.timed_join(1000);
}


void WorkerThread::start(int fd, MasterAdapter& master)
{
   assert(fd);

   mAster = &master;
   mAster->setTrigger(*this);

   mNotifier.reset(new Notifier(fd));

   Thread temp(std::tr1::bind(&WorkerThread::run, this));
   mThread = temp;
}


void WorkerThread::stop()
{
   mActive = false;
   trigger();
}


void WorkerThread::run()
{
   {
      sigset_t set;
      (void)::sigfillset(&set);
      if (::pthread_sigmask(SIG_SETMASK, &set, nullptr) != 0)
         Log::error("Cannot set workerthread's signal mask");
   }

   bool timeout = true ;
   Log::message( 3, "workerthread is running" );

   mActive = true ;

   while( mActive )
   {
      if( !mAster->isConnected() )
      {
         mAster->removePending(*mNotifier);

         if( mAster->connect() )
         {
            if (!mAster->sendIdPing())
            {
               mAster->disconnect();
            }
            else
               mNotifier->masterConnected();
         }
         else
         {
             sleepMs(SB_MASTERADAPTER_RECONNECT_TIMEOUT);
         }
      }
      else
      {
         if (mAster->executePending(*mNotifier))
         {
            if (timeout)
            {
               if (!mAster->sendPing())
               {
                  mAster->disconnect();
                  mNotifier->masterDisconnected();
               }
            }
         }
         else
         {
            mAster->disconnect();
            mNotifier->masterDisconnected();
         }

         timeout = !mTrigger.timed_wait(SB_MASTERPING_TIMEOUT);
      }
   }

   mAster->disconnect();
   mNotifier->masterDisconnected();
}



