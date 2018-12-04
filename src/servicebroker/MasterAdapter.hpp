/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_MASTERADAPTER_HPP
#define DSI_SERVICEBROKER_MASTERADAPTER_HPP


#include "config.h"

#include "dsi/private/CNonCopyable.hpp"

#include "JobQueue.hpp"
#include "io.hpp"
#include "WorkerThread.hpp"

// forward decl
class Notifier;
struct MasterConnectorSocket;


/**
 * Master interaction encapsulation on slave side.
 */
class MasterAdapter : public DSI::Private::CNonCopyable
{
public:

   /**
    * @param address either a path to a resource manager(MP) or a ip:port combination (TCP/IP)
    */
   MasterAdapter(const char* address);

   ~MasterAdapter();

   /// connect to the master
   bool connect();

   /// disconnect from the master
   void disconnect();

   /// send a ping command to the servicebroker master
   bool sendPing();

   /// send a ping command including extended ID to the servicebroker master
   bool sendIdPing();

   bool isConnected() const;

   /// retrieve string representation of master, e.g. /master for resourcemanager connection
   inline
   const char* address() const
   {
      return mAddress;
   }

   inline
   void setTrigger(Triggerable& trigger)
   {
      mTrigger = &trigger;
   }

   /// add a job to the internal job queue
   inline
   void eval(Job* job)
   {
      mQueue.push(job);

      if (mTrigger)
         mTrigger->trigger();
   }

   /// remove all pending jobs and send an appropriate notification for each
   void removePending(Notifier& notifier);

   /// execute all pending jobs and send an appropriate notification for each
   bool executePending(Notifier& notifier);

   /// IPv4 address in network byte order. This call is only valid on IP connections,
   /// resource manager connections will yield an assertions.
   uint32_t getMasterAddress() const;

   /// @return the number of jobs in the queue
   unsigned int getJobQueueSize() const;

   //@return the total statistical counter of the queue
   unsigned int getJobQueueTotalCounter() const;

private:

   /// execute a single job and send an appropriate notification
   bool execute(Job& job, Notifier& notifier);

   std::auto_ptr<MasterConnectorSocket> mConnector;
   const char* mAddress;

   JobQueue mQueue;
   Triggerable* mTrigger;
};


inline
unsigned int MasterAdapter::getJobQueueSize() const
{
   return mQueue.getSize();
}


inline
unsigned int MasterAdapter::getJobQueueTotalCounter() const
{
   return mQueue.getTotalCounter();
}

#endif   // DSI_SERVICEBROKER_MASTERADAPTER_HPP
