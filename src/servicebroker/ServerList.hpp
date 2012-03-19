/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_SERVERLIST_HPP
#define DSI_SERVICEBROKER_SERVERLIST_HPP


#include <sstream>
#include <list>

#include "InterfaceDescription.hpp"


/**
 * @internal
 *
 * @brief Describes a single entry in the server table.
 */
struct ServerListEntry
{
   /** create a new entry */
   ServerListEntry();
   
   /** internal unique id of the entry */
   int32_t id ;
   /** The server ID of this entry. */
   SPartyID partyID;
   /** The server ID on the master of this entry. */
   SPartyID masterID;
   /** The node ID of the remote node. */
   int32_t nid;
   /** The process ID of the server process. */
   pid_t pid;
   /** The channel ID of the server. */
   int32_t chid;
   /** The interface major version number. */
   InterfaceDescription ifDescription ;
   /** The implementation major version number. */
   SFNDImplementationVersion implVersion ;
   /** the group id of -1 if everybody has access */
   int32_t grpid;
   /** is the service local? */
   bool local ;

   inline 
   operator const char*() const
   {
      return ifDescription.name.c_str() ;
   }

   inline 
   operator const SPartyID&() const
   {
      return partyID ;
   }
} ;


/**
 * @internal
 *
 * @brief Describes a server table.
 */
class ServerList
{
   typedef std::list<ServerListEntry> tContainerType;

public:

   typedef tContainerType::const_iterator const_iterator;
   typedef tContainerType::iterator       iterator;

   ServerList();
   ~ServerList();

   /**
    * @internal
    *
    * @brief Adds a new entry to a server table.
    *
    * @param entry The new entry to insert
    */
   void add( const ServerListEntry& entry );

   /**
    * @internal
    *
    * @brief Finds an entry in a server table.
    *
    * @param ifDescription description of the interface
    *
    * @return pointer to the table entry matching name/verion or NULL if not found
    */
   ServerListEntry* find( const SFNDInterfaceDescription &ifDescription );

   /**
    * @internal
    *
    * @brief Find an entry in the server table by its server ID.
    *
    * @param serverID The server ID of the entry to find.
    *
    * @return Pointer to the wanted entry or NULL if the entry does not exist
    */
   ServerListEntry* find( const SPartyID& serverID );

   ServerListEntry* findByLocalID( uint32_t localID );

   ServerListEntry* findByLocalIDandInvalidMasterID( uint32_t localID );
   /**
    * @internal
    *
    * @brief Find an entry in the server table by its name.
    *
    * @param ifName the name of the entry to find.
    *
    * @return Pointer to the wanted entry or NULL if the entry does not exist
    */
   ServerListEntry* find( const char* ifName );

   /**
    * @internal
    *
    * @brief Find an entry in the server table by its internal id.
    *
    * @param ifName the internal id of the entry to find.
    *
    * @return Pointer to the wanted entry or NULL if the entry does not exist
    */
   ServerListEntry* find( int32_t id );

   /**
    * @internal
    *
    * @brief Removes an entry from a table.
    *
    * @param serverID The server id of the entry to be removed
    */
   void remove( const SPartyID& serverID );   

   /**
    * @internal
    *
    * @brief sets the master ids of all entries in the list to 0
    */
   void clearMasterIDs();

   /**
    * @internal
    *
    * @brief writes server statistics to a buffer
    */
   void dumpStats( std::ostringstream& ostream ) const;

   inline
   const_iterator begin() const
   {
      return mEntries.begin();
   }

   inline
   const_iterator end() const
   {
      return mEntries.end();
   }

   inline
   iterator begin()
   {
      return mEntries.begin();
   }

   inline
   iterator end()
   {
      return mEntries.end();
   }

   inline
   size_t size() const
   {
      return mEntries.size();
   }

   unsigned int getTotalCounter() const;

private:

   tContainerType mEntries;

   //statistical counters
   unsigned int mTotalCounter;
};


inline 
unsigned int ServerList::getTotalCounter() const
{
   return mTotalCounter;
}

#endif /* DSI_SERVICEBROKER_SERVERLIST_HPP */
