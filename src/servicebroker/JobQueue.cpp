/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "JobQueue.hpp"

#include "LockGuard.hpp"

#include "MessageContext.hpp"
#include "ClientSpecificData.hpp"

#define __max(x,y) (x) > (y) ? (x) : (y)

using namespace DSI;




/*static*/
Job* Job::create(SocketMessageContext& msg, int theCookie, dcmd_t theDcmd, void* parg, size_t theSbytes, size_t theRbytes)
{
  Job* job = (Job*) ::malloc( sizeof(Job) + static_cast<size_t>(__max(theSbytes, theRbytes)) );
  if( job )
  {
  /// CMakeFiles/servicebroker.dir/JobQueue.cpp.o:
  /// In function `Job::create(SocketMessageContext&, int, int, void*, unsigned long, unsigned long)':
  /// /opt/DSI/src/servicebroker/JobQueue.cpp:29: undefined reference to
  /// `Job::Job(int, SocketMessageContext*, int, unsigned long, unsigned long)'
  /// int cookie, SocketMessageContext* ctx, dcmd_t dcmd, size_t sbytes, size_t rbytes
    Job* tmp = new Job(theCookie, &msg, theDcmd, theSbytes, theSbytes);
    new(job) Job(theCookie, &msg, theDcmd, theSbytes, theRbytes);
    memset( job+1, 0, __max(theSbytes, theRbytes) );

    if( theSbytes > 0 )
    {
       memcpy( job->arg, parg, theSbytes );
    }

  }

  return job ;
}


/*static*/
Job* Job::create( int theCookie, dcmd_t theDcmd, void* parg, size_t theSbytes, size_t theRbytes)
{
  Job* job = (Job*) ::malloc( sizeof(Job) + static_cast<size_t> (__max(theSbytes, theRbytes)) );
  if( job )
  {
     /// JobQueue.cpp: undefined reference to `Job::Job(int, SocketMessageContext*, int, unsigned long, unsigned long)'
    new(job) Job(theCookie, nullptr, theDcmd, theSbytes, theRbytes);
    memset( job+1, 0, __max(theSbytes, theRbytes) );

    if( theSbytes > 0 )
    {
       memcpy( job->arg, parg, theSbytes );
    }
  }

  return job ;
}


void Job::free()
{
  this->~Job();
  ::free( this );
}


void Job::finalize()
{
  if (ctx)
    ctx.finalize(*this);
}


ClientSpecificData& Job::getClientSpecificData()
{
  assert(ctx);
  return ctx.context().getData();
}


const SocketConnectionContext& Job::getConnectionContext()
{
  assert(ctx);
  return ctx.context();
}



// ------------------------------------------------------------------------------


JobQueue::JobQueue()
 : mFront(nullptr)
 , mBack(nullptr)
 , mSize(0)
 , mTotalCounter(0)
{
  // NOOP
}


JobQueue::~JobQueue()
{
  // NOOP
}


void JobQueue::push(Job* job)
{
  LockGuard<RecursiveMutex> lock( mQueueLock );

  assert(job);
  assert(!job->next);

  // queue empty?
  if (mBack == nullptr)
  {
    mFront = job;
  }
  else
  {
     mBack->next = job;
  }

  mBack = job;

  ++mSize;
  ++mTotalCounter;
}


Job* JobQueue::pop()
{
  Job* job = nullptr;

  LockGuard<RecursiveMutex> lock( mQueueLock );

  if (mFront)
  {
    job = mFront;
    mFront = job->next;
    job->next = nullptr;

    // queue now empty?
    if (!mFront)
    {
       mBack = nullptr;
    }

    --mSize;
  }

  return job;
}

