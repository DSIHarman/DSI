/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_SOCKETCONNECTIONCONTEXT_HPP
#define DSI_SERVICEBROKER_SOCKETCONNECTIONCONTEXT_HPP

#include <unistd.h>

#include "ConnectionContext.hpp"


// forward decl
class ServicebrokerClientBase;


/**
 * Attributes about a client connected to the servicebroker.
 */
class SocketConnectionContext : public ConnectionContext
{   
   friend class SocketMessageContext;
   
public:

   /**
    * @param nodeAddress The node address/id where the connection comes from (in case of ip in host byte order)    
    */
   SocketConnectionContext(ClientSpecificData& data, uint32_t nodeAddress, uint32_t localAddress, ServicebrokerClientBase& client);        
   
   /**
    * The client is connected locally.
    */
   inline
   bool isLocal() const
   {
      uint32_t nd = getNodeAddress() ;
      return nd == SB_LOCAL_NODE_ADDRESS || nd == SB_LOCAL_IP_ADDRESS ;
   }
      
   /**    
    * Get the node id of the peer process. Returns @c SB_LOCAL_NODE_ADDRESS for the local node.
    * In case of IP connection the ip of the remote node is returned in host byte order as retrieved 
    * by the system call @c getpeername().
    */   
   inline
   uint32_t getNodeAddress() const
   {      
      return mNodeAddress;
   }
   
   /**
    * Local IP address in host byte order of the socket connection, as retrieved by the system 
    * call @c getsockname().
    */
   inline
   uint32_t getLocalAddress() const
   {
      return mLocalAddress;
   }
      
private:    

   ServicebrokerClientBase& mClient;
   uint32_t mNodeAddress;
   uint32_t mLocalAddress;
};


#endif   // DSI_SERVICEBROKER_SOCKETCONNECTIONCONTEXT_HPP
