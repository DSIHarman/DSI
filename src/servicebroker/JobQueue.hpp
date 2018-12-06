/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_JOBQUEUE_HPP
#define DSI_SERVICEBROKER_JOBQUEUE_HPP

#include "dsi/clientlib.h"

#include "config.h"

#include "dsi/private/CNonCopyable.hpp"

#include "RecursiveMutex.hpp"
#include "RegExp.hpp"
#include "SocketMessageContext.hpp"


// forward decl
class ClientSpecificData;


/*
 * the Job Object
 */
class Job : public DSI::Private::CNonCopyable
{
public:

   /**
    * Creates a new job which also receives data to be sent back to the client
    */
   static Job* create( SocketMessageContext& msg, int cookie, dcmd_t dcmd, void* parg, size_t sbytes, size_t rbytes = 0 );

   /**
    * Creates a job which is
    */
   static Job* create( int cookie, dcmd_t dcmd, void* parg, size_t sbytes, size_t rbytes = 0 );

   /*
    * free the Job
    */
   void free();

   void finalize();

   ClientSpecificData& getClientSpecificData();

   const SocketConnectionContext& getConnectionContext();

   // cookie for this job
   int cookie;

   SocketMessageContext ctx;

   // the devctl command of the job
   dcmd_t dcmd;

   // the return status of the devctl call or any other os transport errno
   int status;

   // the return value of the server to the client (FNDxxx value)
   int ret_val;

   // how many bytes to send
   size_t sbytes;

   // how many bytes to receive
   size_t rbytes;

   // regular expression needed for some calls
   RegExp regex;

   // this is an intrusive container
   Job* next;

   // the argument to the devctl call
   void* arg;

private:

   Job(int cookie, SocketMessageContext* ctx, dcmd_t dcmd, size_t sbytes, size_t rbytes){}

};


/*
 * the Job Queue
 */
class JobQueue : public DSI::Private::CNonCopyable
{
public:

   JobQueue();
   ~JobQueue();

   /*
    * adds a new job to the job queue
    */
   void push(Job* job);

   /*
    * returns and removes the next job from the job queue
    */
   Job* pop();

   ///Getter method.
   unsigned int getTotalCounter() const;

   ///@return size of the queue
   unsigned int getSize() const;

private:
   Job* mFront;
   Job* mBack;
   unsigned int mSize;

   // the critical section that protects the job queue
   DSI::RecursiveMutex mQueueLock ;

   //statistical counter
   unsigned int mTotalCounter;
};


inline
unsigned int JobQueue::getTotalCounter() const
{
   return mTotalCounter;
}


inline
unsigned int JobQueue::getSize() const
{
   return mSize;
}


#endif // DSI_SERVICEBROKER_JOBQUEUE_HPP
