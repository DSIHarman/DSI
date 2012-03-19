/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "ClientSpecificData.hpp"

#include "Log.hpp"
#include "Servicebroker.hpp"
#include "JobQueue.hpp"


ClientSpecificData::ClientSpecificData()
 : mExtendedID(0)
{
   // NOOP
}


ClientSpecificData::~ClientSpecificData()
{
   // NOOP
}


void ClientSpecificData::clear(bool isSlave)
{   
   for(PartyIDList::const_iterator iter = mServerList.begin(); iter != mServerList.end(); ++iter)
   {
      SPartyID serverID = *iter;
      Log::message( 2, "+CLOSE SERVER_ID: <%d.%d>", serverID.s.extendedID, serverID.s.localID );

      ServerListEntry *entry = Servicebroker::getInstance().mConnectedServers.find( serverID );
      if( entry )
      {
         Servicebroker::getInstance().unregisterInterfaceMaster( *entry );
         Servicebroker::getInstance().mConnectedServers.remove( serverID );
         Servicebroker::getInstance().mAttachedClients.removeServer(serverID);
      }
   }

   /* remove all attached clients */
   for(PartyIDList::const_iterator iter = mClientList.begin(); iter != mClientList.end(); ++iter)
   {
      SPartyID clientID = *iter;
      Log::message( 2, "+CLOSE CLIENT_ID: <%d.%d>", clientID.s.extendedID, clientID.s.localID );
      Servicebroker::getInstance().mAttachedClients.remove( clientID );

      if( Servicebroker::getInstance().isMasterConnected() )
      {
         SFNDInterfaceDetachArg arg ;
         memset( &arg, 0, sizeof(arg));
         arg.i.sbVersion.majorVersion = DSI_SERVICEBROKER_VERSION_MAJOR;
         arg.i.sbVersion.minorVersion = DSI_SERVICEBROKER_VERSION_MINOR;
         arg.i.clientID = clientID ;
         Job *job = Job::create( 0, DCMD_FND_DETACH_INTERFACE, &arg, sizeof(arg.i) );
         Servicebroker::getInstance().master().eval(job);
      }
   }

   bool bServerListChanged = ( 0 != mServerList.size() ) ;

   /* Server IDs owned by this connection are invalid now. Send notifications. */
   Servicebroker::getInstance().mServerDisconnectNotifications.trigger( mServerList );

   /* Client IDs owned by this connection are invalid now. Send notificatons. */
#ifdef USE_SERVER_CACHE
   if (Servicebroker::getInstance().mUseServerCache && isSlave)
   {
      //is a slave SB Ocb closed then send all detach notifications set by the slave SB
      Servicebroker::getInstance().mClientDetachNotifications.triggerLocal(mExtendedID);
   }
   else
   {
      Servicebroker::getInstance().mClientDetachNotifications.trigger( mClientList );
   }
#else
   (void)isSlave;
   Servicebroker::getInstance().mClientDetachNotifications.trigger( mClientList );
#endif

   /* remove all pending notifications */
   Servicebroker::getInstance().mServerDisconnectNotifications.remove( mNotificationList );
   Servicebroker::getInstance().mClientDetachNotifications.remove( mNotificationList );
   Servicebroker::getInstance().mServerConnectNotifications.remove( mNotificationList );
   Servicebroker::getInstance().mServerListChangeNotifications.remove( mNotificationList );

   if( bServerListChanged )
   {
      /* triger serverlist change notifications */
      Servicebroker::getInstance().mServerListChangeNotifications.triggerAll();
   }
}
