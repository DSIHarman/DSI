/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_PULSECHANNELMANAGER_HPP
#define DSI_SERVICEBROKER_PULSECHANNELMANAGER_HPP


#include <vector>

#include "config.h"
#include "SocketConnectionContext.hpp"

#include "dsi/private/servicebroker.h"


/// @internal for PulseChannelManager
struct PulseChannel
{
   PulseChannel(int theFd, int theNid, int thePid, int theChid)
    : nid(theNid)
    , pid(thePid)
    , chid(theChid)
    , fd(theFd)
    , refCount(1)
   {
      // NOOP
   }
   
   int nid;
   int pid;
   int chid;
   
   int fd;
   int refCount;
};


/**
 * Handles channel and socket connections.
 * Connection pool to client applications to avoid multiple connections to the same client in socket mode. 
 */
class PulseChannelManager
{
public:

   typedef std::vector<PulseChannel> tPulseChannelListType;

   /**
    * Singleton.
    */
   static PulseChannelManager& getInstance();
   
   /**
    * Get a connection to the client identified by nid, pid and chid.
    * If nid refers to the local node then pid and chid identify a local socket,
    * otherwise pid and chid identify a TCP socket connection with pid=ip and chid=port.
    *
    * @return a hopefully valid socket descriptor.
    */
   int attach(int nid, int pid, int chid);
      
   /**
    * Detach from the given socket descriptor.
    */
   void detach(int handle);
   
   /**
    * Send a pulse on the given handle
    */
   void sendPulse(int handle, notificationid_t notificationID, int code, int value);
   
private:

   PulseChannelManager();   
      
   tPulseChannelListType mSockets;      
};


#endif   // DSI_SERVICEBROKER_PULSECHANNELMANAGER_HPP
