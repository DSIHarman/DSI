/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/DSI.hpp"
#include "dsi/Log.hpp"

#include <cstring>
#include <cstdlib>

#include "CServicebroker.hpp"
#include "LockGuard.hpp"
#include "RecursiveMutex.hpp"
#include "DSI.hpp"


TRC_SCOPE_DEF( dsi_base, CServicebroker, global );


namespace /*anonymous*/
{
   DSI::RecursiveMutex SBAccessLock;
   int SBHandle = -1;
}


int DSI::CServicebroker::GetSBHandle()
{
   // lock the handle. this function is called from within two different threads
   LockGuard<> lock(SBAccessLock);

   if( SBHandle <= 0 )
   {
      // try to open the servicebroker
      const char* mountpoint = ::getenv("DSI_SERVICEBROKER");

      if (!mountpoint)
         mountpoint = FND_SERVICEBROKER_PATHNAME;

      SBHandle = SBOpen( mountpoint );

      if( -1 == SBHandle )
      {
         Log::syslog(Log::Critical, "DSI: Error opening servicebroker (%s)", mountpoint );
      }
   }

   return SBHandle;
}


void DSI::CServicebroker::closeHandle()
{
   LockGuard<> lock(SBAccessLock);

   SBClose(SBHandle);
   SBHandle = -1 ;
}


bool DSI::CServicebroker::registerInterface( CServer& server, int32_t chid, const std::string& userGroup )
{
   // Call service broker and handle result
   int retval = -1 ;
   if( !userGroup.empty() )
   {
      retval = SBRegisterGroupInterface( GetSBHandle()
                                       , server.mIfDescription.name
                                       , server.mIfDescription.version.majorVersion
                                       , server.mIfDescription.version.minorVersion
                                       , chid
                                       , userGroup.c_str()
                                       , &server.mServerID );
   }
   else
   {
      retval = SBRegisterInterface( GetSBHandle()
                                  , server.mIfDescription.name
                                  , server.mIfDescription.version.majorVersion
                                  , server.mIfDescription.version.minorVersion
                                  , chid
                                  , &server.mServerID );
   }

   return retval == 0;
}


bool DSI::CServicebroker::registerInterface( std::vector<CServer*>& serverlist, int32_t chid )
{
   TRC_SCOPE( dsi_base, CServicebroker, global );
   bool retval = true ;

   if( 0 < serverlist.size() )
   {
      int allocationSize = serverlist.size() * sizeof(SFNDInterfaceDescription);
      SFNDInterfaceDescription *ifs = (SFNDInterfaceDescription*)alloca( allocationSize );

      allocationSize = serverlist.size() * sizeof(SPartyID);
      SPartyID *serverIDs = (SPartyID*)alloca(allocationSize);
      if( !ifs || !serverIDs )
      {
         DBG_ERROR(( "alloca: Out of memory" ));
         retval = false ;
      }
      else
      {
         for( unsigned int idx=0; idx<serverlist.size(); idx++ )
         {
            *(ifs+idx) = serverlist[idx]->mIfDescription ;
         }
         if( 0 != SBRegisterInterfaceEx( GetSBHandle(), ifs, serverlist.size(), chid, serverIDs ))
         {
            DBG_ERROR(( "SBRegisterInterfaceEx failed" ));
            retval = false ;
         }
         else
         {
            for( unsigned int idx=0; idx<serverlist.size(); idx++ )
            {
               serverlist[idx]->mServerID = serverIDs[idx] ;
            }
         }
      }
   }
   return retval ;
}


bool DSI::CServicebroker::registerInterfaceTCP( CServer& server, uint32_t address, uint32_t port )
{
   // Call service broker and handle result
   int retval = SBRegisterInterfaceTCP( GetSBHandle()
                                      , server.mIfDescription.name
                                      , server.mIfDescription.version.majorVersion
                                      , server.mIfDescription.version.minorVersion
                                      , address
                                      , port
                                      , &server.mTCPServerID );
   return 0 == retval ;
}


void DSI::CServicebroker::unregisterInterface( const SPartyID& serverID )
{
   SBUnregisterInterface( GetSBHandle(), serverID );
}


notificationid_t DSI::CServicebroker::setServerAvailableNotification( SFNDInterfaceDescription& ifDescription, int32_t chid, int32_t pulseValue )
{
   notificationid_t notificationID = 0 ;
   SBSetServerAvailableNotification( GetSBHandle()
                                   , ifDescription.name
                                   , ifDescription.version.majorVersion
                                   , ifDescription.version.minorVersion
                                   , chid
                                   , static_cast<int32_t>(DSI::PULSE_SERVER_AVAILABLE)
                                   , pulseValue
                                   , &notificationID );
   return notificationID ;
}


notificationid_t DSI::CServicebroker::setClientDetachNotification( const SPartyID &clientID, int32_t chid, int32_t pulseValue )
{
   notificationid_t notificationID = 0;

   (void)SBSetClientDetachNotification( GetSBHandle()
                                      , clientID
                                      , chid
                                      , static_cast<int32_t>(DSI::PULSE_CLIENT_DETACHED)
                                      , pulseValue
                                      , &notificationID );

   return notificationID ;
}


notificationid_t DSI::CServicebroker::setServerDisconnectNotification( const SPartyID &serverID, int32_t chid, int32_t pulseValue )
{
   notificationid_t notificationID = 0;

   (void)SBSetServerDisconnectNotification( GetSBHandle()
                                          , serverID
                                          , chid
                                          , static_cast<int32_t>(DSI::PULSE_SERVER_DISCONNECT)
                                          , pulseValue
                                          , &notificationID );

   return notificationID ;
}



void DSI::CServicebroker::clearNotification(notificationid_t notificationID )
{
   (void)SBClearNotification( GetSBHandle(), notificationID );
}


void DSI::CServicebroker::detachInterface( const SPartyID& clientID )
{
   (void)SBDetachInterface( GetSBHandle(), clientID );
}


int DSI::CServicebroker::attachInterface( SFNDInterfaceDescription& ifDescription, SConnectionInfo &connInfo )
{
   return SBAttachInterface( GetSBHandle()
                           , ifDescription.name
                           , ifDescription.version.majorVersion
                           , ifDescription.version.minorVersion
                           , &connInfo );
}


int DSI::CServicebroker::attachInterfaceTCP( SFNDInterfaceDescription& ifDescription, STCPConnectionInfo &connInfo )
{
   return SBAttachInterfaceTCP( GetSBHandle()
                           , ifDescription.name
                           , ifDescription.version.majorVersion
                           , ifDescription.version.minorVersion
                           , &connInfo );
}

