/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/CClient.hpp"
#include "dsi/CRequestWriter.hpp"
#include "dsi/CCommEngine.hpp"
#include "dsi/Log.hpp"

#include "dsi/private/CRequestHandle.hpp"

#include "CTraceManager.hpp"
#include "CDummyChannel.hpp"
#include "CTCPChannel.hpp"
#include "CServicebroker.hpp"
#include "DSI.hpp"
#include "CClientConnectSM.hpp"

#include <cstdio>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

#define netmgr_remote_nd( a, b ) 0


TRC_SCOPE_DEF( dsi_base, CClient, global );
TRC_SCOPE_DEF( dsi_base, CClient, handleDataResponse );


DSI::CClient::CClient( const char* ifname, const char* rolename, int majorVersion, int minorVersion )
 : CBase( ifname, rolename, majorVersion, minorVersion )
 , mNotificationID( 0 )
 , mChannel(CDummyChannel::getInstancePtr())
 , mConnector(0)
 , mProtoMinor(0)
 , d(0)
{
   // NOOP
}


DSI::CClient::~CClient()
{
   if (mCommEngine)
   {
      DSI::CCommEngine* engine = mCommEngine;
      mCommEngine = 0;
      (void)engine->remove(*this);
   }
}


void DSI::CClient::setServerAvailableNotification( int32_t channel )
{
   TRC_SCOPE( dsi_base, CClient, global );
   DBG_MSG(("CClient::setServerAvailableNotification() %s %d.%d chid=%d", mIfDescription.name,
            mIfDescription.version.majorVersion, mIfDescription.version.minorVersion, channel ));

   mNotificationID = CServicebroker::setServerAvailableNotification( mIfDescription, channel, mId );
   if( 0 >= mNotificationID )
   {
      DBG_ERROR(( "Error setting server available notification %s %d.%d", mIfDescription.name,
                  mIfDescription.version.majorVersion, mIfDescription.version.minorVersion ));
   }
}


void DSI::CClient::removeNotification()
{
   TRC_SCOPE( dsi_base, CClient, global );
   if( mNotificationID )
   {
      DBG_MSG(("CClient::removeNotification() %s %d.%d - %d"
            , mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion
            , mNotificationID ));

      CServicebroker::clearNotification( mNotificationID );
      mNotificationID = 0 ;
   }
}


void DSI::CClient::detachInterface(bool resetNotification)
{
   TRC_SCOPE( dsi_base, CClient, global );
   DBG_MSG(("CClient::detachInterface() %s %d.%d", mIfDescription.name, mIfDescription.version.majorVersion,
            mIfDescription.version.minorVersion ));

   removeNotification();

   mConnector.reset(0);    // drop him if he is in action

   if (mClientID)
   {
      (void)sendDisconnectRequest();

      CServicebroker::detachInterface( mClientID );
      CTraceManager::remove(mClientID);
      
      mClientID = 0 ;
   }

   if (mServerID)
   {      
      if (mCommEngine)   // don't call invalidated virtual function in destructor
         doComponentDisconnected();

      mServerID = 0 ;
   }

   if (resetNotification && mCommEngine)
   {
      // Reset the notification to 'server available'
      setServerAvailableNotification(mCommEngine->getNotificationSocketChid());
   }
}


void DSI::CClient::handleDataResponse( Private::CDataResponseHandle &handle )
{
   TRC_SCOPE( dsi_base, CClient, handleDataResponse );
   if (mProtoMinor != handle.getProtoMinor())
   {
      DBG_ERROR(("handleDataResponse: bad protocol version (expected: %d.%d, received: %d.%d)"
                 , DSI_PROTOCOL_VERSION_MAJOR, mProtoMinor
                 , DSI_PROTOCOL_VERSION_MAJOR, handle.getProtoMinor() ));
      return;
   }

   mCurrentSequenceNr = handle.getSequenceNumber();

   DBG_MSG(( "CClient::handleDataResponse() %s %d.%d  %s - %s (0x%08X), seq:%d"
                  , mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion
                  , DSI::toString( handle.getResponseType() )
                  , getUpdateIDString(handle.getRequestId()), handle.getRequestId(), mCurrentSequenceNr ));

   processResponse(handle);
   mCurrentSequenceNr = DSI::INVALID_SEQUENCE_NR ;
}


void DSI::CClient::handleConnectResponse(CConnectRequestHandle& handle)
{
   assert(mConnector.get());
   mConnector->handleConnectResponse(handle);
}


void DSI::CClient::attachInterface()
{
   TRC_SCOPE(dsi_base, CClient, global);
   DBG_MSG(("CClient::attachInterface() %s %d.%d", mIfDescription.name, mIfDescription.version.majorVersion,
             mIfDescription.version.minorVersion));

   assert(mCommEngine);
   assert(!mConnector.get());

   mConnector.reset(new CClientConnectSM(*this));
   mConnector->attach();
}


int32_t DSI::CClient::sendDisconnectRequest()
{
   TRC_SCOPE( dsi_base, CClient, global );
   DBG_MSG(("CClient::sendDisconnectRequest() %s %d.%d", mIfDescription.name, mIfDescription.version.majorVersion,
            mIfDescription.version.minorVersion ));

   if (!mChannel.expired())
   {
      CRequestWriter writer(*mChannel.lock(), DSI::DisconnectRequest, mClientID, mServerID);
      (void)writer.flush();      
   }
   else
   {
      assert(!mChannel.expired());
   }

   return 0 ;
}


void DSI::CClient::setNotification( notificationid_t id )
{
   TRC_SCOPE( dsi_base, CClient, global );
   DBG_MSG(("CClient::setNotification() %s %d.%d - %s (0x%08X)", mIfDescription.name,
            mIfDescription.version.majorVersion, mIfDescription.version.minorVersion, getUpdateIDString(id), id ));

   if (!mChannel.expired())
   {
      CRequestWriter writer(*mChannel.lock(), DSI::REQUEST_NOTIFY, DSI::DataRequest, id, mClientID, mServerID, 
                            DSI::INVALID_SEQUENCE_NR, mProtoMinor);
      (void)writer.flush();      
   }
   else
   {
      assert(!mChannel.expired());
   }

}


void DSI::CClient::clearNotification( notificationid_t id )
{
   TRC_SCOPE( dsi_base, CClient, global );
   DBG_MSG(("CClient::clearNotification() %s %d.%d - %s (0x%08X)", mIfDescription.name,
            mIfDescription.version.majorVersion, mIfDescription.version.minorVersion, getUpdateIDString(id), id ));

   if (!mChannel.expired())
   {
      CRequestWriter writer(*mChannel.lock(), DSI::REQUEST_STOP_NOTIFY, DSI::DataRequest, id, mClientID, mServerID,
                            DSI::INVALID_SEQUENCE_NR);
      (void)writer.flush();
   }
   else
   {
      assert(!mChannel.expired());
   }

}


void DSI::CClient::clearAllNotifications()
{
   TRC_SCOPE( dsi_base, CClient, global );
   DBG_MSG(("CClient::clearAllNotifications() %s %d.%d", mIfDescription.name, mIfDescription.version.majorVersion,
            mIfDescription.version.minorVersion ));

   if (!mChannel.expired())
   {
      CRequestWriter writer(*mChannel.lock(), DSI::REQUEST_STOP_ALL_NOTIFY, DSI::DataRequest, DSI::INVALID_ID, mClientID, mServerID,
                            DSI::INVALID_SEQUENCE_NR) ;
      (void)writer.flush();
   }
   else
   {
      assert(!mChannel.expired());
   }
}


