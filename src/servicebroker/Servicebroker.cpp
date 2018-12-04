/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "Servicebroker.hpp"
#include "Log.hpp"
#include "MessageContext.hpp"
#include "ConnectionContext.hpp"
#include "RegExp.hpp"
#include "ClientSpecificData.hpp"
#include "JobQueue.hpp"
#include "SignallingAddress.hpp"
#include "config.h"

#include "SocketConnectionContext.hpp"

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include <cctype>
#include <cmath>

#define MAX_MULTIPLE_COUNT 32


const int32_t Servicebroker::MAJOR_VERSION = DSI_SERVICEBROKER_VERSION_MAJOR ;
const int32_t Servicebroker::MINOR_VERSION = DSI_SERVICEBROKER_VERSION_MINOR ;
const int32_t Servicebroker::BUILD_VERSION = 54 ;
const char* Servicebroker::BUILD_TIME = __DATE__ ", " __TIME__ ;


// -------------------------------------------------------------------------------------------


namespace /*anonymous*/
{


inline
bool isTcpService(const char* ifName)
{
   if (ifName)
   {
      size_t len = strlen(ifName);
      return len >  5 && !strncmp(ifName + len - 4, "_tcp", 4);  
   }
   else
   {
      return false;
   }
}


struct NodeAddressTranslator
{
   inline
   NodeAddressTranslator(const SocketConnectionContext& ctx)
    : ctx_(ctx)
   {
      // NOOP
   }

   inline
   uint32_t translateRemote(const char* ifName, uint32_t pid) const
   {
      return ctx_.isSlave() && pid == SB_LOCAL_IP_ADDRESS && isTcpService(ifName) ? ctx_.getNodeAddress() : pid;
   }

   inline
   uint32_t translateLocal(const char* ifName, uint32_t pid) const
   {
      return ctx_.isSlave() && pid == SB_LOCAL_IP_ADDRESS && isTcpService(ifName) ? ctx_.getLocalAddress() : pid;
   }

   const SocketConnectionContext& ctx_;
};


// object generator function
inline
NodeAddressTranslator IPv4NodeAddressTranslator(const SocketConnectionContext& ctx)
{
   return NodeAddressTranslator(ctx);
}

}   // namespace anonymous


// ---------------------------------------------------------------------------------------


/*static*/
Servicebroker& Servicebroker::getInstance()
{
   static Servicebroker instance;
   return instance;
}


Servicebroker::Servicebroker()
   : mNextClientID( 100001U )
   , mNextServerID( 500001U )
   , mNextPendingRemoteServerRequestID(1U)
   , mExtendedID(0)
   , mUseServerCache(false)
   , mAsyncMode(false)
   , mAster(0)
{
   // NOOP
}


Servicebroker::~Servicebroker()
{
   mAster = 0;
}


void Servicebroker::init(MasterAdapter* theMaster, NodeIdTranslator translator, const char* configfile,
                         int the_id, bool serverCache, bool asyncMode)
{
   mAster = theMaster;
   mTranslateNodeId = translator;
   mConfig.load( configfile ? configfile : "" );
   mConfig.setTreeMode(the_id > 0);
   mAsyncMode = (the_id > 0 && asyncMode) ? true : false;
   mUseServerCache = serverCache;
   mExtendedID = (the_id > 0) ? the_id : (isMaster() ? 1000U : 0U) ;
}


void Servicebroker::setSignallingAddress(const SignallingAddress& addr)
{
   mSigAddr = addr;
}


bool Servicebroker::check( const SFNDInterfaceVersion &version )
{
   bool versionOK = version.majorVersion == DSI_SERVICEBROKER_VERSION_MAJOR
      && version.minorVersion <= DSI_SERVICEBROKER_VERSION_MINOR ;

   if( !versionOK )
   {
      Log::error( "Bad serviceborker DSI version %d.%d (expected %d.%d)"
                  , version.majorVersion, version.minorVersion
                  , DSI_SERVICEBROKER_VERSION_MAJOR, DSI_SERVICEBROKER_VERSION_MINOR );
   }

   return versionOK ;
}


bool Servicebroker::check( const SFNDInterfaceDescription &ifDescription )
{
   /* assume a valid name */
   bool result = true ;

   const char* ptr = &ifDescription.name[0] ;
   const char* end = &ifDescription.name[sizeof(ifDescription.name)];
   while( (ptr < end) && *ptr )
   {
      /* must be a 'readable' character */
      if( iscntrl( *ptr ) || ((*ptr & 0x080) != 0))
      {
         result = false ;
         break ;
      }

      /* continue with next position */
      ++ptr;
   }


   return result
      && '\0' == *ptr   /* last character must be '\0', otherwise the name is not valid */
      && (ifDescription.version.majorVersion != 0 || ifDescription.version.minorVersion != 0 /* and the version must be valid */) ;
}


// -------------------------------------------------------------------------------------------------------------


// FIXME rename pid and chid here to something transport independent
bool Servicebroker::addInterface( ClientSpecificData &ocb, const InterfaceDescription& ifDescription, const SocketConnectionContext& ctx, int32_t pid, int32_t chid, bool isLocal, int32_t grpid, SPartyID& serverID )
{
   bool rc = false;
   
   /* search for the entry */
   if(mConnectedServers.find( ifDescription.name.c_str() ))
   {
      /* the interface name already exists -> error */
      Log::error( " Interface %s already exists", ifDescription.name.c_str() );
   }
   else
   {
      /* add a new server */
      ServerListEntry newEntry ;      
      newEntry.ifDescription = ifDescription;
      newEntry.partyID.s.extendedID = mExtendedID ;
      newEntry.partyID.s.localID = mNextServerID++ ;

      // FIXME automatic node address retrieval from the connection context is buggy since if the servicebrokers
      //       are connected via TCP/IP the master will see a different service where node-id is not the
      //       QNX Q-Net node address but the IP address of the other servicebroker. Therefore, services can
      //       only connect via DSI over TCP/IP, but not via message passing.
      //       -> the node address should be given via the devctl and should be hand over to the next node's sb.
      newEntry.nid = ctx.getNodeAddress();

      newEntry.pid = pid;
      newEntry.chid = chid;
      newEntry.grpid = grpid ;
      newEntry.local = isLocal ;

      /* insert interface */
      mConnectedServers.add( newEntry );

      /* insert server ID */
      ocb.addServer( newEntry.partyID ) ;

      /* send the result back */
      serverID = newEntry.partyID ;

      /* notify clients waiting for this interface */
      mServerConnectNotifications.trigger( newEntry.ifDescription, newEntry.grpid );

      /* triger serverlist change notifications */
      Log::message( 1,"mServerListChangeNotifications.triggerAll %s",newEntry.ifDescription.name.c_str());
      mServerListChangeNotifications.triggerAll( newEntry.ifDescription );

      /* success */
      rc = true;
   }

   return rc;
}


void Servicebroker::handleRegisterInterface( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceRegisterArg &arg )
{   
   bool isLocal = mConfig.isLocalService( arg.i.ifDescription.name );

   Log::message( 1, "*[%d] %s %s %d.%d%s"
                 , msg.context().getId()
                 , GetDCmdString(DCMD_FND_REGISTER_INTERFACE)
                 , arg.i.ifDescription.name
                 , arg.i.ifDescription.version.majorVersion
                 , arg.i.ifDescription.version.minorVersion
                 , isLocal ? " [LOCAL]" : "" );

   /*
    * verify foundation version, interface name and version,
    * and interface name, implementation version, and server channel
    */
   if(check( arg.i.ifDescription ))
   {
      /* first check if the call comes from the local node */
      if( !msg.context().isLocal() )
      {
         /* WRONG CALL */
         Log::error( "%s not called from local node!", GetDCmdString(DCMD_FND_REGISTER_INTERFACE) );         
         
         msg.prepareResponse(FNDBadArgument);
      }
      else
      {
         bool success = addInterface( ocb, arg.i.ifDescription, msg.context(), arg.i.pid, arg.i.chid, isLocal, -1, arg.o.serverID );

         /* trigger master registration */
         registerInterfacesMaster();

         if (success)
         {
            Log::message( 1, " SERVER_ID <%d.%d>", arg.o.serverID.s.extendedID, arg.o.serverID.s.localID );

            /* send the result back */
            msg.prepareResponse(FNDOK, &arg.o, sizeof(arg.o));   
         }
         else
            msg.prepareResponse(FNDInterfaceAlreadyRegistered);
      }
   }
   else
      msg.prepareResponse(FNDBadArgument);  
}


void Servicebroker::handleRegisterInterfaceEx( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceRegisterExArg &arg )
{
   Log::message( 1, "*[%d] %s", msg.context().getId(), GetDCmdString(DCMD_FND_REGISTER_INTERFACE_EX));

   SPartyID serverID ;
   SFNDInterfaceDescription ifDescription ;
   bool isLocal ;

   for( int32_t idx=0; idx<arg.i.ifCount; idx++ )
   {
      // reading the next interface description from the client
      if( -1 == msg.readAncillaryData( &ifDescription, sizeof(ifDescription)))
      {
         msg.prepareResponse(FNDBadArgument);   
         return;
      }

      // is it a local one?
      isLocal = mConfig.isLocalService( ifDescription.name );

      // add the interface
      if( !addInterface( ocb, ifDescription, msg.context(), arg.i.pid, arg.i.chid, isLocal, -1, serverID ) )
      {
         serverID.globalID = (uint64_t)-1 ;
      }

      Log::message( 1, " ADDING INTERFACE %s %d.%d: <%d.%d>%s"
                    , ifDescription.name
                    , ifDescription.version.majorVersion
                    , ifDescription.version.minorVersion
                    , serverID.s.extendedID, serverID.s.localID
                    , isLocal ? " [LOCAL]" : "" );

      // write the server ID to the client
      msg.writeAncillaryData( &serverID, sizeof(serverID));
   }

   /* trigger master registration */
   registerInterfacesMaster();

   /* send back the answer to the client */
   msg.prepareResponse(FNDOK);   
}


void Servicebroker::handleRegisterInterfaceGroupID( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceRegisterGroupIDArg &arg )
{
   bool success = false;
   bool isLocal = mConfig.isLocalService( arg.i.ifDescription.name );

   Log::message( 1, "*[%d] %s %s %d.%d - Group %d%s"
                 , msg.context().getId()
                 , GetDCmdString(DCMD_FND_REGISTER_INTERFACE_GROUPID)
                 , arg.i.ifDescription.name
                 , arg.i.ifDescription.version.majorVersion
                 , arg.i.ifDescription.version.minorVersion
                 , arg.i.grpid
                 , isLocal ? " [LOCAL]" : "" );

   /*
    * verify foundation version, interface name and version,
    * and interface name, implementation version, and server channel
    */
   if(check( arg.i.ifDescription ))
   {
      /* fist check if the call comes from the local node */
      if( !msg.context().isLocal() )
      {
         /* WRONG CALL */
         Log::error( "%s not called from local node!", GetDCmdString(DCMD_FND_REGISTER_INTERFACE) );
      }
      else
      {
         success = addInterface( ocb, arg.i.ifDescription, msg.context(), arg.i.pid, arg.i.chid, isLocal, arg.i.grpid, arg.o.serverID );

         /* trigger master registration */
         registerInterfacesMaster();

         if( success )
         {
            Log::message( 1, " SERVER_ID <%d.%d>", arg.o.serverID.s.extendedID, arg.o.serverID.s.localID );

            /* send the result back */
            msg.prepareResponse(FNDOK, &arg.o, sizeof(arg.o));  
         }
      }
   }

   if (!success)
      msg.prepareResponse(FNDBadArgument);  
}


void Servicebroker::handleUnregisterInterface( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceUnregisterArg &arg )
{
   bool success = false;
   Log::message( 1, "*[%d] %s: <%d.%d>"
                 , msg.context().getId()
                 , GetDCmdString(DCMD_FND_UNREGISTER_INTERFACE)
                 , arg.i.serverID.s.extendedID
                 , arg.i.serverID.s.localID);

   /* test for found server ID */
   /* has to be changed */
   if(ocb.serverExists( arg.i.serverID ))
   {
      /* remove server ID from list of server IDs of this ocb */
      ocb.removeServer( arg.i.serverID );

      ServerListEntry *entry = mConnectedServers.find( arg.i.serverID );

      /* found ? */
      if( entry )
      {
         /* OK */
         success = true;

         Log::message( 1, " INTERFACE: %s %d.%d%s"
                       , entry->ifDescription.name.c_str()
                       , entry->ifDescription.majorVersion
                       , entry->ifDescription.minorVersion
                       , entry->local ? " [LOCAL]" : "" );

         /* send notifications */
         mServerDisconnectNotifications.trigger( arg.i.serverID );

         /* if there is a master involved we need to unregister the interface at the master */
         unregisterInterfaceMaster( *entry );

         /* remove entry */
         mConnectedServers.remove( arg.i.serverID );

         //remove all clients connected to the server
         mAttachedClients.removeServer(arg.i.serverID);


         /* send the result back */
         msg.prepareResponse(FNDOK);   

         /* triger serverlist change notifications */
         mServerListChangeNotifications.triggerAll();
      }
   }

   if (!success)
      msg.prepareResponse(FNDBadArgument);   
}


void Servicebroker::handleAttachInterface( SocketMessageContext &msg, ClientSpecificData &ocb,
                                           SFNDInterfaceAttachArg &arg)
{
   SBStatus result = FNDOK;
   bool isLocal = mConfig.isLocalService( arg.i.ifDescription.name );

   Log::message( 1, "*[%d] %s %s %d.%d%s"
                 , msg.context().getId()
                 , GetDCmdString(DCMD_FND_ATTACH_INTERFACE)
                 , arg.i.ifDescription.name
                 , arg.i.ifDescription.version.majorVersion
                 , arg.i.ifDescription.version.minorVersion
                 , isLocal ? " [LOCAL]" : "" );

   if( check( arg.i.ifDescription ))
   {
      result = FNDUnknownInterface ;
      ServerListEntry* entry = mConnectedServers.find( arg.i.ifDescription );
#ifdef USE_SERVER_CACHE
      int32_t cachePos(-1);
#endif

      if( entry )
      {
         result = handleAttachLocalInterface(msg, ocb, arg, entry);
      }
#ifdef USE_SERVER_CACHE
      else if (mUseServerCache && -1 != (cachePos = mRemoteServersCache.find(arg.i.ifDescription)))
      {
         //we have information about the server in the cache
         const SServerInfo &serverInfo = mRemoteServersCache[cachePos].getValue();

         arg.o.ifVersion = arg.i.ifDescription.version;
         arg.o.channel = serverInfo.channel;
         arg.o.serverID = serverInfo.serverID;
         addClient(arg.o.clientID, msg.context(), serverInfo.serverID);

         /* insert client ID */
         result = FNDOK;
      }
#endif
      else if( !isLocal && mConfig.forwardService(arg.i.ifDescription.name) && isMasterConnected() )
      {
         //not in cache, we check now if it exists on the master.
         Job *job = Job::create(msg, mNextPendingRemoteServerRequestID++, DCMD_FND_ATTACH_INTERFACE, &arg,
                                sizeof(arg.i), sizeof(arg.o));
#ifdef USE_SERVER_CACHE
         if (mUseServerCache)
         {
            mPendingRemoteServersRequests.set(job->cookie, arg.i.ifDescription);
         }
#endif
         Log::message(1, " Forwarding %s to master JobID = %u", GetDCmdString(DCMD_FND_ATTACH_INTERFACE),
                      static_cast<uint32_t>(job->cookie));

         master().eval(job);
         msg.deferResponse();
      }
   }
   else
   {
      result = FNDBadArgument;
   }

   if (msg.responseState() != MessageContext::State_DEFERRED)
   {
      if( FNDOK == result )
      {
         /* send the result back */
         msg.prepareResponse(result, &arg, sizeof(arg.o));
      }
      else
      {
         msg.prepareResponse(result);
      }
   }
}


SBStatus Servicebroker::handleAttachLocalInterface(SocketMessageContext &msg, ClientSpecificData &/*ocb*/,
                                                   SFNDInterfaceAttachArg &arg, ServerListEntry* entry)
{
   SBStatus result = FNDOK;

   assert(entry != 0);
   if( SB_UNKNOWN_GROUP_ID != entry->grpid )
   {
      result = FNDAccessDenied ;

      if( !msg.context().isLocal() )
      {
         Log::error( " Access restricted interfaces can only be attached locally" );
      }
      else if( checkAccessPrivileges( msg.context(), entry ))
      {
         result = FNDOK ;
      }
   }

   if( result == FNDOK )
   {
      arg.o.ifVersion.majorVersion = entry->ifDescription.majorVersion ;
      arg.o.ifVersion.minorVersion = entry->ifDescription.minorVersion ;
      arg.o.serverID = entry->partyID ;

      addClient(arg.o.clientID, msg.context(), entry->partyID);
      arg.o.channel.pid = IPv4NodeAddressTranslator(msg.context()).translateLocal(entry->ifDescription.name.c_str(), entry->pid);
      arg.o.channel.chid = entry->chid ;

      assert(mTranslateNodeId);
      arg.o.channel.nid = mTranslateNodeId(msg.context().getNodeAddress(), entry->nid);

      Log::message( 1, " SUCCESS - NID: %u, PID: %d, CHID: %d, CLIENT_ID: <%u.%u>, SERVER_ID: <%u.%u>"
                    , arg.o.channel.nid
                    , arg.o.channel.pid
                    , arg.o.channel.chid
                    , arg.o.clientID.s.extendedID
                    , arg.o.clientID.s.localID
                    , arg.o.serverID.s.extendedID
                    , arg.o.serverID.s.localID );
   }
   else
   {
      Log::error( " Attaching to interface failed" );
   }

   return result;
}


void Servicebroker::handleAttachInterface(SocketMessageContext &msg, ClientSpecificData &ocb,
                                          SFNDInterfaceAttachExtendedArg &arg)
{
   const bool isLocal = mConfig.isLocalService(arg.i.ifDescription.name);

   Log::message(1, "*[%d] %s %s %u.%u%s"
                , msg.context().getId()
                , GetDCmdString(DCMD_FND_ATTACH_INTERFACE_EXTENDED)
                , arg.i.ifDescription.name
                , arg.i.ifDescription.version.majorVersion
                , arg.i.ifDescription.version.minorVersion
                , isLocal ? " [LOCAL]" : "");   

   SBStatus result(FNDInternalError);
   union SFNDNotifyServerAvailableArg serverAvailableArg;
   union SFNDNotifyServerDisconnectArg serverDisconnectArg;   

   //pointers used to transfer the data HOMOGENEOUSLY between the tAttachInterfaceExtendedCommonArg argument and other
   //data structures. In this way the function is keept short and the focus is more on the logic than on data handling.
   struct SFNDInterfaceVersion *sbVersion = &arg.i.sbVersion;
   struct SFNDInterfaceDescription *ifDescription = &arg.i.ifDescription;   
   
   struct SConnectionInfo *connInfo = &arg.o.connInfo;
   notificationid_t *outNotificationID = &arg.o.notificationID;      
   notificationid_t *inMainNotificationID = &serverDisconnectArg.o.notificationID;   

   // prepare pointers and, server connect/disconnect notifications before changing the arg union
   union SFNDNotifyServerDisconnectArg& commonArg = serverDisconnectArg;
   
   serverAvailableArg.i.sbVersion = *sbVersion;
   serverAvailableArg.i.ifDescription = *ifDescription;
   serverAvailableArg.i.pulse = arg.i.pulse;

   if (!check(*ifDescription))
   {
      result = FNDBadArgument;
   }
   else
   {
      result = FNDUnknownInterface ;
      ServerListEntry* entry = mConnectedServers.find(*ifDescription);
#ifdef USE_SERVER_CACHE
      int32_t pos;
#endif
      if (entry)
      {
         SFNDInterfaceAttachArg ifAttachArg;

         serverDisconnectArg.i.serverID = entry->partyID;
         ifAttachArg.i.sbVersion = *sbVersion;
         ifAttachArg.i.ifDescription = *ifDescription;
         result = handleAttachLocalInterface(msg, ocb, ifAttachArg, entry);
         *connInfo = ifAttachArg.o;
         if (FNDOK == result)
         {
            //if server on local SB set server disconnect notification
            result = handleNotifyServerDisconnectNoResponse(msg, ocb, commonArg);
            *outNotificationID = *inMainNotificationID;            
         }
      }
#ifdef USE_SERVER_CACHE
      else if (mUseServerCache && -1 != (pos = mRemoteServersCache.find(*ifDescription)))
      {
         //found remote server connection information in the remote server cache
         const SServerInfo &remoteServerInfo = mRemoteServersCache[pos].getValue();

         dumpCache();   // FIXME ???
         Log::message(3, "Cached server info for %s %d.%d found : <%d.%d>", ifDescription->name,
                      ifDescription->version.majorVersion, ifDescription->version.minorVersion,
                      remoteServerInfo.serverID.s.extendedID, remoteServerInfo.serverID.s.localID);
         serverDisconnectArg.i.serverID = remoteServerInfo.serverID;

         result = handleNotifyServerDisconnectNoResponse(msg, ocb, commonArg);
         if (FNDOK == result)
         {
            addClient(connInfo->clientID, msg.context(), remoteServerInfo.serverID);
            connInfo->serverID = remoteServerInfo.serverID;
            connInfo->channel = remoteServerInfo.channel;
            *outNotificationID = *inMainNotificationID;            
         }
      }
#endif
      else if (isLocal || (!mConfig.forwardService(ifDescription->name)) || !isMasterConnected())
      {
         result = handleNotifyServerAvailableNoResponse(msg, ocb, serverAvailableArg);
         *outNotificationID = serverAvailableArg.o.notificationID;         
         if (FNDOK == result)
         {
            //interface not known at this moment, connect notification set
            result = FNDUnknownInterface;            
         }
      }
      else      
         result = forwardAttachExtendedToMaster(msg, ocb, arg);      
   }

   if (msg.responseState() != MessageContext::State_DEFERRED)
   {      
      if (FNDUnknownInterface == result || FNDOK == result)
      {
         msg.prepareResponse(result, &arg, sizeof(arg.o)); 
      }
      else
         msg.prepareResponse(result);         
   }
}


SBStatus Servicebroker::forwardAttachExtendedToMaster(SocketMessageContext &msg, ClientSpecificData &ocb,
                                                      SFNDInterfaceAttachExtendedArg &arg)
{
   SBStatus result(FNDOK);
   bool poolFound(true);
   Job *job(0);
   struct SFNDInterfaceDescription *ifDescription(0);      
   
   notificationid_t *outNotificationID(0);      
   
   ifDescription = &arg.i.ifDescription;   
   outNotificationID = &arg.o.notificationID;   

   const bool isLocal = mConfig.isLocalService(arg.i.ifDescription.name);
   int32_t pos(mMasterNotificationPool.find(arg.i.ifDescription, true));

   if (-1 == pos)
   {
      pos = mMasterNotificationPool.add(*ifDescription, true);
      poolFound = false;
   }
   
   notificationid_t notificationID = mServerConnectNotifications.add(&ocb
                                                        , arg.i.ifDescription
                                                        , msg.context()
                                                        , arg.i.pulse
                                                        , isLocal
                                                        , msg.context().getEffectiveUid());   

   if (notificationID == Notification::INVALID_NOTIFICATION_ID)
   {
      result = FNDInvalidNotificationID;
   }
   else
   {
      //not optimal to search after add() the notification again
      Notification* theAttachNotification = mServerConnectNotifications.find(notificationID);
      if (!theAttachNotification)
      {
         result = FNDInternalError;
      }
      else
      {
         ocb.addNotification(notificationID);
         mMasterNotificationPool[pos].attach(*theAttachNotification);
      }      
   }

   if (FNDOK == result)
   {
      result = FNDUnknownInterface;
      if (!poolFound || mMasterNotificationPool[pos].getState() == NotificationPoolEntry::STATE_CONNECTED)
      {
         const int dcmd = DCMD_FND_ATTACH_INTERFACE_EXTENDED;
         Log::message(1, " Forwarding %s to master JobID = %u", GetDCmdString(dcmd), notificationID);

         // forward job to the master, but change first the pulse information
         arg.i.pulse.pid = mSigAddr.internal.major;  
         arg.i.pulse.chid = mSigAddr.internal.minor; 
         arg.i.pulse.code = PULSE_MASTER_ATTACH_EXTENDED;
         arg.i.pulse.value = mMasterNotificationPool[pos].getPoolID();
         tNotificationsGroup notificationsGroup;
         uint32_t jobID(0);

         notificationsGroup.mainNotificationID = notificationID;
#ifdef USE_SERVER_CACHE         
         mPendingAttachExtendedJobs.set(++jobID, notificationsGroup);
#endif         
         void *pArg  =  static_cast<void *>(&arg);
         const uint32_t inSize = sizeof(arg.i);
         const uint32_t outSize = sizeof(arg.o);

         if (!mAsyncMode)
         {
            //client is blocked until we get an answer from the master
            job = Job::create(msg, static_cast<int32_t>(jobID), dcmd, pArg, inSize, outSize);
            msg.deferResponse();
         }
         else
         {
            //unblock the client and send the job to the master
            job = Job::create(static_cast<int32_t>(jobID), dcmd, pArg, inSize, outSize);
            *outNotificationID = notificationID;            
         }
      }
      else
      {
         *outNotificationID = notificationID;         
      }

      if (mMasterNotificationPool[pos].getState() == NotificationPoolEntry::STATE_CONNECTED)
      {
         (void)mMasterNotificationPool[pos].setState(NotificationPoolEntry::STATE_PRECACHING);
      }
      else if (mMasterNotificationPool[pos].getState() == NotificationPoolEntry::STATE_PRECACHING)
      {
         //a request to get the server information was already sent so wait for the answer
         msg.deferResponse();
      }

      if (job)      
         master().eval(job);      
   }

   return result;
}


void Servicebroker::handleGetServerInformation( SocketMessageContext &msg, ClientSpecificData &/*ocb*/,
                                                SFNDGetServerInformation &arg )
{
   SBStatus result = FNDOK;
   bool isLocal = mConfig.isLocalService( arg.i.ifDescription.name );

   Log::message( 1, "*[%d] %s %s %d.%d%s"
                 , msg.context().getId()
                 , GetDCmdString(DCMD_FND_GET_SERVER_INFORMATION)
                 , arg.i.ifDescription.name
                 , arg.i.ifDescription.version.majorVersion
                 , arg.i.ifDescription.version.minorVersion
                 , isLocal ? " [LOCAL]" : "" );

   if( check( arg.i.ifDescription ))
   {
      result = FNDUnknownInterface ;
      ServerListEntry* entry = mConnectedServers.find( arg.i.ifDescription );
#ifdef USE_SERVER_CACHE               
      int32_t cachePos(-1);
#endif      

      if (entry)
      {
         //server connected on the local node
         arg.o.channel.nid = entry->nid;
         arg.o.channel.pid = IPv4NodeAddressTranslator(msg.context()).translateLocal(entry->ifDescription.name.c_str(), entry->pid);
         arg.o.channel.chid = entry->chid;
         arg.o.serverID = entry->partyID;
         result = FNDOK;
      }
#ifdef USE_SERVER_CACHE      
      else if (-1 != (cachePos = mRemoteServersCache.find(arg.i.ifDescription)))
      {
         //we have information about the server in the cache
         arg.o = mRemoteServersCache[cachePos].getValue();
         result = FNDOK;
      }
#endif      
      else if(!isLocal && isMasterConnected())
      {
         Job *job = Job::create( msg, mNextPendingRemoteServerRequestID++, DCMD_FND_GET_SERVER_INFORMATION, &arg,
                                 sizeof(arg.i), sizeof(arg.o) );                        
         mPendingRemoteServersRequests[job->cookie] = arg.i.ifDescription;
         Log::message( 1, " Forwarding %s to master JobID = %u", GetDCmdString(DCMD_FND_GET_SERVER_INFORMATION),
                       static_cast<uint32_t>(job->cookie));

         master().eval(job);
         msg.deferResponse();
      }
   }
   else
   {
      result = FNDBadArgument;
   }

   if (msg.responseState() != MessageContext::State_DEFERRED)
   {
      if( FNDOK == result )
      {
         /* send the result back */
         msg.prepareResponse(result, &arg, sizeof(arg.o));
      }
      else
      {
         msg.prepareResponse(result);
      }
   }
}


bool Servicebroker::checkAccessPrivileges( const SocketConnectionContext& context, ServerListEntry* entry, int32_t uid )
{
   bool grantAccess = true ;

   if( entry && SB_UNKNOWN_GROUP_ID != entry->grpid )
   {
      grantAccess = false ;

      if( SB_UNKNOWN_USER_ID == uid )
      {
         uid = context.getEffectiveUid();
      }


      if( SB_UNKNOWN_USER_ID != uid )
      {
         if( Log::getLevel() > 0 )
         {
            const char* username = "<unknown>";
            passwd* pw = getpwuid( uid );
            if( pw )
            {
               username = pw->pw_name ;
            }
            Log::message( 3, " User %s wants to attach to interface %s", username, entry->ifDescription.name.c_str() );
         }

         if( 0 == uid )
         {
            // root is always allowed to access all interfaces
            grantAccess = true ;
         }
         else
         {
            group* gr = getgrgid( entry->grpid );
            if( gr )
            {
               for( char** p = gr->gr_mem; *p; p++ )
               {
                  passwd* pw = getpwnam( *p );
                  if( pw && (int)pw->pw_uid == (int)uid )
                  {
                     grantAccess = true ;
                     break;
                  }
               }
            }
         }
         if( !grantAccess )
         {
            const char* name = "<unknown>";
            passwd* pw = getpwuid( uid );
            if( pw )
            {
               name = pw->pw_name ;
            }
            Log::error( " Attaching to interface %s denied for user %s", entry->ifDescription.name.c_str(), name );

            name = "<unknown>";
            group* gr = getgrgid( entry->grpid );
            if( gr )
            {
               name = gr->gr_name ;
            }
            Log::error( " Only users of group %s are allowed to attach to the interface %s", name, entry->ifDescription.name.c_str() );
         }
      }
   }

   return grantAccess ;
}


void Servicebroker::handleDetachInterface( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceDetachArg &arg )
{
   bool immediateResponse = true;

   Log::message( 1, "*[%d] %s <%d.%d>"
                 , msg.context().getId()
                 , GetDCmdString(DCMD_FND_DETACH_INTERFACE)
                 , arg.i.clientID.s.extendedID
                 , arg.i.clientID.s.localID);

   SBStatus result = FNDInvalidClientID ;
   SPartyID clientID = arg.i.clientID ;

#ifdef USE_SERVER_CACHE
   if ((!mUseServerCache && ocb.clientExists( clientID ) && mAttachedClients.find(clientID) != -1) || mUseServerCache)
   {
#else
   if (ocb.clientExists( clientID ) && mAttachedClients.find(clientID) != -1)
   {
#endif
      // potentially mExtendedID != arg.i.clientID.s.extendedID since it will be appended
      // to the ocb after the id was created by the master -> in this case the clientID will
      // be part of the ocb structures but not part of mAttachedClients

      result = FNDOK ;

      // remove client from OCB, notify server if demanded
      mClientDetachNotifications.trigger( clientID );
      mAttachedClients.remove( clientID );
      ocb.removeClient( clientID );
   }
   if (isMasterConnected() && (mUseServerCache || (!mUseServerCache && result != FNDOK)) )
   {
      /*
       * the interface must be an interface on the master
       */
      Job *job = Job::create( msg, 0, DCMD_FND_DETACH_INTERFACE, &arg, sizeof(arg.i) );
      master().eval(job);
      immediateResponse = false;
      msg.deferResponse();

      // remove it definitely from the local ocb
      ocb.removeClient( clientID );
   }

   if( immediateResponse )
      msg.prepareResponse( result );
}


void Servicebroker::handleNotifyServerDisconnect( SocketMessageContext &msg, ClientSpecificData &ocb,
                                                  SFNDNotifyServerDisconnectArg &arg )
{
   SBStatus result;

   Log::message( 2, "*[%d] %s <%d.%d>"
                 , msg.context().getId()
                 , GetDCmdString(DCMD_FND_NOTIFY_SERVER_DISCONNECT)
                 , arg.i.serverID.s.extendedID
                 , arg.i.serverID.s.localID );   

   result = handleNotifyServerDisconnectNoResponse(msg, ocb, arg);
   if (FNDOK == result)
   {
      msg.prepareResponse( FNDOK, &arg, sizeof(arg.o) ); 
   }
   else
   {
      msg.prepareResponse(result);  
   }
}


SBStatus Servicebroker::handleNotifyServerDisconnectNoResponse(SocketMessageContext &msg, ClientSpecificData &ocb,
                                                               SFNDNotifyServerDisconnectArg &arg)
{   
   SBStatus result(FNDOK);
   SPartyID serverID = arg.i.serverID;   
      
   notificationid_t notificationID = mServerDisconnectNotifications.add(&ocb, serverID, msg.context(), arg.i.pulse);      

   if (Notification::INVALID_NOTIFICATION_ID != notificationID)
   {
      /* add notification ID */
      ocb.addNotification(notificationID);
      
      /* check if the client wants to set a notification on a local or global server */

      /* send notification if the server does not exist anymore */
      if((!mConnectedServers.find(serverID)) &&  mExtendedID != serverID.s.extendedID && isMasterConnected())
      {
         Notification* theNotification = mServerDisconnectNotifications.find(notificationID);
         if(theNotification)
         {
            // trying to find an existing notification pool
            int32_t pos = mMasterNotificationPool.find(serverID);
            if(-1 == pos)
            {
               // no pool available -> creating a new one
               pos = mMasterNotificationPool.add(serverID);

               SFNDNotifyServerDisconnectArg arg1;
               arg1.i.sbVersion.majorVersion = DSI_SERVICEBROKER_VERSION_MAJOR;
               arg1.i.sbVersion.minorVersion = DSI_SERVICEBROKER_VERSION_MINOR;
               arg1.i.serverID = serverID;
               arg1.i.pulse.pid = mSigAddr.internal.major; 
               arg1.i.pulse.chid = mSigAddr.internal.minor;
               arg1.i.pulse.code = PULSE_MASTER_SERVER_DISCONNECT;
               arg1.i.pulse.value = mMasterNotificationPool[pos].getPoolID();

               Job *job = Job::create(mMasterNotificationPool[pos].getPoolID(), DCMD_FND_NOTIFY_SERVER_DISCONNECT, &arg1,
                                      sizeof(arg1.i), sizeof(arg1.o));
               master().eval(job);
            }
            mMasterNotificationPool[pos].attach(*theNotification);            
         }
      }
      else if(!mConnectedServers.find(serverID))
      {
         mServerDisconnectNotifications.trigger(serverID);
      }

      /* send the result back */
      static char buf[50];
      
      arg.o.notificationID = notificationID;
      snprintf(buf, sizeof(buf), "PID: %d, CHID: %d", arg.i.pulse.pid, arg.i.pulse.chid);

      Log::message(1, "*[%d] Notification disconnect set on <%d.%d> for %s", msg.context().getId(),
                   serverID.s.extendedID, serverID.s.localID, buf);
      result = FNDOK;
   }
   else
   {
      result = FNDInternalError;
   }
   return result;
}


void Servicebroker::handleNotifyServerAvailable( SocketMessageContext &msg, ClientSpecificData &ocb,
                                                 SFNDNotifyServerAvailableArg &arg )
{
   SBStatus result = FNDOK;
   bool isLocal = mConfig.isLocalService( arg.i.ifDescription.name );

   Log::message( 2, "*[%d] %s %s %d.%d%s"
                 , msg.context().getId()
                 , GetDCmdString(DCMD_FND_NOTIFY_SERVER_AVAILABLE)
                 , arg.i.ifDescription.name
                 , arg.i.ifDescription.version.majorVersion
                 , arg.i.ifDescription.version.minorVersion
                 , isLocal ? " [LOCAL]" : "" );
   result = handleNotifyServerAvailableNoResponse(msg, ocb, arg);
   if (FNDOK == result)
   {
      msg.prepareResponse( FNDOK, &arg, sizeof(arg.o) );   
   }
   else
   {
      msg.prepareResponse(result);  
   }
}


SBStatus Servicebroker::handleNotifyServerAvailableNoResponse( SocketMessageContext &msg, ClientSpecificData &ocb,
                                                               SFNDNotifyServerAvailableArg &arg)
{
   SBStatus result = FNDOK;
   bool isLocal = mConfig.isLocalService( arg.i.ifDescription.name );

   /* save the interface description for later use */
   SFNDInterfaceDescription ifDescription = arg.i.ifDescription ;

   /* checking if the caller is allowed to access the interface */
   ServerListEntry* entry = mConnectedServers.find( ifDescription ) ;
   if( entry && !checkAccessPrivileges( msg.context(), entry ))
   {
      result = FNDAccessDenied;
   }
   else
   {
      /* add notification */
      notificationid_t notificationID = mServerConnectNotifications.add( &ocb
                                                               , arg.i.ifDescription
                                                               , msg.context()
                                                               , arg.i.pulse
                                                               , isLocal
                                                               , msg.context().getEffectiveUid() );

      if( Notification::INVALID_NOTIFICATION_ID == notificationID )
      {
         result = FNDBadArgument;
      }
      else
      {
         /* assign notification ID */
         ocb.addNotification( notificationID );

         /* send back the notification ID */
         arg.o.notificationID = notificationID ;

         /* send notification if server is already available locally or in the remote server cache. */
#ifdef USE_SERVER_CACHE
         if ((mUseServerCache && (entry || (-1 != mRemoteServersCache.find(arg.i.ifDescription))))
             || (!mUseServerCache && entry))
         {
#else
         if( entry )
         {
#endif
            mServerConnectNotifications.trigger( ifDescription );
         }
         else if( !isLocal && isMasterConnected() )
         {
            /* the slave must set a notification at the master */
            forwardServerAvailableNotifications();
         }
         else
         {
            ServerListEntry* entry1 = mConnectedServers.find( ifDescription.name ) ;
            if( entry1 )
            {
               Log::message( 0, "Notification set on %s %d.%d, version available: %d.%d", entry1->ifDescription.name.c_str()
                             , arg.i.ifDescription.version.majorVersion, arg.i.ifDescription.version.minorVersion
                             , entry1->ifDescription.majorVersion, entry1->ifDescription.minorVersion );
            }
         }
      }
   }

   return result;
}

void Servicebroker::handleNotifyServerAvailable( SocketMessageContext &msg, ClientSpecificData &ocb,
                                                 SFNDNotifyServerAvailableExArg &arg )
{
   SFNDNotificationID ret ;
   SFNDNotificationDescription n ;
   for( int32_t idx=0; idx<arg.i.count; idx++ )
   {
      // reading the next notification from the client
      if( -1 == msg.readAncillaryData( &n, sizeof(n)))
      {
         msg.prepareResponse(FNDBadArgument);   
         return;
      }

      ret.cookie = n.cookie ;
      ret.notificationID = static_cast<notificationid_t>(-1);

      bool isLocal = mConfig.isLocalService( n.ifDescription.name );

      Log::message( 2, "*[%d] %s %s %d.%d%s"
                    , msg.context().getId()
                    , GetDCmdString(DCMD_FND_NOTIFY_SERVER_AVAILABLE_EX)
                    , n.ifDescription.name
                    , n.ifDescription.version.majorVersion
                    , n.ifDescription.version.minorVersion
                    , isLocal ? " [LOCAL]" : "" );

      /* checking if the caller is allowed to access the interface */
      ServerListEntry* entry = mConnectedServers.find( n.ifDescription ) ;
      if( !entry || checkAccessPrivileges( msg.context(), entry ))
      {
         /* add notification */
         notificationid_t notificationID = mServerConnectNotifications.add( &ocb
                                                                  , n.ifDescription
                                                                  , msg.context()
                                                                  , n.pulse
                                                                  , isLocal
                                                                  , msg.context().getEffectiveUid() );

         if( Notification::INVALID_NOTIFICATION_ID != notificationID )
         {
            /* assign notification ID */
            ocb.addNotification( notificationID );

            /* send back the notification ID */
            ret.notificationID = notificationID ;

            /* send notification if server is already available */
            if( entry )
            {
               mServerConnectNotifications.trigger( n.ifDescription );
            }
            else if( !isLocal && isMasterConnected() )
            {
               /* the slave must set a notification at the master */
               forwardServerAvailableNotifications();
            }
            else
            {
               ServerListEntry* entry1 = mConnectedServers.find( n.ifDescription.name ) ;
               if( entry1 )
               {
                  Log::message( 0, "Notification set on %s %d.%d, version available: %d.%d", entry1->ifDescription.name.c_str()
                                , n.ifDescription.version.majorVersion, n.ifDescription.version.minorVersion
                                , entry1->ifDescription.majorVersion, entry1->ifDescription.minorVersion );
               }
            }
         }
      }
      // write the server ID to the client
      msg.writeAncillaryData( &ret, sizeof(ret));
   }
   // send it!
   msg.prepareResponse(FNDOK);  
}


void Servicebroker::handleNotifyClientDetach(SocketMessageContext &msg, ClientSpecificData &ocb,
                                             SFNDNotifyClientDetachArg &arg )
{
   Log::message( 2, "*[%d] %s <%d.%d>"
                 , msg.context().getId()
                 , GetDCmdString(DCMD_FND_NOTIFY_CLIENT_DETACH)
                 , arg.i.clientID.s.extendedID
                 , arg.i.clientID.s.localID );   
   
   notificationid_t notificationID = mClientDetachNotifications.add(&ocb
                                                       , arg.i.clientID
                                                       , msg.context()
                                                       , arg.i.pulse);

   if (notificationID != Notification::INVALID_NOTIFICATION_ID)
   {
      /* add the notification to the ocb's notification list */
      ocb.addNotification(notificationID);
      
      /* send the notification ID back */
      arg.o.notificationID = notificationID ;
      
      /* send result */
      msg.prepareResponse(FNDOK, &arg, sizeof(arg.o)); 

      /* send notification if the server does not exist anymore */
      if (isMasterConnected() && mExtendedID != arg.i.clientID.s.extendedID)
      {
         SFNDNotifyClientDetachArg arg1 ;
         arg1.i.sbVersion.majorVersion = DSI_SERVICEBROKER_VERSION_MAJOR ;
         arg1.i.sbVersion.minorVersion = DSI_SERVICEBROKER_VERSION_MINOR ;
         arg1.i.clientID = arg.i.clientID ;
         arg1.i.pulse.pid = mSigAddr.internal.major ;  
         arg1.i.pulse.chid = mSigAddr.internal.minor;  
         arg1.i.pulse.code = PULSE_MASTER_CLIENT_DETACH ;
         arg1.i.pulse.value = notificationID ;

         Job *job = Job::create(notificationID, DCMD_FND_NOTIFY_CLIENT_DETACH, &arg1, sizeof(arg1.i), sizeof(arg1.o));
         master().eval(job);
      }
      else if((!mUseServerCache || mExtendedID == arg.i.clientID.s.extendedID) && -1 == mAttachedClients.find(arg.i.clientID))
      {
         mClientDetachNotifications.trigger(arg.i.clientID);
      }
   }
   else
   {
      msg.prepareResponse(FNDInternalError); 
   }
}


void Servicebroker::handleClearNofification( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDClearNotificationArg &arg )
{
   Log::message( 2, "*[%d] %s -%d-"
                 , msg.context().getId()
                 , GetDCmdString(DCMD_FND_CLEAR_NOTIFICATION)
                 , arg.i.notificationID);

   /* version is ok, so see if the notification ID is known */
   SBStatus result = FNDInvalidNotificationID ;

   if( ocb.notificationExists( arg.i.notificationID ))
   {
      ocb.removeNotification( arg.i.notificationID );
      mClientDetachNotifications.remove( arg.i.notificationID );
      mServerDisconnectNotifications.remove( arg.i.notificationID );
      mServerConnectNotifications.remove( arg.i.notificationID );
      mServerListChangeNotifications.remove( arg.i.notificationID );
      result = FNDOK;
   }

   /* send the result back */
   msg.prepareResponse(result);
}


void Servicebroker::handleGetInterfaceList( SocketMessageContext &msg, ClientSpecificData &/*ocb*/, SFNDGetInterfaceListArg &arg )
{
   int32_t inCount = arg.i.inElementCount ;
   int32_t retCount = 0 ;

   Log::message( 2, "*[%d] %s", msg.context().getId(), GetDCmdString(DCMD_FND_GET_INTERFACELIST));

   if( isMasterConnected() )
   {
      Job *job = Job::create( msg, inCount, DCMD_FND_GET_INTERFACELIST, &arg, sizeof(arg.i), sizeof(arg.o) + inCount * sizeof(SFNDInterfaceDescription));
      master().eval(job);
      msg.deferResponse();
   }
   else
   {
      if( mConnectedServers.size() <= (uint32_t)inCount )
      {
         for( ServerList::const_iterator iter = mConnectedServers.begin(); iter != mConnectedServers.end(); iter++ )
         {
            // only return local services to a local caller
            if( !msg.context().isSlave() || !iter->local )
            {
               SFNDInterfaceDescription ifDescription = iter->ifDescription ;
               msg.writeAncillaryData( &ifDescription, sizeof(ifDescription));
               retCount++;
            }
         }

         msg.prepareResponse(-retCount);   // note the minus here!
      }
      else
         msg.prepareResponse(FNDBadArgument); 
   }
}


void Servicebroker::handleNotifyInterfaceListChange( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDNotifyInterfaceListChangeArg &arg )
{
   Log::message( 2, "*[%d] %s", msg.context().getId(), GetDCmdString(DCMD_FND_NOTIFY_INTERFACELIST_CHANGE));

   notificationid_t notificationID = mServerListChangeNotifications.add( msg.context(), arg.i.pulse );

   if( Notification::INVALID_NOTIFICATION_ID != notificationID )
   {
      /* add the notification to the ocb's notification list */
      ocb.addNotification( notificationID );

      if( !isMasterConnected() )
      {
         /* send the notification ID back */
         arg.o.notificationID = notificationID ;

         /* send result */
         msg.prepareResponse( FNDOK, &arg, sizeof(arg.o) );
      }
      else
      {
         arg.i.pulse.pid = mSigAddr.internal.major;   
         arg.i.pulse.chid = mSigAddr.internal.minor;  
         arg.i.pulse.code = PULSE_MASTER_SERVERLIST_CHANGE ;
         arg.i.pulse.value = notificationID ;

         /*
          * set master notification
          */
         Job *job = Job::create( notificationID, DCMD_FND_NOTIFY_INTERFACELIST_CHANGE, &arg, sizeof(arg.i), sizeof(arg.o) );
         master().eval( job );

         /* send the notification ID back */
         arg.o.notificationID = notificationID ;

         /* send result */
         msg.prepareResponse( FNDOK, &arg, sizeof(arg.o) );
      }
   }
   else
      msg.prepareResponse(FNDBadArgument);
}


void Servicebroker::handleMatchInterfaceList( SocketMessageContext &msg, ClientSpecificData &/*ocb*/, SFNDMatchInterfaceListArg &arg )
{
   Log::message( 2, "*[%d] %s /%s/", msg.context().getId(), GetDCmdString(DCMD_FND_MATCH_INTERFACELIST), arg.i.regExpr);

   RegExp re ;
   int32_t err = re.compile( arg.i.regExpr, REG_EXTENDED | REG_NOSUB );
   if( 0 != err )
   {
      char buffer[128];
      (void)re.error( err, buffer, sizeof(buffer)-1);
      Log::error( "RegExp Error: %s /%s/", buffer, arg.i.regExpr );
      msg.prepareResponse(-(int32_t)FNDRegularExpression, 0, 0 );
   }
   else if( isMasterConnected() )
   {
      Job *job = Job::create( msg, arg.i.inElementCount, DCMD_FND_MATCH_INTERFACELIST, &arg, sizeof(arg.i), sizeof(arg.o) + arg.i.inElementCount * sizeof(SFNDInterfaceDescription));
      job->regex = re ;
      master().eval(job);
      msg.deferResponse();
   }
   else
   {
      int32_t retCount = 0 ;
      int32_t inCount = arg.i.inElementCount ;

      bool sendResult = true;

      for( ServerList::const_iterator iter = mConnectedServers.begin(); iter != mConnectedServers.end(); iter++ )
      {
         if( 0 == re.execute( iter->ifDescription.name, 0 ) )
         {
            if( retCount >= inCount )
            {
               sendResult = false ;
               break;
            }
            else
            {
               // only return local services to a local caller
               if( !msg.context().isSlave() || !iter->local )
               {
                  SFNDInterfaceDescription ifDescription = iter->ifDescription ;
                  msg.writeAncillaryData( &ifDescription, sizeof(ifDescription));
                  retCount++;
               }
            }
         }
      }

      if( sendResult == true )
      {
         msg.prepareResponse( -retCount );   // note the minus here!
      }
      else
         msg.prepareResponse(FNDBadArgument);
   }
}


void Servicebroker::handleNotifyInterfaceListMatch( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDNotifyInterfaceListMatchArg &arg )
{
   Log::message( 2, "*[%d] %s /%s/", msg.context().getId(), GetDCmdString(DCMD_FND_NOTIFY_INTERFACELIST_MATCH),
                 arg.i.regExpr );

   notificationid_t notificationID = mServerListChangeNotifications.add( arg.i.regExpr, msg.context(), arg.i.pulse );

   if( Notification::INVALID_NOTIFICATION_ID != notificationID )
   {
      /* add the notification to the ocb's notification list */
      ocb.addNotification( notificationID );

      if( isMasterConnected() )
      {
         /*
          * set master notification
          */
         SFNDNotifyInterfaceListMatchArg marg ;
         marg.i.sbVersion.majorVersion = DSI_SERVICEBROKER_VERSION_MAJOR ;
         marg.i.sbVersion.minorVersion = DSI_SERVICEBROKER_VERSION_MINOR ;
         marg.i.pulse.pid = mSigAddr.internal.major; 
         marg.i.pulse.chid = mSigAddr.internal.minor;
         marg.i.pulse.code = PULSE_MASTER_SERVERLIST_CHANGE ;
         marg.i.pulse.value = notificationID ;
         strncpy( marg.i.regExpr, arg.i.regExpr, sizeof(marg.i.regExpr)) ;

         Job *job = Job::create( notificationID, DCMD_FND_NOTIFY_INTERFACELIST_MATCH, &marg, sizeof(marg.i), sizeof(marg.o) );
         master().eval( job );
      }

      /* send the notification ID back */
      arg.o.notificationID = notificationID ;

      /* send result */
      msg.prepareResponse( FNDOK, &arg, sizeof(arg.o) ); 

      /* check for matching interfaces */
      Notification *n = mServerListChangeNotifications.find( notificationID );
      if( n )
      {
         for( ServerList::const_iterator iter = mConnectedServers.begin(); iter != mConnectedServers.end(); iter++ )
         {
            if( SB_LOCAL_NODE_ADDRESS == msg.context().getNodeAddress() || !iter->local )
            {
               if( n->matchRegExp( iter->ifDescription.name ))
               {
                  /* we found at least one match for the regular expression */
                  n->send();
                  break;
               }
            }
         }
      }
   }
   else
      msg.prepareResponse(FNDBadArgument);
}


void Servicebroker::handleMasterPingId(SocketMessageContext &msg, ClientSpecificData &ocb, SFNDMasterPingIdArg &arg )
{
   Log::message( 4, "*[%d] %s extendedID=%d", msg.context().getId(), GetDCmdString(DCMD_FND_MASTER_PING_ID), arg.i.id );

   ocb.setExtendedID(arg.i.id);
   const_cast<SocketConnectionContext&>(msg.context()).setSlave(true);

   msg.prepareResponse(FNDOK);
}


// ----------------------------------------------------------------------------------------------


void Servicebroker::unregisterInterfaceMaster( ServerListEntry &entry )
{
   if( 0 < (int32_t)entry.masterID.s.extendedID && isMasterConnected() )
   {
      SFNDInterfaceUnregisterArg arg ;
      arg.i.sbVersion.majorVersion = DSI_SERVICEBROKER_VERSION_MAJOR ;
      arg.i.sbVersion.minorVersion = DSI_SERVICEBROKER_VERSION_MINOR ;
      arg.i.serverID = entry.masterID ;
      entry.masterID = 0;

      Job *job = Job::create( 0, DCMD_FND_UNREGISTER_INTERFACE, &arg, sizeof(arg.i) );
      master().eval(job);
   }
}

void Servicebroker::registerInterfacesMaster()
{
   if( isMasterConnected() )
   {   
      struct MasterRegArg
      {
         SFNDInterfaceRegisterMasterExArg hdr ;
         SFNDInterfaceDescriptionEx descr[MAX_MULTIPLE_COUNT];
      } *arg = new MasterRegArg() ;
      if(arg)
      {    
         memset( arg, 0, sizeof(*arg));
         arg->hdr.i.sbVersion.majorVersion = DSI_SERVICEBROKER_VERSION_MAJOR ;
         arg->hdr.i.sbVersion.minorVersion = DSI_SERVICEBROKER_VERSION_MINOR ;

         for( ServerList::iterator iter = mConnectedServers.begin(); iter != mConnectedServers.end(); iter++ )
         {                
            if( !iter->masterID && !iter->local && mConfig.forwardService(iter->ifDescription.name))
            {            
               arg->descr[arg->hdr.i.ifCount].ifDescription = iter->ifDescription ;
               arg->descr[arg->hdr.i.ifCount].implVersion = iter->implVersion ;
               arg->descr[arg->hdr.i.ifCount].chid = iter->chid ;
               arg->descr[arg->hdr.i.ifCount].pid = iter->pid ;
               arg->descr[arg->hdr.i.ifCount].nd = iter->nid ; // translate node?!?
               arg->descr[arg->hdr.i.ifCount].serverID = iter->partyID ;
               iter->masterID = static_cast<notificationid_t>(-1) ;

               if( ++arg->hdr.i.ifCount == MAX_MULTIPLE_COUNT )
               {             
                  Job *job = Job::create( arg->hdr.i.ifCount, DCMD_FND_REGISTER_MASTER_INTERFACE_EX, arg, sizeof(arg->hdr) + (arg->hdr.i.ifCount*sizeof(arg->descr[0])), arg->hdr.i.ifCount*sizeof(arg->hdr.o.serverID[0]));
                  master().eval(job);
                  arg->hdr.i.ifCount = 0 ;
               }
            }
         }
         if( 0 < arg->hdr.i.ifCount )
         {         
            Job *job = Job::create( arg->hdr.i.ifCount, DCMD_FND_REGISTER_MASTER_INTERFACE_EX, arg, sizeof(arg->hdr) + (arg->hdr.i.ifCount*sizeof(arg->descr[0])), arg->hdr.i.ifCount*sizeof(arg->hdr.o.serverID[0]));
            master().eval(job);
         }
         delete arg ;
      }
   }
}


void Servicebroker::createClearNotificationJob( notificationid_t notificationID )
{
   if( isMasterConnected() )
   {
      SFNDClearNotificationArg arg ;
      arg.i.sbVersion.majorVersion = DSI_SERVICEBROKER_VERSION_MAJOR ;
      arg.i.sbVersion.minorVersion = DSI_SERVICEBROKER_VERSION_MINOR ;
      arg.i.notificationID = notificationID ;

      Job *job = Job::create( 0, DCMD_FND_CLEAR_NOTIFICATION, &arg, sizeof(arg.i) );
      master().eval( job );
   }
}


void Servicebroker::handleRegisterMasterInterface( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceRegisterMasterExArg &arg )
{
   bool success = true;
   SFNDInterfaceDescriptionEx ifDescr ;

   for( int32_t idx=0; idx<arg.i.ifCount; idx++ )
   {
      if( -1 == msg.readAncillaryData( &ifDescr, sizeof(ifDescr)))
      {
         success = false ;
         Log::error("Error reading interface data from slave");
         break;
      }

      Log::message( 1, "*[%d] %s %s %d.%d <%d.%d>, pid:%d, chid:%d"
                    , msg.context().getId()
                    , GetDCmdString(DCMD_FND_REGISTER_MASTER_INTERFACE_EX)
                    , ifDescr.ifDescription.name
                    , ifDescr.ifDescription.version.majorVersion
                    , ifDescr.ifDescription.version.minorVersion
                    , ifDescr.serverID.s.extendedID
                    , ifDescr.serverID.s.localID
                    , ifDescr.pid
                    , ifDescr.chid );

      if( ifDescr.serverID.s.extendedID == mExtendedID )
      {
         Log::error( " ID conflict: %d", ifDescr.serverID.s.extendedID );

         // return error server ID
         SPartyID serverID ;
         serverID.s.localID = ifDescr.serverID.s.localID ;
         serverID.s.extendedID = ~0 ;
         msg.writeAncillaryData(&serverID, sizeof(serverID));
      }
      else if( !check( ifDescr.ifDescription ) || ifDescr.chid <= 0 )
      {
         Log::error( " Interface check failed for %s", ifDescr.ifDescription.name );

         // return error server ID
         SPartyID serverID ;
         serverID.s.localID = ifDescr.serverID.s.localID ;
         serverID.s.extendedID = ~0 ;
         msg.writeAncillaryData(&serverID, sizeof(serverID));
      }
      else if(mConnectedServers.find( ifDescr.ifDescription.name ))
      {
         /* the interface name already exists -> error */
         Log::error( " Interface %s already registered", ifDescr.ifDescription.name );

         // return error server ID
         SPartyID serverID ;
         serverID.s.localID = ifDescr.serverID.s.localID ;
         serverID.s.extendedID = ~0 ;
         msg.writeAncillaryData(&serverID, sizeof(serverID));
      }
      else if( ifDescr.serverID.s.extendedID == 0 && 1000U != mExtendedID )
      {
         Log::error( " standard slave cannot connect to tree mode servicebroker" );

         // return error server ID
         SPartyID serverID ;
         serverID.s.localID = ifDescr.serverID.s.localID ;
         serverID.s.extendedID = ~0 ;
         msg.writeAncillaryData(&serverID, sizeof(serverID));
      }
      else
      {
         // rewrite ip address (=pid) to the given peer address
         //  * if it is of value INADDR_LOOPBACK
         //  * and the SB-transport type is TCP/IP
         //  * the interface is of type DSI over TCP/IP (name contains _tcp at the end)
         ifDescr.pid = IPv4NodeAddressTranslator(msg.context()).translateRemote(ifDescr.ifDescription.name, ifDescr.pid);

         /* add a new server */
         ServerListEntry newEntry ;
         newEntry.ifDescription = ifDescr.ifDescription;
         newEntry.partyID = ifDescr.serverID ;

         if( newEntry.partyID.s.extendedID == 0 )
         {
            newEntry.partyID.s.extendedID = 1000U + msg.context().getId() ;   // add arbitrary unique id potentially for debugging purposes
         }

         newEntry.nid = msg.context().getNodeAddress();
         newEntry.pid = ifDescr.pid;
         newEntry.chid = ifDescr.chid;
         newEntry.implVersion = ifDescr.implVersion ;

         /* insert interface */
         mConnectedServers.add( newEntry );
         /* insert server ID */
         ocb.addServer( newEntry.partyID );

         Log::message( 1, " SERVER_ID <%d.%d>", newEntry.partyID.s.extendedID, newEntry.partyID.s.localID );

         // write the server ID to the client
         msg.writeAncillaryData( &newEntry.partyID, sizeof(newEntry.partyID));

         /* notify clients waiting for this interface */
         mServerConnectNotifications.trigger( newEntry.ifDescription );

         /* triger serverlist change notifications */
         mServerListChangeNotifications.triggerAll( newEntry.ifDescription );
      }
   }

   /* send back the answer to the client */
   msg.prepareResponse(success ? FNDOK : FNDBadArgument); 

   /* forward interfaces to next servicebroker */
   registerInterfacesMaster();
}


void Servicebroker::dumpStats(std::ostringstream& ostream)
{   
   ostream << "servicebroker " << (isMaster() ? "master" : "slave") << "\n" ;
   ostream << "build: " << Servicebroker::BUILD_TIME << "\n" ;
   ostream << "compile settings: ";
   ostream << "tcp iface, tcp client, ";
#ifdef HAVE_TCP_CLIENT_TIMEOUT_SUPERVISION
   ostream << "tcp timeo supervision, ";
#endif
#ifdef HAVE_HTTP_SERVICE
   ostream << "http iface, ";
#endif
#ifdef USE_SERVER_CACHE
   ostream << "cache enabled, ";
#endif
   ostream << "tcp send/recv timeo=" << SB_MASTERADAPTER_SENDTIMEO << "/" << SB_MASTERADAPTER_RECVTIMEO << " ms";
   ostream << "\n";

   if( !isMaster() )
   {
      ostream << "master: " << master().address() << " (" << (isMasterConnected() ? "" : "NOT ") <<"CONNECTED)\n" ;
   }
   ostream << "version: " << Servicebroker::MAJOR_VERSION << "." << Servicebroker::MINOR_VERSION << "." << Servicebroker::BUILD_VERSION << "\n" ;

   ostream << "\nStatistics:";
   dumpStatistics(ostream);
   ostream << "\n";

   if( 0 != mConnectedServers.size() )
   {
      ostream << "\nConnected Servers:\n" ;
      dumpServers(ostream);
   }

   if( 0 != mAttachedClients.size() )
   {
      ostream << "\nConnected Clients:\n" ;
      dumpClients(ostream);
   }

   if( 0 != mServerConnectNotifications.size())
   {
      ostream << "\nServer Connect Notifications:\n" ;
      dumpServerConnectNotifications(ostream);
   }

   if( 0 != mServerDisconnectNotifications.size())
   {
      ostream << "\nServer Disconnect Notifications:\n" ;
      dumpServerDisconnectNotifications(ostream);
   }

   if( 0 != mClientDetachNotifications.size())
   {
      ostream << "\nClient Detach Notifications:\n" ;
      dumpClientDetachNotifications(ostream);
   }

   mConfig.dumpStats( ostream );

   ostream << "\n" ;
}


void Servicebroker::dumpStatistics(std::ostringstream& ostream) const
{
   ostream << "\n   Servers: " << mConnectedServers.size() << "|" << mConnectedServers.getTotalCounter();
#ifdef USE_SERVER_CACHE
   ostream << " , cached : " << mRemoteServersCache.size();
#endif
   ostream << "\n   Clients: " << mAttachedClients.size() << "|" << mAttachedClients.getTotalCounter();
   ostream <<"\n   Notifications : " << Notification::getCurrentCounter() << "|"
           << Notification::getTotalCounter() << " , proxies: " << mMasterNotificationPool.size() << "|"
           << mMasterNotificationPool.getTotalCounter();
   ostream << "\n      Server Connect: " << mServerConnectNotifications.size() << "|"
           << mServerConnectNotifications.getTotalCounter();
   ostream << "\n      Server Disconnect: " << mServerDisconnectNotifications.size() << "|"
           << mServerDisconnectNotifications.getTotalCounter();
   ostream << "\n      Client Detach: " << mClientDetachNotifications.size() << "|"
           << mClientDetachNotifications.getTotalCounter();
   if (isMaster())
   {
      ostream << "\n   Jobs: 0|0";
   }
   else
   {
      ostream << "\n   Jobs: " << master().getJobQueueSize() << "|" << master().getJobQueueTotalCounter();
   }
}


void Servicebroker::forwardServerAvailableNotifications()
{
   if(isMasterConnected())
   {
      struct MasterNotifyArg
      {
         SFNDNotifyServerAvailableExArg hdr ;
         SFNDNotificationDescription n[MAX_MULTIPLE_COUNT];
      } *arg = new MasterNotifyArg() ;
      if(arg)
      {
         memset(arg, 0, sizeof(*arg));
         arg->hdr.i.sbVersion.majorVersion = DSI_SERVICEBROKER_VERSION_MAJOR ;
         arg->hdr.i.sbVersion.minorVersion = DSI_SERVICEBROKER_VERSION_MINOR ;

         for( NotificationList::iterator iter = mServerConnectNotifications.begin(); iter != mServerConnectNotifications.end(); ++iter )
         {
            if( 0 == iter->poolID && !iter->local && mConfig.forwardService(iter->ifDescription.name) )
            {
               // trying to find an existing notification pool
               int32_t pos = mMasterNotificationPool.find( iter->ifDescription );
               if( -1 == pos )
               {
                  // no pool available -> creating a new one
                  pos = mMasterNotificationPool.add( iter->ifDescription ) ;

                  arg->n[arg->hdr.i.count].cookie = mMasterNotificationPool[pos].getPoolID() ;
                  arg->n[arg->hdr.i.count].ifDescription = iter->ifDescription ;
                  arg->n[arg->hdr.i.count].pulse.pid = mSigAddr.internal.major ; 
                  arg->n[arg->hdr.i.count].pulse.chid = mSigAddr.internal.minor ;
                  arg->n[arg->hdr.i.count].pulse.code = PULSE_MASTER_SERVER_AVAILABLE ;
                  arg->n[arg->hdr.i.count].pulse.value = mMasterNotificationPool[pos].getPoolID() ;

                  if( ++arg->hdr.i.count == MAX_MULTIPLE_COUNT )
                  {
                     Job *job = Job::create( arg->hdr.i.count, DCMD_FND_NOTIFY_SERVER_AVAILABLE_EX, arg, sizeof(arg->hdr) + (arg->hdr.i.count*sizeof(arg->n[0])), arg->hdr.i.count*sizeof(arg->hdr.o.notifications[0]));
                     master().eval(job);
                     arg->hdr.i.count = 0 ;
                  }
               }
               // adding the notification to the pool
               iter->poolID = mMasterNotificationPool[pos].getPoolID() ;
            }
         }
         if( 0 < arg->hdr.i.count )
         {
            Job *job = Job::create( arg->hdr.i.count, DCMD_FND_NOTIFY_SERVER_AVAILABLE_EX, arg, sizeof(arg->hdr) + (arg->hdr.i.count*sizeof(arg->n[0])), arg->hdr.i.count*sizeof(arg->hdr.o.notifications[0]));
            master().eval(job);
         }
         delete arg ;
      }
   }
}


void Servicebroker::handleJobFinished(Job& job)
{
   if( 0 != job.status )
   {
      Log::error( "Job %s (0x%08X) returned error %d", GetDCmdString(job.dcmd), job.dcmd, job.status );
   }

   if(0 == job.status)
   {
      static char errMsg[50];

      if (job.ret_val != FNDOK)      
         snprintf(errMsg, sizeof(errMsg), "WARNING %s", toString(static_cast<SBStatus>(job.ret_val)));      
      
      switch( job.dcmd )
      {
      case DCMD_FND_ATTACH_INTERFACE:
      {
         SFNDInterfaceAttachArg* arg = (SFNDInterfaceAttachArg*) job.arg ;
         if( arg )
         {
            Log::message( 1, " %s %s JobID: %u - NID: %u, PID: %d, CHID: %d, CLIENT_ID: "
                          " <%u.%u>, SERVER_ID: <%u.%u>"
                          , GetDCmdString(DCMD_FND_ATTACH_INTERFACE)
                          , (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg
                          , static_cast<uint32_t>(job.cookie)
                          , arg->o.channel.nid
                          , arg->o.channel.pid
                          , arg->o.channel.chid
                          , arg->o.clientID.s.extendedID
                          , arg->o.clientID.s.localID
                          , arg->o.serverID.s.extendedID
                          , arg->o.serverID.s.localID);
            job.getConnectionContext().getData().addClient(arg->o.clientID);
#ifdef USE_SERVER_CACHE
            int32_t pendingReqPos;

            if (mUseServerCache && -1 != (pendingReqPos = mPendingRemoteServersRequests.find(static_cast<uint32_t>(job.cookie))))
            {
               SServerInfo serverInfo;

               serverInfo.channel = arg->o.channel;
               serverInfo.serverID = arg->o.serverID;
               mRemoteServersCache.set(mPendingRemoteServersRequests[pendingReqPos].getValue(), serverInfo);
               dumpCache();
               mPendingRemoteServersRequests.erase(pendingReqPos);
            }
#endif
         }
      }
      break;
      
      case DCMD_FND_ATTACH_INTERFACE_EXTENDED:
      {
         SFNDInterfaceAttachExtendedArg* arg = (SFNDInterfaceAttachExtendedArg*) job.arg ;

         if (arg)
         {
            const uint32_t poolID(static_cast<uint32_t>(job.cookie));

            Log::message(1, " %s %s JobID: %u - NID: %u, PID: %d, CHID: %d, "
                         " CLIENT_ID: <%u.%u>, SERVER_ID: <%u.%u>, NOTIFICATION:%u"
                         ,  GetDCmdString(DCMD_FND_ATTACH_INTERFACE_EXTENDED)
                         , (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg
                         , poolID
                         , arg->o.connInfo.channel.nid
                         , arg->o.connInfo.channel.pid
                         , arg->o.connInfo.channel.chid
                         , arg->o.connInfo.clientID.s.extendedID
                         , arg->o.connInfo.clientID.s.localID
                         , arg->o.connInfo.serverID.s.extendedID
                         , arg->o.connInfo.serverID.s.localID
                         , arg->o.notificationID);            

            handleJobFinishedAttachExtended(*arg, job);
         }
      }
      break;

      case DCMD_FND_GET_SERVER_INFORMATION:
      {
         SFNDGetServerInformation* arg = (SFNDGetServerInformation*) job.arg ;
         if( arg )
         {
            std::map<uint32_t, InterfaceDescription>::iterator pendingReqIterator;

            Log::message( 1, " %s %s JobId: %u -  NID: %d, PID: %d, CHID: %d, "
                          " SERVER_ID: <%d.%d>"
                          , GetDCmdString(DCMD_FND_GET_SERVER_INFORMATION)
                          , (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg
                          , static_cast<uint32_t>(job.cookie)
                          , arg->o.channel.nid
                          , arg->o.channel.pid
                          , arg->o.channel.chid
                          , arg->o.serverID.s.extendedID
                          , arg->o.serverID.s.localID);
            if (mPendingRemoteServersRequests.end() != (pendingReqIterator = mPendingRemoteServersRequests.find(static_cast<uint32_t>(job.cookie))))
            {
#ifdef USE_SERVER_CACHE            
               mRemoteServersCache[pendingReqIterator->second] = arg->o;
               dumpCache();
#endif               
               mPendingRemoteServersRequests.erase(pendingReqIterator);
            }
            else
            {
               Log::error( "No pending remote server request corresponding to the job found");
               job.status = static_cast<int32_t>(FNDInternalError);
            }
         }
         else
         {
            job.status = static_cast<int32_t>(FNDInternalError);
         }
      }
      break;

      case DCMD_FND_NOTIFY_SERVER_AVAILABLE_EX:
      {
         Log::message( 1, " %s %s", GetDCmdString(DCMD_FND_NOTIFY_SERVER_AVAILABLE_EX),
                       (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg);
         SFNDNotifyServerAvailableExArg* arg = (SFNDNotifyServerAvailableExArg*) job.arg ;
         if( arg )
         {
            for( int32_t idx=0; idx<job.cookie; idx++ )
            {
               if( Notification::INVALID_NOTIFICATION_ID == arg->o.notifications[idx].notificationID )
               {
               }
               else
               {
                  int32_t pos = mMasterNotificationPool.find( arg->o.notifications[idx].cookie );
                  if( -1 != pos )
                  {
                     mMasterNotificationPool[pos].notificationID = arg->o.notifications[idx].notificationID ;
                  }
                  else
                  {
                     // clear the master notification again
                     createClearNotificationJob( arg->o.notifications[idx].notificationID );
                  }
               }
            }
         }
      }
      break;

      case DCMD_FND_NOTIFY_SERVER_DISCONNECT:
      {
         SFNDNotifyServerDisconnectArg* arg = (SFNDNotifyServerDisconnectArg*) job.arg ;
         if( arg )
         {
            Log::message( 1, " %s %s JobID: %u NOTIFICATION: %u"
                          , GetDCmdString(DCMD_FND_NOTIFY_SERVER_DISCONNECT)
                          , (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg
                          , static_cast<uint32_t>(job.cookie), arg->o.notificationID);
            // "job.cookie" is the local notificationID
            int32_t pos = mMasterNotificationPool.find( job.cookie );
            if( -1 != pos )
            {
               mMasterNotificationPool[pos].notificationID = arg->o.notificationID ;
            }
            else
            {
               // clear the master notification again
               createClearNotificationJob( arg->o.notificationID );
            }
         }
      }
      break;

      case DCMD_FND_NOTIFY_CLIENT_DETACH:
      {
         SFNDNotifyClientDetachArg *arg = (SFNDNotifyClientDetachArg*) job.arg ;
         if( arg )
         {
            Log::message( 1, " %s %s NOTIFICATION: %u", GetDCmdString(DCMD_FND_NOTIFY_CLIENT_DETACH)
                          , (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg, arg->o.notificationID);
            // "job.cookie" is the local notificationID
            Notification *entry = mClientDetachNotifications.find( job.cookie );
            if( entry )
            {
               entry->masterNotificationID = arg->o.notificationID ;
            }
            else
            {
               // clear the master notification again
               createClearNotificationJob( arg->o.notificationID );
            }
         }
      }
      break;

      case DCMD_FND_NOTIFY_INTERFACELIST_CHANGE:
      {
         SFNDNotifyInterfaceListChangeArg *arg = (SFNDNotifyInterfaceListChangeArg*) job.arg ;
         if( arg )
         {
            Log::message( 1, " %s %s NOTIFICATION: %u", GetDCmdString(DCMD_FND_NOTIFY_INTERFACELIST_CHANGE)
                          , (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg, arg->o.notificationID);
            // "job.cookie" is the local notificationID
            Notification *entry = mServerListChangeNotifications.find( job.cookie );
            if( entry )
            {
               entry->masterNotificationID = arg->o.notificationID ;
            }
            else
            {
               // clear the master notification again
               createClearNotificationJob( arg->o.notificationID );
            }
         }
      }
      break;

      case DCMD_FND_NOTIFY_INTERFACELIST_MATCH:
      {
         SFNDNotifyInterfaceListMatchArg *arg = (SFNDNotifyInterfaceListMatchArg*) job.arg ;
         if( arg )
         {
            Log::message( 1, " %s %s NOTIFICATION: %u", GetDCmdString(DCMD_FND_NOTIFY_INTERFACELIST_MATCH)
                          , (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg, arg->o.notificationID);
            // "job.cookie" is the local notificationID
            Notification *entry = mServerListChangeNotifications.find( job.cookie );
            if( entry )
            {
               entry->masterNotificationID = arg->o.notificationID ;
            }
            else
            {
               // clear the master notification again
               createClearNotificationJob( arg->o.notificationID );
            }
         }
      }
      break;

      case DCMD_FND_REGISTER_MASTER_INTERFACE_EX:
      {
         Log::message( 1, " %s %s", GetDCmdString(DCMD_FND_REGISTER_MASTER_INTERFACE_EX)
                       , (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg);
         SFNDInterfaceRegisterMasterExArg *arg = (SFNDInterfaceRegisterMasterExArg*) job.arg ;
         if( arg )
         {
            for( int32_t idx=0; idx<job.cookie; idx++ )
            {
               ServerListEntry *entry = mConnectedServers.findByLocalIDandInvalidMasterID( arg->o.serverID[idx].s.localID );
               if( entry )
               {
                  if( arg->o.serverID[idx].s.extendedID == (uint32_t)~0 )
                  {
                     Log::error("Error forwarding interface to master (see master log for details)" );
                  }
                  else
                  {
                     entry->masterID = arg->o.serverID[idx] ;
                  }
               }
               else
               {
                  // seems like the interface has been removed again
                  ServerListEntry se ;
                  se.masterID = arg->o.serverID[idx] ;
                  unregisterInterfaceMaster( se );
               }
            }
         }
      }
      break;

      case DCMD_FND_GET_INTERFACELIST:

         Log::message( 1, " %s %s", GetDCmdString(DCMD_FND_GET_INTERFACELIST)
                       , (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg);
         if( 0 == job.status )
         {
            SFNDGetInterfaceListArg *arg = (SFNDGetInterfaceListArg*) job.arg ;
            if( arg )
            {
               // append local interfaces
               for( ServerList::const_iterator iter = mConnectedServers.begin(); iter != mConnectedServers.end(); iter++ )
               {
                  if( iter->local || !mConfig.forwardService(iter->ifDescription.name))
                  {
                     if( job.ret_val >= job.cookie )
                     {
                        // return buffer not big enough
                        job.status = EINVAL ;
                        job.rbytes = 0 ;
                        break;
                     }
                     else
                     {
                        arg->o.interfaceDescriptions[job.ret_val++] = iter->ifDescription ;
                     }
                  }
               }

               if( 0 == job.status )
               {
                  int i;
                  Log::message( 1, "DCMD_FND_GET_INTERFACELIST Job Finished %d",job.ret_val );
                  for (i = 0; i < job.ret_val; i++)
                  {
                     Log::message( 4, "DCMD_FND_GET_INTERFACELIST Job Finished %d %s",i,arg->o.interfaceDescriptions[i].name);
                  }
                  job.rbytes = job.ret_val * sizeof(SFNDInterfaceDescription);
                  job.ret_val = -job.ret_val;   // note the minus here!
               }
               else
               {
                  Log::message( 1, "DCMD_FND_GET_INTERFACELIST Job Finished failed %d",job.status );
               }
            }
         }
         break;


      case DCMD_FND_MATCH_INTERFACELIST:

         Log::message( 1, " %s %s", GetDCmdString(DCMD_FND_MATCH_INTERFACELIST)
                       , (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg);
         if( 0 == job.status )
         {
            SFNDGetInterfaceListArg *arg = (SFNDGetInterfaceListArg*) job.arg ;
            if( arg )
            {
               // append local interfaces
               for( ServerList::const_iterator iter = mConnectedServers.begin(); iter != mConnectedServers.end(); iter++ )
               {
                  if( (iter->local || !mConfig.forwardService(iter->ifDescription.name)) && 0 == job.regex.execute( iter->ifDescription.name, 0 ))
                  {
                     if( job.ret_val >= job.cookie )
                     {
                        // return buffer not big enough
                        job.status = EINVAL ;
                        job.rbytes = 0 ;
                        break;
                     }
                     else
                     {
                        arg->o.interfaceDescriptions[job.ret_val++] = iter->ifDescription ;
                     }
                  }
               }

               if( 0 == job.status )
               {
                  int i;
                  Log::message( 1, "DCMD_FND_MATCH_INTERFACELIST Job Finished %d",job.ret_val );
                  for (i = 0; i < job.ret_val; i++)
                  {
                     Log::message( 4, "DCMD_FND_MATCH_INTERFACELIST Job Finished %d %s",i,arg->o.interfaceDescriptions[i].name);
                  }
                  job.rbytes = job.ret_val * sizeof(SFNDInterfaceDescription);
                  job.ret_val = -job.ret_val;   // note the minus here!
               }
               else
               {
                  Log::message( 1, "DCMD_FND_MATCH_INTERFACELIST Job Finished failed %d", job.status);
               }
            }
         }
         break;

      case DCMD_FND_DETACH_INTERFACE:
         Log::message( 1, " %s %s ", GetDCmdString(DCMD_FND_DETACH_INTERFACE),
                       (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg);
         break;

      case DCMD_FND_UNREGISTER_INTERFACE:
         Log::message( 1, " %s %s ", GetDCmdString(DCMD_FND_UNREGISTER_INTERFACE)
                       , (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg);
         break;

      case DCMD_FND_CLEAR_NOTIFICATION:
         Log::message( 1, " %s %s ", GetDCmdString(DCMD_FND_CLEAR_NOTIFICATION)
                       , (FNDOK == job.ret_val) ?  "SUCCESS" : errMsg);
         break;

      default:
         Log::warning(3, "DCMD %u command not handled", job.dcmd);
         break;
      }    
   }
   else
   {
      Log::error("Job ERROR %d", job.status);
   }

   job.finalize();
}


void Servicebroker::handleJobFinishedAttachExtended(SFNDInterfaceAttachExtendedArg &arg, Job &job)
{
   NotificationPoolEntry::EPoolState poolState(NotificationPoolEntry::STATE_UNDEFINED);
   const uint32_t jobID(static_cast<uint32_t>(job.cookie));
   Notification *theNotification(0);
   int32_t poolPos(-1);
   std::map<uint32_t, tNotificationsGroup>::iterator jobIter = mPendingAttachExtendedJobs.find(jobID);

   //do all consistency checkings and set before the error code for all
   job.status = FNDInternalError;
   if (mPendingAttachExtendedJobs.end() == jobIter)
   {
      Log::error("Pending attach extended job with ID %u not found", jobID);
   }
   else if (!(theNotification = mServerConnectNotifications.find(
                 jobIter->second.mainNotificationID)))
   {
      Log::error("No notification with ID %u found for attach extended job",
                 jobIter->second.mainNotificationID);
   }
   else if (-1 == (poolPos = mMasterNotificationPool.find(theNotification->poolID)))
   {
      Log::error("No pool %u found for attach extended job", theNotification->poolID);
   }
   else if ((poolState = mMasterNotificationPool[poolPos].getState()) != NotificationPoolEntry::STATE_DEFERRED
            && poolState != NotificationPoolEntry::STATE_PRECACHING)
   {
      Log::error("Inconsistent state %s for pool %u", NotificationPoolEntry::toString(poolState));
   }   
   else
   {      
      notificationid_t *mainNotificationID = &arg.o.notificationID;
      struct SConnectionInfo *connInfo = &arg.o.connInfo;
      
      int32_t count(0);
      const uint32_t poolID(theNotification->poolID);

      //all consistency checkings done
      job.status = 0;      
      
      //if server is found on master, we have a disconnect notification otherwise connect
      const bool isDisconnectNotification = connInfo->channel.chid != 0;

      mMasterNotificationPool[poolPos].notificationID = *mainNotificationID;
      count = mServerConnectNotifications.setMasterNotificationID(*mainNotificationID, poolID);
      //restore the notification ids on the local node and rewrite the one received from master
      *mainNotificationID = jobIter->second.mainNotificationID;      
      if (NotificationPoolEntry::STATE_DEFERRED == poolState && (0 == count))
      {
         job.status = FNDInternalError;
         Log::error("No attach notification found for poolID: %u", poolID);
      }
      if (0 == job.status)
      {
         if (isDisconnectNotification)
         {
#ifdef USE_SERVER_CACHE
            if (mUseServerCache)
            {
               SServerInfo serverInfo;

               serverInfo.channel = connInfo->channel;
               serverInfo.serverID = connInfo->serverID;
               //before changing pool type update cache information
               if (mMasterNotificationPool[poolPos].getInterfaceDescription())
               {
                  mRemoteServersCache.set(*mMasterNotificationPool[poolPos].getInterfaceDescription(), serverInfo);
               }
               else
               {
                  Log::warning(1, "Can not update cache on the basis of poolId %u",
                               mMasterNotificationPool[poolPos].getPoolID());
               }
            }
#endif
            if (!mAsyncMode)
            {
               addClient(connInfo->clientID, job.getConnectionContext(), connInfo->serverID);
            }
            else
            {
               //in async mode the client is managed only on attach
               job.getClientSpecificData().addClient(connInfo->clientID);
               handleMasterServerAvailable(poolID);
            }
            (void)mServerDisconnectNotifications.move(mServerConnectNotifications, poolID);
            (void)mServerDisconnectNotifications.setType(poolID, connInfo->serverID);
            (void)mMasterNotificationPool[poolPos].setState(connInfo->serverID);
         }
         else
         {
            (void)mMasterNotificationPool[poolPos].setState(NotificationPoolEntry::STATE_CONNECTING);
         }
         if (count)
         {
            Log::message(3, "Moved %u deferred -> %s notifications", count,
                         (isDisconnectNotification) ? "disconnect" : "connect");
         }
      }
   }
   if (jobIter != mPendingAttachExtendedJobs.end())
   {
      //remove pending job data once we have processed the job
      mPendingAttachExtendedJobs.erase(jobIter);
   }
}


void Servicebroker::handleMasterConnected()
{
   Log::message( 1, "opened master: %s", master().address() );

   /* forward the interface data to the master */
   registerInterfacesMaster();

   /* the slave must set a notification at the master */
   forwardServerAvailableNotifications();
}


void Servicebroker::handleMasterDisconnected()
{
   Log::message( 1, "connection to master lost: %s", master().address() );

   mConnectedServers.clearMasterIDs();
   mMasterNotificationPool.clear();
   mPendingRemoteServersRequests.clear();
   mPendingAttachExtendedJobs.clear();

   // May only trigger clients when the server is not visible in the local
   // server table (if so, the server is at the master or another further up node.
   // If the server is visible, it is on any slave node of this servicebroker.
   // Removed triggerNotLocal call here therefore.
   NotificationList::iterator iter = mServerDisconnectNotifications.begin();
   while( iter != mServerDisconnectNotifications.end() )
   {
      if (iter->active
          && mExtendedID != iter->partyID.s.extendedID
          && mConnectedServers.find(iter->partyID) == 0)   // server not here...
      {
         iter->send();
         iter = mServerDisconnectNotifications.erase(iter);
      }
      else
         ++iter ;
   }

   mClientDetachNotifications.triggerNotLocal( mExtendedID );
   (void)mServerConnectNotifications.clearMasterNotification();
#ifdef USE_SERVER_CACHE
   if (mUseServerCache)
   {
      mAttachedClients.removeNotLocal(mExtendedID);
      dropCache();
   }
#endif
}


void Servicebroker::handleMasterServerAvailable( uint32_t poolID)
{
   Log::message( 2, "*PULSE_MASTER_SERVER_AVAILABLE -%d-", poolID );
   mServerConnectNotifications.triggerPool( poolID );
}


void Servicebroker::handleMasterServerDisconnect( uint32_t poolID )
{
   Log::message( 2, "*PULSE_MASTER_SERVER_DISCONNECT -%d-", poolID);
#ifdef USE_SERVER_CACHE
   if (mUseServerCache)
   {
      int32_t pos = mMasterNotificationPool.find(poolID);

      if (pos != -1)
      {
         const SPartyID *serverID = mMasterNotificationPool[pos].getServerID();

         if (serverID)
         {
            dropCache(*serverID);
            mAttachedClients.removeServer(*serverID);
         }
         mServerDisconnectNotifications.triggerPool(poolID);
      }
   } else
   {
      mServerDisconnectNotifications.triggerPool(poolID);
   }
#else
   mServerDisconnectNotifications.triggerPool(poolID);
#endif
}


void Servicebroker::handleMasterClientDetach(notificationid_t notificationId)
{
   Log::message( 2, "*PULSE_MASTER_CLIENT_DETACH -%d-", notificationId);

   Notification *aNotification = mClientDetachNotifications.find(notificationId);
   SPartyID aPartyID;
   if (aNotification)
   {
      aPartyID = aNotification->partyID;
      mClientDetachNotifications.trigger( aPartyID );
   }
   else
   {
      mClientDetachNotifications.trigger( notificationId );
   }
}


void Servicebroker::handleMasterServerlistChange(notificationid_t notificationId)
{
   Log::message( 2, "*PULSE_MASTER_SERVERLIST_CHANGE -%d-", notificationId);
   mServerListChangeNotifications.trigger( notificationId, false );
}


void Servicebroker::handleMasterAttachExtended(uint32_t poolID)
{
   Log::message( 2, "*PULSE_MASTER_ATTACH_EXTENDED -%d-", poolID);
   int32_t pos = mMasterNotificationPool.find(poolID);

   if (-1 == pos)
   {
      Log::error("*PULSE_MASTER_ATTACH_EXTENDED -%d- no pool", poolID);
   }
   else
   {
      if (mMasterNotificationPool[pos].getState() == NotificationPoolEntry::STATE_MONITOR_DISCONNECT)
      {
         handleMasterServerDisconnect(poolID);
      }
      else if (mMasterNotificationPool[pos].getState() == NotificationPoolEntry::STATE_CONNECTING)
      {
         mMasterNotificationPool[pos].setState(NotificationPoolEntry::STATE_CONNECTED);
         if (mAsyncMode)
         {
            //send the second attach extended job to get the server information and set the disconnect notification
            SFNDInterfaceAttachExtendedArg arg;

            memset(&(arg.i), 0, sizeof(arg.i));
            arg.i.ifDescription = *(mMasterNotificationPool[pos].getInterfaceDescription());
            arg.i.sbVersion.majorVersion = DSI_SERVICEBROKER_VERSION_MAJOR ;
            arg.i.sbVersion.minorVersion = DSI_SERVICEBROKER_VERSION_MINOR ;
            arg.i.pulse.pid = mSigAddr.internal.major;  
            arg.i.pulse.chid = mSigAddr.internal.minor; 
            arg.i.pulse.code = PULSE_MASTER_ATTACH_EXTENDED;
            arg.i.pulse.value = poolID;
            Job *job = Job::create(static_cast<int32_t>(pos),
                                   DCMD_FND_ATTACH_INTERFACE_EXTENDED, &arg, sizeof(arg.i),
                                   sizeof(arg.o));
            master().eval(job);
         }
         else
         {
            //do not remove the pool, the pool will migrate to PRECACHING as soon as a client sends the next attach
            //extended
            handleMasterServerAvailable(poolID);
         }
      }
      else
      {
         Log::error("*PULSE_MASTER_ATTACH_EXTENDED pool type %s is not valid",
                    NotificationPoolEntry::toString(mMasterNotificationPool[pos].getState()));
      }
   }
}


void Servicebroker::dropCache(const SPartyID &serverID)
{   
#ifdef USE_SERVER_CACHE
   if (mUseServerCache)
   {
      InterfaceDescription ifDescription;

      for (int i = 0 ; i < mRemoteServersCache.size(); i++)
      {
         if (serverID == mRemoteServersCache[i].getValue().serverID)
         {
            ifDescription = mRemoteServersCache[i].getKey();
            Log::message(3, "Removed from cache entry %s %d.%d, remaining: %u entries", ifDescription.name.c_str(),
                         ifDescription.majorVersion, ifDescription.minorVersion, mRemoteServersCache.size());
            mRemoteServersCache.erase(i);
            break;
         }
      }
   }
#else
   (void)serverID;         
#endif
}


void Servicebroker::dispatch(int code, int value)
{
   switch(code)
   {
   case PULSE_JOB_EXECUTED:
   {
      Job* job = (Job*)value;
      if (job)
      {
         handleJobFinished(*job);
         job->free();
      }
   }
   break;

   case PULSE_MASTER_CONNECTED:
      handleMasterConnected();
      break;

   case PULSE_MASTER_DISCONNECTED:
      handleMasterDisconnected();
      break;

   case PULSE_MASTER_SERVER_AVAILABLE:
      handleMasterServerAvailable(value);
      break;

   case PULSE_MASTER_SERVER_DISCONNECT:
      handleMasterServerDisconnect(value);
      break;

   case PULSE_MASTER_CLIENT_DETACH:
      handleMasterClientDetach(value);
      break;

   case PULSE_MASTER_SERVERLIST_CHANGE:
      handleMasterServerlistChange(value);
      break;

   case PULSE_MASTER_ATTACH_EXTENDED:
      handleMasterAttachExtended(value);
      break;

   default:
      // NOOP
      break;
   }
}


bool Servicebroker::dispatch(dcmd_t cmd, SocketMessageContext& context, ClientSpecificData& data)
{
   bool rc = true;

   if (cmd == DCMD_FND_MASTER_PING)
   {
      /* mark the connection as slave connection */
      Log::message( 5, "%s", GetDCmdString(DCMD_FND_MASTER_PING) );
      const_cast<SocketConnectionContext&>(context.context()).setSlave(true);

      context.prepareResponse(FNDOK, true);
   }
   else
   {
      /* with all receivable elements */
      union
      {
         /* servicebroker version */
         SFNDInterfaceVersion sbVersion;
         /* argument of DCMD_FND_REGISTER_INTERFACE */
         SFNDInterfaceRegisterArg ifRegisterArg;
         /* argument of DCMD_FND_REGISTER_INTERFACE_EX */
         SFNDInterfaceRegisterExArg ifRegisterExArg;
         /* argument of DCMD_FND_REGISTER_INTERFACE_GROUPID */
         SFNDInterfaceRegisterGroupIDArg ifRegisterGroupIDArg;
         /* argument of DCMD_FND_UNREGISTER_INTERFACE */
         SFNDInterfaceUnregisterArg ifUnregisterArg;
         /* argument of DCMD_FND_ATTACH_INTERFACE */
         SFNDInterfaceAttachArg ifAttachArg;
         /* argument of DCMD_FND_ATTACH_INTERFACE_EXTENDED */
         SFNDInterfaceAttachExtendedArg ifAttachExtendedArg;         
         /* argument of DCMD_FND_GET_INTERFACELIST */
         SFNDGetServerInformation getServerInformationArg;
         /* argument of DCMD_FND_DETACH_INTERFACE */
         SFNDInterfaceDetachArg ifDetachArg;
         /* argument of DCMD_FND_NOTIFY_SERVER_DISCONNECT */
         SFNDNotifyServerDisconnectArg notifyServerDisconnectArg;
         /* argument of DCMD_FND_NOTIFY_CLIENT_DETACH */
         SFNDNotifyClientDetachArg notifyClientDetachArg;
         /* argument of DCMD_FND_NOTIFY_SERVER_AVAILABLE */
         SFNDNotifyServerAvailableArg notifyServerAvailableArg;
         /* argument of DCMD_FND_NOTIFY_SERVER_AVAILABLE_EX */
         SFNDNotifyServerAvailableExArg notifyServerAvailableExArg;
         /* argument of DCMD_FND_CLEAR_NOTIFICATION */
         SFNDClearNotificationArg clearNotificationArg;
         /* argument of DCMD_FND_REGISTER_MASTER_INTERFACE_EX */
         SFNDInterfaceRegisterMasterExArg ifRegisterMasterExArg ;
         /* argument of DCMD_FND_GET_INTERFACELIST */
         SFNDGetInterfaceListArg ifGetInterfaceListArg ;
         /* argument of DCMD_FND_NOTIFY_INTERFACELIST_CHANGE */
         SFNDNotifyInterfaceListChangeArg notifyInterfaceListChangeArg ;
         /* argument of DCMD_FND_MATCH_INTERFACELIST */
         SFNDMatchInterfaceListArg ifMatchInterfaceListArg ;
         /* argument of DCMD_FND_NOTIFY_INTERFACELIST_MATCH */
         SFNDNotifyInterfaceListMatchArg notifyInterfaceListMatchArg ;
         /* argument of DCMD_FND_MASTER_PING_ID */
         SFNDMasterPingIdArg masterPingIdArg;                  
      } rcvBuffer;

      switch(cmd)
      {
      case DCMD_FND_REGISTER_INTERFACE:          /* fall-through */
      case DCMD_FND_REGISTER_INTERFACE_EX:       /* fall-through */
      case DCMD_FND_REGISTER_INTERFACE_GROUPID:  /* fall-through */
      case DCMD_FND_UNREGISTER_INTERFACE:        /* fall-through */
      case DCMD_FND_ATTACH_INTERFACE:            /* fall-through */
      case DCMD_FND_ATTACH_INTERFACE_EXTENDED:   /* fall-through */      
      case DCMD_FND_GET_SERVER_INFORMATION:      /* fall-through */
      case DCMD_FND_DETACH_INTERFACE:            /* fall-through */
      case DCMD_FND_NOTIFY_SERVER_DISCONNECT:    /* fall-through */
      case DCMD_FND_NOTIFY_SERVER_AVAILABLE:     /* fall-through */
      case DCMD_FND_NOTIFY_SERVER_AVAILABLE_EX:  /* fall-through */
      case DCMD_FND_NOTIFY_CLIENT_DETACH:        /* fall-through */
      case DCMD_FND_CLEAR_NOTIFICATION:          /* fall-through */
      case DCMD_FND_REGISTER_MASTER_INTERFACE_EX:/* fall-through */
      case DCMD_FND_GET_INTERFACELIST:           /* fall-through */
      case DCMD_FND_NOTIFY_INTERFACELIST_CHANGE: /* fall-through */
      case DCMD_FND_MATCH_INTERFACELIST:         /* fall-through */
      case DCMD_FND_NOTIFY_INTERFACELIST_MATCH:  /* fall-through */
      case DCMD_FND_MASTER_PING_ID:                    
         break;

      default:
         rc = false;
         break;
      }

      if (rc)
      {
         /* read message data */
         int32_t nBytes = context.readData(&rcvBuffer, sizeof(rcvBuffer));

         /* test for successful read operation */
         if (nBytes >= 0)
         {
            if((size_t)nBytes >= sizeof(rcvBuffer.sbVersion) && Servicebroker::check(rcvBuffer.sbVersion))
            {
               /* evaluate command */
               switch( cmd )
               {
                  /* Register an interface */
               case DCMD_FND_REGISTER_INTERFACE:
                  if( nBytes == sizeof(rcvBuffer.ifRegisterArg) )    /* test for expected message length */
                  {
                     handleRegisterInterface( context, data, rcvBuffer.ifRegisterArg );
                  }
                  break;

               case DCMD_FND_REGISTER_INTERFACE_EX:
                  if( nBytes > (int32_t)sizeof(rcvBuffer.ifRegisterExArg) )
                  {
                     context.setGetOffset(sizeof(rcvBuffer.ifRegisterExArg.i));
                     handleRegisterInterfaceEx( context, data, rcvBuffer.ifRegisterExArg );
                  }
                  break;

               case DCMD_FND_REGISTER_INTERFACE_GROUPID:
                  if( nBytes == (int32_t)sizeof(rcvBuffer.ifRegisterGroupIDArg) )
                  {
                     handleRegisterInterfaceGroupID( context, data, rcvBuffer.ifRegisterGroupIDArg );
                  }
                  break;

                  /* Unregister an interface */
               case DCMD_FND_UNREGISTER_INTERFACE:
                  if( nBytes == sizeof(rcvBuffer.ifUnregisterArg))
                  {
                     handleUnregisterInterface( context, data, rcvBuffer.ifUnregisterArg );
                  }
                  break;

                  /* Attach an interface */
               case DCMD_FND_ATTACH_INTERFACE:
                  if( nBytes == sizeof(rcvBuffer.ifAttachArg))
                  {
                     handleAttachInterface( context, data, rcvBuffer.ifAttachArg );
                  }
                  break;

                  /* Attach an interface */
               case DCMD_FND_ATTACH_INTERFACE_EXTENDED:
                  if( nBytes == sizeof(rcvBuffer.ifAttachExtendedArg))
                  {
                     handleAttachInterface( context, data, rcvBuffer.ifAttachExtendedArg);
                  }
                  break;
               
                  /* Get server information */
               case DCMD_FND_GET_SERVER_INFORMATION:
                  if( nBytes == sizeof(rcvBuffer.getServerInformationArg))
                  {
                     handleGetServerInformation( context, data, rcvBuffer.getServerInformationArg);
                  }
                  break;

                  /* detach an interface */
               case DCMD_FND_DETACH_INTERFACE:
                  if( nBytes == sizeof(rcvBuffer.ifDetachArg))
                  {
                     handleDetachInterface( context, data, rcvBuffer.ifDetachArg );
                  }
                  break;

                  /* notify for server disconnect */
               case DCMD_FND_NOTIFY_SERVER_DISCONNECT:
                  if( nBytes == sizeof(rcvBuffer.notifyServerDisconnectArg))
                  {
                     handleNotifyServerDisconnect( context, data, rcvBuffer.notifyServerDisconnectArg );
                  }
                  break;
               
                  /* notify for server connect*/
               case DCMD_FND_NOTIFY_SERVER_AVAILABLE:
                  if( nBytes == sizeof(rcvBuffer.notifyServerAvailableArg))
                  {
                     handleNotifyServerAvailable( context, data, rcvBuffer.notifyServerAvailableArg );
                  }
                  break;

                  /* notify for server connect*/
               case DCMD_FND_NOTIFY_SERVER_AVAILABLE_EX:
                  if( nBytes >= (int32_t)sizeof(rcvBuffer.notifyServerAvailableExArg))
                  {
                     context.setGetOffset(sizeof(rcvBuffer.notifyServerAvailableExArg.i));
                     handleNotifyServerAvailable( context, data, rcvBuffer.notifyServerAvailableExArg );
                  }
                  break;

                  /* notify for client detach */
               case DCMD_FND_NOTIFY_CLIENT_DETACH:
                  if( nBytes == sizeof(rcvBuffer.notifyClientDetachArg))
                  {
                     handleNotifyClientDetach( context, data, rcvBuffer.notifyClientDetachArg );
                  }
                  break;                  

               case DCMD_FND_CLEAR_NOTIFICATION:
                  if( nBytes == sizeof(rcvBuffer.clearNotificationArg))
                  {
                     handleClearNofification( context, data, rcvBuffer.clearNotificationArg);
                  }
                  break;

               case DCMD_FND_REGISTER_MASTER_INTERFACE_EX:
                  if( nBytes >= (int32_t)sizeof(rcvBuffer.ifRegisterMasterExArg) )
                  {
                     context.setGetOffset(sizeof(rcvBuffer.ifRegisterMasterExArg.i));
                     handleRegisterMasterInterface( context, data, rcvBuffer.ifRegisterMasterExArg);
                  }
                  break;

               case DCMD_FND_GET_INTERFACELIST:
                  //if( sizeof(rcvBuffer.ifGetInterfaceListArg.o) == (context.getGetBufferLength() % sizeof(SFNDInterfaceDescription)))
               {
                  //context.setPutOffset(sizeof(rcvBuffer.ifGetInterfaceListArg.o));
                  handleGetInterfaceList( context, data, rcvBuffer.ifGetInterfaceListArg );
               }
               break;

               case DCMD_FND_NOTIFY_INTERFACELIST_CHANGE:
                  if( nBytes == sizeof(rcvBuffer.notifyInterfaceListChangeArg))
                  {
                     handleNotifyInterfaceListChange( context, data, rcvBuffer.notifyInterfaceListChangeArg );
                  }

                  break;

               case DCMD_FND_MATCH_INTERFACELIST:
                  //if( sizeof(rcvBuffer.ifMatchInterfaceListArg.o) == (context.getGetBufferLength() % sizeof(SFNDInterfaceDescription)))
               {
                  //context.setPutOffset(sizeof(rcvBuffer.ifMatchInterfaceListArg.o));
                  handleMatchInterfaceList( context, data, rcvBuffer.ifMatchInterfaceListArg );
               }

               break;

               case DCMD_FND_NOTIFY_INTERFACELIST_MATCH:
                  if( nBytes == sizeof(rcvBuffer.notifyInterfaceListMatchArg))
                  {
                     handleNotifyInterfaceListMatch( context, data, rcvBuffer.notifyInterfaceListMatchArg );
                  }

                  break;

               case DCMD_FND_MASTER_PING_ID:
                  if( nBytes >= static_cast<int32_t>(sizeof(rcvBuffer.masterPingIdArg)))
                  {
                     handleMasterPingId( context, data, rcvBuffer.masterPingIdArg );
                  }

                  break;

               default:
                  assert(false);  // should be handled above...
                  break;
               }
               
               if (!(context.responseState() == MessageContext::State_SENT || context.responseState() == MessageContext::State_DEFERRED))
               {
                  // don't allow this common programming error: either call 'prepareResponse' or 'deferResponse' method on context object
                  // since we may have assertions disabled we need a semi-correct handling of this
                  context.prepareResponse(FNDInternalError, true);
               }
            }
            else
               context.prepareResponse(FNDBadFoundationVersion, true);  
         }
         else
            context.prepareResponse(FNDInternalError, true);   
      }
      else
         context.prepareResponse(FNDInternalError, true);   
   }


   return rc;
}


// ---------------------------------------------------------------------------------------------


const char* GetDCmdString( dcmd_t dcmd )
{
   switch( dcmd )
   {
   case DCMD_FND_REGISTER_INTERFACE:
      return "DCMD_FND_REGISTER_INTERFACE";
   case DCMD_FND_REGISTER_INTERFACE_EX:
      return "DCMD_FND_REGISTER_INTERFACE_EX";
   case DCMD_FND_UNREGISTER_INTERFACE:
      return "DCMD_FND_UNREGISTER_INTERFACE";
   case DCMD_FND_ATTACH_INTERFACE:
      return "DCMD_FND_ATTACH_INTERFACE";
   case DCMD_FND_ATTACH_INTERFACE_EXTENDED:
      return "DCMD_FND_ATTACH_INTERFACE_EXTENDED";   
   case DCMD_FND_DETACH_INTERFACE:
      return "DCMD_FND_DETACH_INTERFACE";
   case DCMD_FND_NOTIFY_SERVER_DISCONNECT:
      return "DCMD_FND_NOTIFY_SERVER_DISCONNECT";   
   case DCMD_FND_NOTIFY_SERVER_AVAILABLE:
      return "DCMD_FND_NOTIFY_SERVER_AVAILABLE";
   case DCMD_FND_NOTIFY_SERVER_AVAILABLE_EX:
      return "DCMD_FND_NOTIFY_SERVER_AVAILABLE_EX";
   case DCMD_FND_NOTIFY_CLIENT_DETACH:
      return "DCMD_FND_NOTIFY_CLIENT_DETACH";
   case DCMD_FND_CLEAR_NOTIFICATION:
      return "DCMD_FND_CLEAR_NOTIFICATION";
   case DCMD_FND_REGISTER_MASTER_INTERFACE_EX:
      return "DCMD_FND_REGISTER_MASTER_INTERFACE_EX";
   case DCMD_FND_MASTER_PING:
      return "DCMD_FND_MASTER_PING";
   case DCMD_FND_GET_INTERFACELIST:
      return "DCMD_FND_GET_INTERFACELIST";
   case DCMD_FND_NOTIFY_INTERFACELIST_CHANGE:
      return "DCMD_FND_NOTIFY_INTERFACELIST_CHANGE";
   case DCMD_FND_NOTIFY_INTERFACELIST_MATCH:
      return "DCMD_FND_NOTIFY_INTERFACELIST_MATCH";
   case DCMD_FND_REGISTER_INTERFACE_GROUPID:
      return "DCMD_FND_REGISTER_INTERFACE_GROUPID";
   case DCMD_FND_GET_SERVER_INFORMATION:
      return "DCMD_FND_GET_SERVER_INFORMATION";
   case DCMD_FND_MASTER_PING_ID:
      return "DCMD_FND_MASTER_PING_ID";
   case DCMD_FND_MATCH_INTERFACELIST:
      return "DCMD_FND_MATCH_INTERFACELIST";   
   default:
      return "<UNKNOWN DCMD>";
   }
}


void Servicebroker::addClient(SPartyID& clientID, const SocketConnectionContext& connCtx, const SPartyID& serverID)
{
   //check if this SB can manage the new client:
   //      in cache mode when this is a connection from a dsi client and not servicebroker
   if (!mUseServerCache || (mUseServerCache && !connCtx.isSlave()))
   {
      // make sure the global ID pair extendedID/mNextClientID is really unique,
      // so don't mix up IDs of different nodes
      if (mConfig.isTreeMode())
      {
         clientID.s.extendedID = mExtendedID;
      }
      else
      {
         clientID.s.extendedID = serverID.s.extendedID;
      }
      clientID.s.localID = mNextClientID++;
      mAttachedClients.push_back(clientID, serverID, connCtx);
   }
   if (clientID.globalID)
   {
      connCtx.getData().addClient(clientID);
   }
}


void Servicebroker::dumpCache()
{
#ifdef USE_SERVER_CACHE
   Log::message(4, "Cached server info:");
   for(int i = 0; i < mRemoteServersCache.size(); i++)
   {
      Log::message(4, "[%d] %s %d.%d -> <%d.%d>, NID: %d, PID: %d, CHID: %d",
                   i,
                   mRemoteServersCache[i].getKey().name.c_str(),
                   mRemoteServersCache[i].getKey().majorVersion, mRemoteServersCache[i].getKey().minorVersion,
                   mRemoteServersCache[i].getValue().serverID.s.extendedID,
                   mRemoteServersCache[i].getValue().serverID.s.localID,
                   mRemoteServersCache[i].getValue().channel.nid,
                   mRemoteServersCache[i].getValue().channel.pid,
                   mRemoteServersCache[i].getValue().channel.chid);
   }
#endif
}


// @note This function can only be called from within a locked servicebroker instance. Otherwise,
//       unexpected behaviour will be the probably result.
bool Servicebroker::findServerInterface(const SPartyID& server, InterfaceDescription &ifDescr)
{
   ServerListEntry* entry = mConnectedServers.find(server);

   if (entry)
   {
      ifDescr = entry->ifDescription;
      return true;
   }
   else
   {
#ifdef USE_SERVER_CACHE   
      for (int i = 0; i < mRemoteServersCache.size(); i++)
      {
         if (mRemoteServersCache[i].getValue().serverID == server)
         {
            ifDescr = mRemoteServersCache[i].getKey();
            return true;
         }
      }
#endif
   }
   return false;
}


const char* Servicebroker::toString(SBStatus status)
{
   switch (status)
   {
   case FNDOK:
      return "FNDOK";

   case FNDVersionMismatch:
      return "FNDVersionMismatch";

   case FNDUnknownInterface:
      return "FNDUnknownInterface";

   case FNDInterfaceAlreadyRegistered:
      return "FNDInterfaceAlreadyRegistered";

   case FNDInvalidServerID:
      return "FNDInvalidServerID";

   case FNDInvalidClientID:
      return "FNDInvalidClientID";

   case FNDInvalidNotificationID:
      return "FNDInvalidNotificationID";

   case FNDBadArgument:
      return "FNDBadArgument";

   case FNDBadFoundationVersion:
      return "FNDBadFoundationVersion";

   case FNDRegularExpression:
      return "FNDRegularExpression";

   case FNDAccessDenied:
      return "FNDAccessDenied";

   case FNDUnknownCommand:
      return "FNDUnknownCommand";

   case FNDInternalError:
      return "FNDInternalError";

   default:
      return "FND???";
   }
}
