/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "ClientList.hpp"

#include <cstdio>
#include <sstream>
#include <iomanip>

#include "Servicebroker.hpp"
#include "ClientSpecificData.hpp"
#include "util.hpp"
#include "SocketConnectionContext.hpp"


void ClientList::remove( const SPartyID& clientId )
{
  for(std::vector<ClientListEntry>::iterator iter = mEntries.begin(); iter != mEntries.end(); ++iter)
  {
    if( iter->mClientId.globalID == clientId.globalID )
    {
      mEntries.erase(iter);
      break;
    }
  }
}


void ClientList::removeServer( const SPartyID& serverId )
{
  for(std::vector<ClientListEntry>::iterator iter = mEntries.begin(); iter != mEntries.end();)
  {
    if( iter->mServerId.globalID == serverId.globalID )
    {
      Log::message(3, "Removing client <%d.%d> due to server disconnect of <%u.%u>",
               iter->mClientId.s.extendedID, iter->mClientId.s.localID,
               serverId.s.extendedID, serverId.s.localID);
      iter = mEntries.erase(iter);         
    }
    else
      ++iter;
  }
}


void ClientList::removeNotLocal(uint32_t extendedId)
{
  for(std::vector<ClientListEntry>::iterator iter = mEntries.begin(); iter != mEntries.end();)
  {
    if( iter->mServerId.s.extendedID != extendedId )
    {
      Log::message(3, "Removing client <%u.%u>", iter->mClientId.s.extendedID,
               iter->mClientId.s.localID);
      iter = mEntries.erase(iter);         
    }
    else
      ++iter;
  }
}


void ClientList::dumpStats( std::ostringstream& ostream ) const
{
  char c1[32], c2[32] ;
  char processName[24];   

  ostream << "     pid  Process                  ClientId      ServerId        Interface\n" ;

  for(std::vector<ClientListEntry>::const_iterator iter = mEntries.begin(); iter != mEntries.end(); ++iter)
  {
    const ClientListEntry& entry = *iter ;

    strcpy(processName, "<unknown>");   
    if( SB_LOCAL_NODE_ADDRESS == entry.mCtx->getNodeAddress() )
    {
      (void)GetProcessName( entry.mCtx->getPid(), processName, sizeof(processName) );
    }

    (void)snprintf( c1, sizeof(c1)-1, "<%d.%d>", entry.mClientId.s.extendedID, entry.mClientId.s.localID);
    (void)snprintf( c2, sizeof(c2)-1, "<%d.%d>", entry.mServerId.s.extendedID, entry.mServerId.s.localID );

    ostream << std::right << std::setw(8) << entry.mCtx->getPid() << "  "
          << std::left << std::setw(24) << processName << " "
          << std::setw(14) << c1
          << std::setw(14) << c2 ;

    InterfaceDescription ifDescr;

    if (Servicebroker::getInstance().findServerInterface(entry.mServerId, ifDescr))
    {
      char ifName[NAME_MAX+2];
      memset(ifName, 0, sizeof(ifName));         

      if (endsWith(ifDescr.name, "_tcp"))
      {         
        ifDescr.name.copy(ifName, ifDescr.name.size()-4);
      }
      else 
        ifDescr.name.copy(ifName, ifDescr.name.size());            

      ostream << "  " << ifName << " "
            << ifDescr.majorVersion << "."
            << ifDescr.minorVersion << ' ';

      if (endsWith(ifDescr.name, "_tcp"))
        ostream << "[TCP]";
    }

    ostream << "\n" ;
  }
}

