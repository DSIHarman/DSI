/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_SERVICEBROKER_HPP
#define DSI_SERVICEBROKER_SERVICEBROKER_HPP

#include "dsi/clientlib.h"

#include "ClientList.hpp"
#include "ServerList.hpp"
#include "NotificationList.hpp"
#include "ConfigFile.hpp"
#include "SignallingAddress.hpp"
#include "MasterAdapter.hpp"
#include "NotificationPool.hpp"

#include <map>


extern const char* GetDCmdString(dcmd_t dcmd);

// forward decls
class SocketMessageContext;
class ClientSpecificData;
class Job;

/**
 * The main servicebroker functional singleton.
 */
class Servicebroker
{
   friend class ClientSpecificData;      ///< must do some cleanup stuff
   friend class NotificationPoolEntry;   ///< must do some cleanup stuff
   friend class Notification;            ///< must do some cleanup stuff

public:

   typedef int(*NodeIdTranslator)(int, int);

   ~Servicebroker();

   /**
    * Version information
    */
   static const int32_t MAJOR_VERSION ;
   static const int32_t MINOR_VERSION ;
   static const int32_t BUILD_VERSION ;
   static const char* BUILD_TIME ;

   /**
    * May return 0 if no such server was found. This function does not set a lock in multi-interface
    * mode. So if called from a non-locked context the lock has to be set manually. The reason is
    * that the critical section locks are not recursive in at least the QNX implementation.
    */
   bool findServerInterface(const SPartyID& server, InterfaceDescription &ifDescr);

   static Servicebroker& getInstance();

   /**
    * @param master 0 if this servicebroker is already the master.
    // FIXME remove NodeIdTranslator - whats this under Linux, does it have any effect???!!!
    */
   void init(MasterAdapter* master, NodeIdTranslator translator, const char* configfile = nullptr,
             int id = 0, bool serverCache = false, bool asyncMode = false);

   void setSignallingAddress(const SignallingAddress& addr);

   /**
    * handle pulse like commands
    */
   void dispatch(int code, int value);

   /**
    * handle devctl like commands
    * @return true if the command was handled, else false.
    */
   bool dispatch(dcmd_t cmd, SocketMessageContext& context, ClientSpecificData& data);

   void dumpStats(std::ostringstream& stream);

   void dumpServers(std::ostringstream& ostream) const;
   void dumpClients(std::ostringstream& ostream) const;
   void dumpServerConnectNotifications(std::ostringstream& ostream) const;
   void dumpServerDisconnectNotifications(std::ostringstream& ostream) const;
   void dumpClientDetachNotifications(std::ostringstream& ostream) const;
   void dumpStatistics(std::ostringstream& ostream) const;
   /**
    * Dumps the current server cache.
    */
   void dumpCache();

   /**
    * @return the extended id of this servicebroker instance.
    */
   inline
   int32_t id() const
   {
      return mExtendedID;
   }

   /**
    * @internal
    *
    * @brief Verifies the foundation version.
    *
    * @param major The major foundation version number passed by the client.
    * @param minor The minor foundation version number passed by the client.
    *
    * @return '0' if the foundation version passed by the client is compatible  the
    *         with the foundation version implemented by this serverbroker. Any other
    *         value indicates a version mismatch.
    */
   static bool check( const SFNDInterfaceVersion &version );

   /**
    * @internal
    *
    * @brief Tests for a valid interface name.
    *
    * @param name Pointer to the null terminated interface name.
    *        This argument must not be NULL.
    *
    * @return true if the passed name is a valid interface name.
    *         Any other value indicates an error.
    */
   static bool check( const SFNDInterfaceDescription &ifDescription );

   /**
    * Helper method to print out the Servicebroker internal error codes.
    */
   static const char* toString(SBStatus status);

private:
   
   //typedef SFNDNotifyServerDisconnectArg tNotifyServerDisconnectCommonArg;
   //typedef SFNDNotifyClientDetachArg tNotifyClientDetachCommonArg;
   //typedef SFNDInterfaceAttachExtendedArg tAttachInterfaceExtendedCommonArg;

   /** Structure used to keep extra information for the attach extended jobs, especially for the sub-channel version. */
   typedef struct
   {
      notificationid_t mainNotificationID;      
   } tNotificationsGroup;

   /** it's a singleton */
   Servicebroker();

   /** The connected servers. */
   ServerList mConnectedServers;

#ifdef USE_SERVER_CACHE
   /** The remote connected servers known so far from the master SB, this is a cache. */
   std::map<InterfaceDescription, SServerInfo> mRemoteServersCache;
#endif

   /** In case no information regarding a server is found on the local node the request is forwarded to the master
    * node. In this case, the requests are stored until the response is get back.
    */
   std::map<uint32_t, InterfaceDescription> mPendingRemoteServersRequests;

   /** The attached clients. */
   ClientList mAttachedClients;

   /** Contains all active server disconnect notifications. */
   NotificationList mServerDisconnectNotifications;

   /** Contains all active client detach notifications. */
   NotificationList mClientDetachNotifications;

   /** Contains all active server connect notifications. */
   NotificationList mServerConnectNotifications;

   /** Contains all serverlist change notifications */
   NotificationList mServerListChangeNotifications;

   /** List of notifications set at the master servicebroker */
   NotificationPool mMasterNotificationPool ;

   /** Map with information for the attach extended standard + sub-channels pending jobs.*/
   std::map<uint32_t, tNotificationsGroup> mPendingAttachExtendedJobs;

   /** Next free client ID. */
   uint32_t mNextClientID;

   /** Next free server ID. */
   uint32_t mNextServerID ;

   /**
    * The next free request ID.
    */
   uint32_t mNextPendingRemoteServerRequestID;

   /** the extended ID of this servicebroker */
   uint32_t mExtendedID ;

   /** this vector contains the services that are only locally visible */
   ConfigFile mConfig ;

   /** true if server cache can be used. */
   bool mUseServerCache;

   /** true if the asynchronous mode is enabled, clients are unblocked as soon as possible */
   bool mAsyncMode;

private:

   void forwardServerAvailableNotifications();
   void registerInterfacesMaster();
   void unregisterInterfaceMaster( ServerListEntry& entry );
   void createClearNotificationJob( notificationid_t notificationID );
   bool addInterface( ClientSpecificData &ocb, const InterfaceDescription&, const SocketConnectionContext&, int32_t pid, int32_t chid, bool isLocal, int32_t grpid, SPartyID& serverID );
   bool checkAccessPrivileges(const SocketConnectionContext& context, ServerListEntry* entry, uint32_t uid = SB_UNKNOWN_USER_ID );

   /**
    * Creates a unique client ID.
    * @arg clientID the new client ID
    * @arg serverID the server to which the client is connected
    */
   void addClient(SPartyID& clientID, const SocketConnectionContext& connCtx, const SPartyID& serverID);

   /**
    * Drop the cache for a serverID.
    */
   void dropCache(const SPartyID &server);

   /**
    * Drop the whole cache.
    */
   void dropCache();

private:

   // all request handler functions
   void handleRegisterMasterInterface( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceRegisterMasterExArg &arg );
   void handleRegisterInterface( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceRegisterArg &arg );
   void handleRegisterInterfaceEx( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceRegisterExArg &arg );
   void handleRegisterInterfaceGroupID( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceRegisterGroupIDArg &arg );
   void handleUnregisterInterface( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceUnregisterArg &arg );
   void handleAttachInterface( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceAttachArg &arg );   
   void handleAttachInterface( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceAttachExtendedArg &arg) ;
   SBStatus forwardAttachExtendedToMaster(SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceAttachExtendedArg &arg);   
   void handleGetServerInformation( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDGetServerInformation &arg );
   SBStatus handleAttachLocalInterface( SocketMessageContext &msg, ClientSpecificData &ocb,
                                        SFNDInterfaceAttachArg &arg, ServerListEntry* entry);
   void handleDetachInterface( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDInterfaceDetachArg &arg );
   void handleNotifyServerDisconnect( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDNotifyServerDisconnectArg &arg );
   SBStatus handleNotifyServerDisconnectNoResponse( SocketMessageContext &msg, ClientSpecificData &ocb,
                                                    SFNDNotifyServerDisconnectArg &arg);   
   void handleNotifyServerAvailable( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDNotifyServerAvailableArg &arg );
   SBStatus handleNotifyServerAvailableNoResponse( SocketMessageContext &msg, ClientSpecificData &ocb,
                                                   SFNDNotifyServerAvailableArg &arg);
   void handleNotifyServerAvailable( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDNotifyServerAvailableExArg &arg );   
   void handleNotifyClientDetach( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDNotifyClientDetachArg &arg);
   void handleClearNofification( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDClearNotificationArg &arg );
   void handleGetInterfaceList( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDGetInterfaceListArg &arg );
   void handleNotifyInterfaceListChange( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDNotifyInterfaceListChangeArg &arg );
   void handleMatchInterfaceList( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDMatchInterfaceListArg &arg );
   void handleNotifyInterfaceListMatch( SocketMessageContext &msg, ClientSpecificData &ocb, SFNDNotifyInterfaceListMatchArg &arg );
   void handleMasterPingId(SocketMessageContext &msg, ClientSpecificData &ocb, SFNDMasterPingIdArg &arg );

   void handleJobFinished(Job& job);
   void handleJobFinishedAttachExtended(SFNDInterfaceAttachExtendedArg &arg, Job &job);

   void handleMasterConnected();
   void handleMasterDisconnected();

   void handleMasterServerAvailable(uint32_t poolID);
   void handleMasterServerDisconnect(uint32_t poolID);
   void handleMasterClientDetach(uint32_t poolID);
   void handleMasterServerlistChange(uint32_t poolID);
   void handleMasterAttachExtended(uint32_t poolID);

private:

   /**
    * @internal
    * @brief checks if this servicebroker is a master of a slave
    * @return true if it is a master, otherwise false
    */
   inline
   bool isMaster() const
   {
      return mAster == nullptr;
   }

   inline
   bool isMasterConnected() const
   {
      return mAster && mAster->isConnected();
   }

   inline
   MasterAdapter& master() const
   {
      assert(mAster);
      return *mAster;
   }


private:

   MasterAdapter* mAster;
   SignallingAddress mSigAddr;               ///< signalling address for pulses from master to slave
   NodeIdTranslator mTranslateNodeId;        
};


// ------------------------------------------------------------------------------------------------


inline 
void Servicebroker::dumpServers(std::ostringstream& ostream) const
{
   mConnectedServers.dumpStats( ostream );
}


inline 
void Servicebroker::dumpClients(std::ostringstream& ostream) const
{
   mAttachedClients.dumpStats( ostream );
}


inline 
void Servicebroker::dumpServerConnectNotifications(std::ostringstream& ostream) const
{
   mServerConnectNotifications.dumpStats( ostream );
}


inline 
void Servicebroker::dumpServerDisconnectNotifications(std::ostringstream& ostream) const
{
   mServerDisconnectNotifications.dumpStats( ostream );
}


inline 
void Servicebroker::dumpClientDetachNotifications(std::ostringstream& ostream) const
{
   mClientDetachNotifications.dumpStats( ostream );
}


inline 
void Servicebroker::dropCache()
{
#ifdef USE_SERVER_CACHE
   mRemoteServersCache.clear();
#endif   
}

#endif // DSI_SERVICEBROKER_SERVICEBROKER_HPP
