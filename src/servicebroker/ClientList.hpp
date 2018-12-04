/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_CLIENTLIST_HPP
#define DSI_SERVICEBROKER_CLIENTLIST_HPP

#include "dsi/private/servicebroker.h"

#include <vector>
#include <algorithm>
#include <sstream>

#include "config.h"


class SocketConnectionContext;


class ClientListEntry
{
public:

   inline
   ClientListEntry( const SPartyID& clientId, const SPartyID& serverId, const SocketConnectionContext& context )
    : mClientId(clientId)
    , mServerId(serverId)
    , mCtx(&context)
   {
      // NOOP
   }


   inline
   ClientListEntry( const SPartyID& clientId )
    : mClientId(clientId)
    , mCtx(0)
   {
      mServerId.globalID = 0 ;
   }


   inline
   bool operator== ( const ClientListEntry& rhs ) const
   {
      return mClientId == rhs.mClientId ;
   }

   
   SPartyID mClientId ;
   SPartyID mServerId ;

   const SocketConnectionContext* mCtx;
};


class ClientList
{
public:

   void push_back( const SPartyID& clientId, const SPartyID& serverId, const SocketConnectionContext& context );

   void remove( const SPartyID& clientId );  

   //remove all clients connected to the server
   void removeServer( const SPartyID& serverId );  

   //remove all clients that have a connection with a remote servicebroker
   void removeNotLocal(uint32_t extendedId);

   /**
    * @internal
    *
    * @brief writes client statistics to a buffer
    */
   void dumpStats( std::ostringstream& ostream ) const;

   inline
   size_t size() const
   {
      return mEntries.size();
   }

   inline
   const ClientListEntry& operator[](size_t idx) const
   {
      return mEntries[idx];
   }
   
   int find(const SPartyID& id) const
   {
      std::vector<ClientListEntry>::const_iterator iter = std::find(mEntries.begin(), mEntries.end(), ClientListEntry(id));      
      return iter == mEntries.end() ? -1 : std::distance(mEntries.begin(), iter);
   }

   unsigned int getTotalCounter() const;

private:

   std::vector<ClientListEntry> mEntries;

   //statistical counter
   unsigned int mTotalCounter;
};


inline
void ClientList::push_back( const SPartyID& clientId, const SPartyID& serverId, const SocketConnectionContext& context )
{
   mEntries.push_back( ClientListEntry( clientId, serverId, context ) );
   mTotalCounter++;
}


inline 
unsigned int ClientList::getTotalCounter() const
{
   return mTotalCounter;
}


#endif /* DSI_SERVICEBROKER_CLIENTLIST_HPP */
