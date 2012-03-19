/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_CONNECTIONCONTEXT_HPP
#define DSI_SERVICEBROKER_CONNECTIONCONTEXT_HPP

#include <unistd.h>

#include "config.h"


// forward decl
class ServicebrokerClientBase;
class ClientSpecificData;
class ResMgrConnectionContext;
class SocketConnectionContext;


/**
 * Attributes about a client connected to the servicebroker.
 */
class ConnectionContext
{
   friend class MessageContext;

public:
   
   ConnectionContext(ClientSpecificData& data, pid_t pid = -1);

   /**
    * Internal debugging id
    */
   inline
   int getId() const
   {
      return mId;
   }

   /**
    * Set pid and uid of the connected process.
    */
   inline
   void init(pid_t pid, uid_t uid)
   {
      mPid = pid;
      mUid = uid;
   }

   /**
    * Get the pid of the peer process.
    */
   inline
   pid_t getPid() const
   {
      return mPid;
   }   

   inline
   uid_t getEffectiveUid() const
   {
      return mUid;
   }

   inline
   ~ConnectionContext()
   {
      // NOOP
   }   

   /**
    * This client connection is a servicebroker slave and not any other DSI application.
    */
   inline
   bool isSlave() const
   {
      return mSlave;
   }

   /**
    * This connection is marked as a SB slave connected to this master servicebroker.
    */
   inline
   void setSlave(bool slave = true)
   {
      mSlave = slave;
   }

   inline
   ClientSpecificData& getData() const
   {
      return mData;
   }


protected:

   ClientSpecificData& mData;

   bool mSlave;
   int mId;
   mutable uid_t mUid;
   pid_t mPid;
};


#endif   // DSI_SERVICEBROKER_CONNECTIONCONTEXT_HPP
