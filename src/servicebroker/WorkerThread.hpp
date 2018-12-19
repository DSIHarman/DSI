/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_WORKERTHREAD_HPP
#define DSI_SERVICEBROKER_WORKERTHREAD_HPP

#include <memory>

#include "dsi/private/CNonCopyable.hpp"

#include "JobQueue.hpp"
#include "Notifier.hpp"

#include "Thread.hpp"
#include "Trigger.hpp"


// forward decl
class MasterAdapter;


/**
 * A triggerable object.
 */
class Triggerable
{
public:

   // trigger the workerthread to do some work
   inline
   void trigger()
   {
      (void)mTrigger.signal();
   }

protected:

   inline
   Triggerable()
   {
      // NOOP
   }

   inline
   ~Triggerable()
   {
      // NOOP
   }

   // trigger semaphore for the job queue
   DSI::Trigger mTrigger ;
};


/**
 * The Job Worker Thread.
 */
class WorkerThread : public Triggerable, public DSI::Private::CNonCopyable
{
public:
   WorkerThread();
   ~WorkerThread();

   // starts the worker thread, fd is a descriptor to a pipe/queue/channel like kernel object (implementation dependent)
   void start(int fd, MasterAdapter& master);

   // stops the worker thread
   void stop();

   inline
   Notifier& getNotifier()
   {
      return *mNotifier;
   }

   inline
   operator const void*() const
   {
      return mThread ? this : nullptr;
   }

private:

   void run();

   // flag that indicates if the thread can keep on running
   volatile bool mActive;

   // the thread object
   DSI::Thread mThread;

   // the master communication code abstraction
   MasterAdapter* mAster;

   // internal event notifier
   std::unique_ptr<Notifier> mNotifier;
};


#endif // DSI_SERVICEBROKER_WORKERTHREAD_HPP
