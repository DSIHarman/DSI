/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iomanip>

#include "Notification.hpp"
#include "Servicebroker.hpp"
#include "IDList.hpp"
#include "Log.hpp"
#include "ClientSpecificData.hpp"
#include "config.h"
#include "util.hpp"
#include "PulseChannelManager.hpp"

#include <iostream>


namespace /*anonymous*/
{
   notificationid_t sNextNotificationID = 0 ;
}

unsigned int Notification::totalCounter(0);
unsigned int Notification::currentCounter(0);


Notification::Notification()
 : notificationID(++sNextNotificationID)
 , masterNotificationID(INVALID_NOTIFICATION_ID)
 , poolID(0)
 , host(0)
 , local(false)
 , uid(SB_UNKNOWN_USER_ID)
 , active(true)
 , regExpr(0)
 , coid(-1)
 , prio(0)
 , nid(0)
{
   totalCounter++;
   currentCounter++;
   partyID = 0 ;
   memset(&pulse, 0, sizeof(pulse));
   Log::message(3, "NOTIFICATION_ID %u", notificationID);
}


Notification::~Notification()
{
   if (regExpr)
      delete regExpr;

   if (coid != -1)
      PulseChannelManager::getInstance().detach(coid);

   // remove entry from hosting ID list
   if (host)
      host->removeNotification( notificationID );

   if (poolID != 0)
   {
      //for notifications with proxies the proxy will send on the master a notification when proxy not needed anymore
      const int poolPos = Servicebroker::getInstance().mMasterNotificationPool.find(poolID);
      if (poolPos != -1)
      {
         Servicebroker::getInstance().mMasterNotificationPool[poolPos].detach(*this);
      }
   }
   else if (masterNotificationID != INVALID_NOTIFICATION_ID)
   {
      //for notifications without proxy send a clear notification on master
      Servicebroker::getInstance().createClearNotificationJob(notificationID);
   }
   else
   {
      //nothing to clean-up on master
   }

   currentCounter--;
}


Notification::Notification( const Notification& rhs )
 : notificationID( rhs.notificationID )
 , masterNotificationID( rhs.masterNotificationID )
 , poolID(rhs.poolID)
 , host( rhs.host )       
 , partyID( rhs.partyID )
 , ifDescription( rhs.ifDescription )
 , local( rhs.local )
 , uid( rhs.uid )
 , active( rhs.active )
 , regExpr( rhs.regExpr )   
 , pulse( rhs.pulse )
 , coid(rhs.coid)
 , prio( rhs.prio )
 , nid( rhs.nid )
{

   // implement move semantics
   Notification* that = const_cast<Notification*>( &rhs ) ;
   that->host = 0 ;
   that->regExpr = 0 ;
   that->coid = -1 ;
   
   // make sure there is no master interaction on the moved object!
   // FIXME maybe it would be better to use an attach/detach pair to make sure we don't break the code
   that->poolID = 0; 
   that->masterNotificationID = INVALID_NOTIFICATION_ID;
   
   memset(&that->pulse, 0, sizeof(that->pulse));
   
   currentCounter++;
}


void Notification::writeMessage(Log::eSBLogType type, const char* message) const
{
   char buf[128];

   if (nid != SB_LOCAL_NODE_ADDRESS)
   {
      // is TCP socket -> definitely other servicebroker
      struct in_addr addr;

      addr.s_addr = htonl(nid);
      char nid_buf[16];
      strcpy(nid_buf, inet_ntoa(addr));

      addr.s_addr = pulse.pid;
      char pid_buf[16];
      strcpy(pid_buf, inet_ntoa(addr));

      sprintf(buf, "IP:%s, IP-TRANSMITTED:%s, PORT:%d, VALUE: %d", nid_buf, pid_buf, htons(pulse.chid), pulse.value );
   }
   else
   {
      sprintf(buf, "NID:%d, PID:%d, CHID:%d, CODE: %d VALUE: %d", nid, pulse.pid, pulse.chid, pulse.code, pulse.value );
   }

   if (type == Log::SBLOG_MESSAGE)
   {
      Log::message(3, "%s%s", message, buf);
   }
   else
      Log::error("%s%s", message, buf);
}


bool Notification::doConnect()
{
   bool retval = false;

   coid = PulseChannelManager::getInstance().attach(nid, pulse.pid, pulse.chid);

   if (coid < 0)
   {
      writeMessage(Log::SBLOG_ERROR, "Error connecting to ");
      ::memset(&pulse, 0, sizeof(pulse));
   }
   else
   {
      writeMessage(Log::SBLOG_MESSAGE, "Notification connected. ");
      retval = true;
   }

   return retval;
}


bool Notification::connect( const SocketConnectionContext& ctx, SFNDPulseInfo &thePulse )
{
   bool retval = false;
   
   pulse = thePulse;

   // set correct node id on notification
   nid = ctx.getNodeAddress();

   if (!ctx.isSlave())
   {
      retval = true;
   }
   else
      retval = doConnect();   // other masters always connect immediately so we will reuse the socket

   return retval ;
}


void Notification::send()
{
   Log::message( 3, "+SENDING NOTIFICATION -%d- CODE:%d VALUE:%d", notificationID, pulse.code, pulse.value );

   // maybe not yet connected?
   if (coid < 0)
      (void)doConnect();

   if (coid >= 0)
      PulseChannelManager::getInstance().sendPulse(coid, notificationID, pulse.code, pulse.value);
}


bool Notification::setRegExp( const char* regexp )
{
   bool retval = false ;

   if (regExpr)
      delete regExpr;

   regExpr = new RegExp();

   if( regExpr )
   {
      int err = regExpr->compile( regexp, REG_EXTENDED | REG_NOSUB );
      if( 0 != err )
      {
         char buffer[128];
         (void)regExpr->error( err, buffer, sizeof(buffer)-1);
         Log::error( "RegExp Error: %s /%s/", buffer, regexp );
      }
      else
      {
         retval = true ;
      }
   }
   return retval ;
}


void Notification::dumpStats( std::ostringstream& ostream ) const
{
   char pidBuf[32];
   char processName[16];
   char ifName[NAME_MAX+2];   
   bool isTcp = false;

   memset(ifName, 0, sizeof(ifName));   
   
   if( SB_LOCAL_NODE_ADDRESS == nid )
   {      
      (void)GetProcessName( pulse.pid, processName, sizeof(processName) );
   }
   else
   {
      struct in_addr addr;
      addr.s_addr = htonl(nid);
      sprintf(processName, "%s", inet_ntoa(addr));
   }   
   
   if( pulse.pid & 0xFF000000 )
   {   
      sprintf( pidBuf, "%d.%d.%d.%d", (pulse.pid>>24) & 0xFF, (pulse.pid>>16) & 0xFF, (pulse.pid>>8) & 0xFF, pulse.pid & 0xFF );
   }
   else
   {
      sprintf( pidBuf, "%d", pulse.pid );
   }

   ostream << std::right << std::setw(8);
   
   if( partyID == 0 )
   {
      if (SB_LOCAL_NODE_ADDRESS != nid) 
      {
         ostream << "sbkr at";
      }
      else  
         ostream << pidBuf;
            
      
      if (endsWith(ifDescription.name, "_tcp"))         
      {
         ifDescription.name.copy(ifName, ifDescription.name.size() - 4);      
         isTcp = true;
      }
      else
         ifDescription.name.copy(ifName, ifDescription.name.size());      
      
      ostream << std::left << "  " << std::setw(24) << processName
              << " -" << notificationID << "- "
              << ifName << " " 
              << ifDescription.majorVersion
              << "." << ifDescription.minorVersion << ' ';      
   }
   else
   {      
      if (SB_LOCAL_NODE_ADDRESS != nid) 
      {
         ostream << "sbkr at";
      }
      else      
         ostream << pidBuf;
      
      ostream << std::left << "  " << std::setw(24) << processName
              << " -" << notificationID << "- "
              << "<" << partyID.s.extendedID << "." << partyID.s.localID << ">";

      InterfaceDescription ifDescr;
      if (Servicebroker::getInstance().findServerInterface(partyID, ifDescr))
      {
         if (endsWith(ifDescr.name, "_tcp"))         
         {
            isTcp = true;
            ifDescr.name.copy(ifName, ifDescr.name.size() - 4);                  
         }
         else
            ifDescription.name.copy(ifName, ifDescr.name.size());
         
         ostream << "  " << ifName << " "
                 << ifDescr.majorVersion << "."
                 << ifDescr.minorVersion << ' ';
      }
   }

   if( local )
   {
      ostream << "[LOCAL]" ;
   }

   if( !active )
   {
      ostream << "[INACTIVE]" ;
   }
   
   if (isTcp)
   {
      ostream << "[TCP]";
   }

   ostream << "\n" ;
}


bool Notification::matchRegExp( const std::string& string )
{
   return regExpr && (0 == regExpr->execute( string, 0 ));
}

