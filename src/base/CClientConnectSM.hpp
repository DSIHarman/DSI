/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_BASE_CCLIENTCONNECTSM_HPP
#define DSI_BASE_CCLIENTCONNECTSM_HPP

#include "dsi/CClient.hpp"

#include "DSI.hpp"
#include "CServicebroker.hpp"


namespace DSI
{

/**
 * Abstraction for the statemachine necessary to step through in order to access a service. The
 * following steps are necessary:
 * 
 * <ul>
 *   <li>(asnychronously) connect a private temporary channel to the server's communication engine
 *   <li>send a connect request
 *   <li>wait for the response from the server
 *   <li>connect a (potentially shared) channel to the server's communication engine
 * </ul>
 *
 * @note we assume the servicebroker to be reactive so we don't have asynchronous servicebroker calls in here.
 */
class CClientConnectSM
{
public:

   explicit
   CClientConnectSM(CClient& client);
   
   /// Unregister from the client given in the destructor.
   ~CClientConnectSM();
   
   /// Main starting point for statemachine.
   void attach();   

   /// Initiate an asynchronous read of the result of the TCPConnectRequest.
   void asyncRead(IPv4::StreamSocket& sock);
      
   void handleConnectResponse(const CConnectRequestHandle& handle);

   struct sExtendedTCPRequestInfo
   {
      DSI::MessageHeader hdr;
      DSI::TCPConnectRequestInfo info;
   };   
  
private:   

   bool initiateConnectRequest();   
   bool initiateConnectRequestTCP();
      
   bool onLegacyConnectRequestTCP(TCPConnectRequestInfo& handle);      
      
   bool onFailure();   
   bool finalizeConnectRequest();
     
   bool readHandlerTCP(size_t /*len*/, io::error_code ec)
   {   
      return ec == io::ok ? onLegacyConnectRequestTCP(mBuffer) : onFailure();            
   }
   
   
   CClient& mClient;                  ///< The client we belong to.
   
   SConnectionInfo mConnInfo;
   STCPConnectionInfo mTcpConnInfo;
   
   /// channel to use for connect request on local connections
   std::tr1::shared_ptr<CChannel> mChannel;
      
   /// receive buffer - there is no DSI message header at all when receiving (TCP)ConnectRequests.   

   bool onConnectRequestTCP( sExtendedTCPRequestInfo& handle);

   TCPConnectRequestInfo mBuffer;      
};

}   // namespace DSI


#endif   // DSI_BASE_CCLIENTCONNECTSM_HPP
