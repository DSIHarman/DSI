/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "ServerList.hpp"

#include <algorithm>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <errno.h>

#include "config.h"
#include "util.hpp"


namespace /*anonymous*/
{
   uint32_t sNextID = 1 ;
}


static inline
bool compare( const char* lhs, const char* rhs )
{
   return 0 < strcmp( lhs, rhs );  
}


ServerListEntry::ServerListEntry()
 : id(++sNextID)  
 , nid(0)
 , pid(0)
 , chid(0) 
 , grpid(SB_UNKNOWN_GROUP_ID)   
 , local(false)
{   
   partyID = 0;
   masterID = 0;
}


// --------------------------------------------------------------------------


ServerList::ServerList()
 : mTotalCounter(0u)
{
   // NOOP
}


ServerList::~ServerList()
{
   // NOOP
}


void ServerList::add( const ServerListEntry& entry )
{
   (void)mEntries.insert(std::lower_bound(mEntries.begin(), mEntries.end(), entry, compare), entry);
   mTotalCounter++;
}


ServerListEntry* ServerList::find( const SFNDInterfaceDescription &ifDescription )
{
   ServerListEntry* result = find(ifDescription.name);
   if( result
      && result->ifDescription.majorVersion == ifDescription.version.majorVersion
      && result->ifDescription.minorVersion >= ifDescription.version.minorVersion )
   {
      return result ;
   }

   return nullptr;
}


ServerListEntry* ServerList::find(const char* ifName)
{   
   tContainerType::iterator iter = std::lower_bound(mEntries.begin(), mEntries.end(), ifName, compare);
   return (iter != mEntries.end() && !compare(ifName, *iter)) ? &*iter : nullptr;
}


ServerListEntry* ServerList::find( const SPartyID& serverID )
{
   for( tContainerType::iterator iter = mEntries.begin(); iter != mEntries.end(); ++iter )
   {
      if( serverID.globalID == iter->partyID.globalID )
         return &(*iter) ;
   }

   return nullptr;
}


ServerListEntry* ServerList::findByLocalID( uint32_t localID )
{
   for( tContainerType::iterator iter = mEntries.begin(); iter != mEntries.end(); ++iter )
   {
      if( localID == iter->partyID.s.localID )
         return &(*iter) ;
   }

   return nullptr;
}

ServerListEntry* ServerList::findByLocalIDandInvalidMasterID( uint32_t localID )
{
   for( tContainerType::iterator iter = mEntries.begin(); iter != mEntries.end(); ++iter )
   {
      if( (localID == iter->partyID.s.localID ) && (iter->masterID.s.localID == (uint32_t)-1))
         return &(*iter) ;
   }

   return nullptr;
}



ServerListEntry* ServerList::find( int32_t id )
{
   for( tContainerType::iterator iter = mEntries.begin(); iter != mEntries.end(); ++iter )
   {
      if( id == iter->id )
         return &(*iter) ;
   }

   return nullptr;
}


void ServerList::remove( const SPartyID& serverID )
{
   for( tContainerType::iterator iter = mEntries.begin(); iter != mEntries.end(); ++iter )
   {
      if( iter->partyID.globalID == serverID.globalID )
      {
         (void)mEntries.erase( iter );
         break;
      }
   }
}


void ServerList::clearMasterIDs()
{
   for( tContainerType::iterator iter = mEntries.begin(); iter != mEntries.end(); iter++ )
   {
      iter->masterID = 0 ;
   }
}


void ServerList::dumpStats( std::ostringstream& ostream ) const
{   
   char pidBuf[32];
   char processName[24];
   char idBuf[32];
   char ifName[NAME_MAX+2];

   memset(ifName, 0, sizeof(ifName));
   
   ostream << "          pid/ip:port  Process                  ServerId        Interface\n" ;               
   
   for( tContainerType::const_iterator iter = mEntries.begin(); iter != mEntries.end(); ++iter )
   {
      strcpy(processName, "<unknown>");
      if (SB_LOCAL_NODE_ADDRESS == iter->nid)
      {
         (void)GetProcessName( iter->pid, processName, sizeof(processName) );
      }

      sprintf(idBuf, "<%d.%d>", iter->partyID.s.extendedID, iter->partyID.s.localID);
      
      if (endsWith(iter->ifDescription.name, "_tcp"))
      {
         sprintf( pidBuf, "%d.%d.%d.%d:%d", (iter->pid>>24) & 0xFF, (iter->pid>>16) & 0xFF, (iter->pid>>8) & 0xFF, iter->pid & 0xFF, iter->chid );
         
         iter->ifDescription.name.copy(ifName, iter->ifDescription.name.size()-4);
         
         ServerList* that = const_cast<ServerList*>(this);
         ServerListEntry* nonTcp = that->find(iter->ifDescription.name.substr(0, iter->ifDescription.name.size()-4).c_str());
         if (nonTcp)
            (void)GetProcessName( nonTcp->pid, processName, sizeof(processName) );         
      }
      else
      {
         sprintf( pidBuf, "%d", iter->pid );
         iter->ifDescription.name.copy(ifName, iter->ifDescription.name.size());         
      }

      ostream << std::right << std::setw(21) << pidBuf << "  "
              << std::left << std::setw(24) << processName << " "
              << std::setw(15) << idBuf << " "
              << ifName << " "
              << iter->ifDescription.majorVersion << "."
              << iter->ifDescription.minorVersion << ' ';              

      if( iter->local )
         ostream << "[LOCAL]";

      if (endsWith(iter->ifDescription.name, "_tcp"))
         ostream << "[TCP]";
         
      ostream << "\n" ;
   }
}

