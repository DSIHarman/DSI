/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_CLIENTSPECIFICDATA_HPP
#define DSI_SERVICEBROKER_CLIENTSPECIFICDATA_HPP


#include <algorithm>

#include "NotificationList.hpp"
#include "ServerList.hpp"
#include "IDList.hpp"


/**
 * All client specific data hold within a client handle on the servicebroker.
 */
class ClientSpecificData
{
public:

   ClientSpecificData();
   ~ClientSpecificData();

   /*
    * Methods for adding/removing/querying notification ids
    */
   void addNotification( notificationid_t notificationId );
   void removeNotification( notificationid_t notificationId );
   bool notificationExists( notificationid_t notificationId );

   /*
    * Methods for adding/removing/querying server ids
    */
   void addServer( const SPartyID& serverId );
   void removeServer( const SPartyID& serverId );
   bool serverExists( const SPartyID& serverId );

   /*
    * Methods for adding/removing/querying client ids
    */
   void addClient( const SPartyID& clientId );
   void removeClient( const SPartyID& clientId );
   bool clientExists( const SPartyID& clientId );

   void setExtendedID(int32_t extendedId);
   int32_t getExtendedID() const;

   //the client is a slave servicebroker
   void clear(bool isSlave = false);

private:

   /** Contains all registered servers. */
   PartyIDList mServerList;

   /** Contains all notifications. */
   NotificationIDList mNotificationList;

   /** Contains all attached clients. */
   PartyIDList mClientList;

   int32_t mExtendedID;   ///< the extended ID of this client

   ClientSpecificData( const ClientSpecificData& );
   ClientSpecificData& operator=( const ClientSpecificData& );
};


// ----------------------------------------------------------------------------------


inline
void ClientSpecificData::setExtendedID(int32_t extendedId)
{
   mExtendedID = extendedId;
}


inline
int32_t ClientSpecificData::getExtendedID() const
{
   return mExtendedID;
}


inline
void ClientSpecificData::addNotification( notificationid_t notificationId )
{
   mNotificationList.push_back( notificationId );
}


inline
void ClientSpecificData::removeNotification( notificationid_t notificationId )
{
   std::remove(mNotificationList.begin(), mNotificationList.end(), notificationId);   
}


inline
bool ClientSpecificData::notificationExists( notificationid_t notificationId )
{
   return std::find(mNotificationList.begin(), mNotificationList.end(), notificationId) != mNotificationList.end();   
}


inline
void ClientSpecificData::addServer( const SPartyID& serverId )
{
   mServerList.insert( serverId );
}


inline
void ClientSpecificData::removeServer( const SPartyID& serverId )
{
   mServerList.remove(serverId);
}


inline
bool ClientSpecificData::serverExists( const SPartyID& serverId )
{
   return mServerList.find(serverId) != mServerList.end();
}


inline
void ClientSpecificData::addClient( const SPartyID& clientId )
{
   mClientList.insert( clientId );
}


inline
void ClientSpecificData::removeClient( const SPartyID& clientId )
{
   mClientList.remove(clientId);   
}


inline
bool ClientSpecificData::clientExists( const SPartyID& clientId )
{
   return mClientList.find(clientId) != mClientList.end();   
}


#endif // DSI_SERVICEBROKER_CLIENTSPECIFICDATA_HPP

