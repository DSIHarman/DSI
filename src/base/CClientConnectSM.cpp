/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "io.hpp"
#include "CClientConnectSM.hpp"
#include "CConnectRequestHandle.hpp"
#include "CDummyChannel.hpp"
#include "CTCPChannel.hpp"
#include "bind.hpp"
#include "CTraceManager.hpp"

#include "dsi/Log.hpp"
#include "dsi/CRequestWriter.hpp"
#include "dsi/CCommEngine.hpp"


using namespace std::tr1::placeholders;


namespace DSI
{
   bool isTCPForced()
   {
      static int forceTCP = -1 ;

      if( -1 == forceTCP )
      {
         const char* f = ::getenv("DSI_FORCE_TCP");
         forceTCP = f ? ::atoi(f) : 0 ;
      }

      return forceTCP > 0;
   }
}


DSI::CClientConnectSM::CClientConnectSM(DSI::CClient& client)
 : mClient(client)
 , mChannel()
 , mBuffer()
{
   ::memset(&mConnInfo, 0, sizeof(mConnInfo));
   ::memset(&mTcpConnInfo, 0, sizeof(mTcpConnInfo));
}


DSI::CClientConnectSM::~CClientConnectSM()
{
   // we must drop it since otherwise we recurse into destructor calls
   (void)mClient.mConnector.release();
}


void DSI::CClientConnectSM::attach()
{
   bool detach = true;
   int ret;

   if ((ret = CServicebroker::attachInterface(mClient.mIfDescription, mConnInfo)) != 0)
   {
      DBG_ERROR(( "Error attaching to interface %s: rc=%d", mClient.mIfDescription.name , ret));
   }
   else
   {
      CTraceManager::add(mConnInfo.clientID, mClient.mIfDescription);
      
      // forced TCP/IP transport?
      if (isTCPForced() || mConnInfo.channel.nid != 0)   // tcp transport adequate?
      {
         if ((ret = CServicebroker::attachInterfaceTCP(mClient.mIfDescription, mTcpConnInfo)) != 0)
         {
            DBG_ERROR(( "Error attaching to interface %s via TCP: rc=%d", mClient.mIfDescription.name , ret));
         }
         else
         {
            mChannel = mClient.mCommEngine->attachTCP(mTcpConnInfo.socket.ipaddress, mTcpConnInfo.socket.port, true);

            if (mChannel)
            {
               if (initiateConnectRequestTCP())
               {
                  detach = false;

                  CServicebroker::detachInterface(mConnInfo.clientID);
                  mConnInfo.clientID = 0;
               }
            }
            else
            {
               DBG_ERROR(("CClient: Error connecting TCP %d.%d.%d.%d:%d",
                    mTcpConnInfo.socket.ipaddress & 0xFF,
                    mTcpConnInfo.socket.ipaddress >> 8  & 0xFF,
                    mTcpConnInfo.socket.ipaddress >> 16 & 0xFF,
                    mTcpConnInfo.socket.ipaddress >> 24,
                    ntohs(mTcpConnInfo.socket.port)));
            }
         }
      }

      if (!mChannel)
      {
         if (mConnInfo.channel.nid == 0)
         {
            // local attach possible?
            mChannel = mClient.mCommEngine->attach(mConnInfo.channel.pid, mConnInfo.channel.chid);

            if (mChannel)
            {               
               if (initiateConnectRequest())
               {
                  detach = false;

                  if (mTcpConnInfo.clientID != 0)
                     CServicebroker::detachInterface(mTcpConnInfo.clientID);
               }
            }
            else
            {
               DBG_ERROR(("Error attaching to interface %s: Invalid Channel (%d, %d, %d)",
                          mClient.mIfDescription.name, mConnInfo.channel.pid, mConnInfo.channel.chid));
            }
         }
      }
   }

   if (detach)
      (void)onFailure();
}


bool DSI::CClientConnectSM::initiateConnectRequest()
{
   TRC_SCOPE( dsi_base, CClient, global );
   DBG_MSG(("CClient::sendConnectRequest() %s %d.%d", mClient.mIfDescription.name, mClient.mIfDescription.version.majorVersion,
            mClient.mIfDescription.version.minorVersion ));
   
   mClient.mClientID = mConnInfo.clientID;   
      
   CRequestWriter writer(*mChannel, DSI::ConnectRequest, mConnInfo.clientID, mConnInfo.serverID, DSI_PROTOCOL_VERSION_MINOR);      
   
   DSI::ConnectRequestInfo* rci = (DSI::ConnectRequestInfo*)writer.pptr();
   rci->pid = ::getpid();
   rci->channel = mClient.mCommEngine->getLocalChannel();
   writer.pbump(sizeof(DSI::ConnectRequestInfo));
      
   DBG_MSG((" pid:%d, chid:%d", rci->pid, rci->channel));   
   
   return writer.flush();
}


bool DSI::CClientConnectSM::initiateConnectRequestTCP()
{
   // it's an asynchronous call - make sure the object is still valid...
   if (mClient.mCommEngine)
   {
      TRC_SCOPE( dsi_base, CClient, global );

      char ipbuf[32];
      CTCPChannel& tcp = *(CTCPChannel*)mChannel.get();

      IPv4::Endpoint ep = tcp.getPeerName();
      DBG_MSG(("CClient::sendConnectRequestTCP() %s:%d", ep.toString(ipbuf)));      

      ep = tcp.getLocalName();
      if (ep)
      {
         struct ExtendedRci
         {
            DSI::TCPConnectRequestInfo rci;
            uint32_t dummy;
         };

         // be aware that the mServerID correlates the mTCPServerID of the server
         CRequestWriter writer(*mChannel, DSI::ConnectRequest, mTcpConnInfo.clientID, mTcpConnInfo.serverID, DSI_PROTOCOL_VERSION_MINOR);            
         
         DSI::TCPConnectRequestInfo* rci = (DSI::TCPConnectRequestInfo*)writer.pptr();
         rci->ipAddress = ep.getIP();
         rci->port = mClient.mCommEngine->getIPPort();
         writer.pbump(sizeof(ExtendedRci));         
         
         if (writer.flush())
         {
            // initiate asynchronous read
            mChannel->asyncRead(this);
            return true;
         }
      }
   }

   return false;
}


void DSI::CClientConnectSM::handleConnectResponse(const CConnectRequestHandle& handle)
{
   if (handle.info().pid)
   {
      mClient.mServerID = handle.getServerID();
      mClient.mChannel = mClient.mCommEngine->attach(handle.info().pid, handle.info().channel);
      uint16_t isprotoMinor = (uint16_t)DSI_PROTOCOL_VERSION_MINOR;
      uint16_t protoMinor = (handle.getProtoMinor() < isprotoMinor) ? handle.getProtoMinor() : isprotoMinor;
      mClient.mProtoMinor = protoMinor;

      (void)finalizeConnectRequest();
   }
   else
      (void)onFailure();
}


bool DSI::CClientConnectSM::onLegacyConnectRequestTCP(DSI::TCPConnectRequestInfo& info)
{
   if (DSI_MESSAGE_MAGIC == info.ipAddress)
   {
      sExtendedTCPRequestInfo aTCPResponse;


      memcpy((void *)&aTCPResponse,(void *)&info, sizeof(info));
      CTCPChannel& tcp = *(CTCPChannel*)mChannel.get();
      tcp.recvAll((char *)&aTCPResponse + sizeof(info), sizeof(aTCPResponse)-sizeof(info));
      if (aTCPResponse.hdr.cmd == DSI::ConnectResponse)
      {
         return onConnectRequestTCP(aTCPResponse);
      }
      else
      {
         return onFailure();
      }
   }
   else
   {
      if (info.ipAddress)
      {
         mClient.mClientID = mTcpConnInfo.clientID;
         mClient.mServerID = mTcpConnInfo.serverID;

         mClient.mChannel = mClient.mCommEngine->attachTCP(info.ipAddress, info.port);
         mClient.mProtoMinor = DSI_PROTOCOL_VERSION_MINOR;

         return finalizeConnectRequest();
      }
      else
         return onFailure();
   }
}

bool DSI::CClientConnectSM::onConnectRequestTCP(sExtendedTCPRequestInfo &extendedInfo)
{
   if (extendedInfo.info.ipAddress)   
   {
      mClient.mClientID = mTcpConnInfo.clientID;
      mClient.mServerID =  mTcpConnInfo.serverID;

      mClient.mChannel = mClient.mCommEngine->attachTCP( extendedInfo.info.ipAddress,  extendedInfo.info.port);
      uint16_t isprotoMinor = (uint16_t)DSI_PROTOCOL_VERSION_MINOR;
      uint16_t protoMinor = (extendedInfo.hdr.protoMinor < isprotoMinor) ? extendedInfo.hdr.protoMinor : isprotoMinor;
      mClient.mProtoMinor = protoMinor;

      return finalizeConnectRequest();
   }
   else
      return onFailure();
}




bool DSI::CClientConnectSM::onFailure()
{
   if (mTcpConnInfo.clientID != 0)
      CServicebroker::detachInterface(mTcpConnInfo.clientID);

   if (mConnInfo.clientID != 0)
   {     
      CServicebroker::detachInterface(mConnInfo.clientID);   
      CTraceManager::remove(mConnInfo.clientID);
   }
      
   mClient.detachInterface(true);      // will also delete the statemachine object
   return false;
}


bool DSI::CClientConnectSM::finalizeConnectRequest()
{
   DBG_MSG(("Connected service %s %d.%d via TCP/IP", mClient.mIfDescription.name,
            mClient.mIfDescription.version.majorVersion, mClient.mIfDescription.version.minorVersion));
   
   // set the server disconnect notification
   mClient.mNotificationID =
      CServicebroker::setServerDisconnectNotification(mClient.mServerID, mClient.mCommEngine->getNotificationSocketChid(), mClient.mId);   
   
   // Notify the proxy about the connection
   mClient.doComponentConnected();

   delete this;
   return false;
}


void DSI::CClientConnectSM::asyncRead(IPv4::StreamSocket& sock)
{
   sock.async_read_all(&mBuffer, sizeof(mBuffer), bind3(&CClientConnectSM::readHandlerTCP, this, _1, _2));
}
