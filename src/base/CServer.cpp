/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/CServer.hpp"
#include "dsi/CIStream.hpp"
#include "dsi/COStream.hpp"
#include "dsi/CCommEngine.hpp"
#include "dsi/Log.hpp"
#include "dsi/private/util.hpp"

#include "CTraceManager.hpp"
#include "CServicebroker.hpp"
#include "CDummyChannel.hpp"
#include "io.hpp"
#include "CTCPChannel.hpp"
#include "CConnectRequestHandle.hpp"
#include "DSI.hpp"

#include <cstdio>
#include <algorithm>
#include <cstdlib>
#include <errno.h>
#include <cstring>
#include <tr1/functional>

// check if server implementor has sent an error or response
#define isResponseDangling(id) (DSI::INVALID_ID != id && getResponseState(id))


using namespace std::tr1::placeholders;


TRC_SCOPE_DEF( dsi_base, CServer, global );
TRC_SCOPE_DEF( dsi_base, CServer, handleDataRequest );
TRC_SCOPE_DEF( dsi_base, CServer, sendNotification );
TRC_SCOPE_DEF( dsi_base, CServer, removeNotification );


namespace DSI
{
   extern bool isTCPForced();
}


DSI::CServer::ClientConnection::ClientConnection()
   : id(0)
   , notificationID(0)
   , channel(CDummyChannel::getInstancePtr())
{
   // NOOP
}


// --------------------------------------------------------------------------------------------


namespace /* anonymous */
{

class SessionHandleComparator
{
public:

   explicit inline
   SessionHandleComparator(int32_t sessionid)
      : mSessionid(sessionid)
   {
      // NOOP
   }

   /// The 'template' function is a workaround in order to avoid public/private clashes.
   /// The functor will always be called for DSI::CServer::Notification.
   template<typename T>
   inline
   bool operator()(const T& n)
   {
      return n.sessionId == mSessionid;
   }

   int32_t mSessionid;
};

}   // namespace


// --------------------------------------------------------------------------------------------------------


DSI::CServer::CServer( const char* ifname, const char* rolename, int majorVersion, int minorVersion
                     , bool enableTCPIP )
   : CBase( ifname, rolename, majorVersion, minorVersion )
   , mSessionId( DSI::INVALID_SESSION_ID )
   , mResponseId( DSI::INVALID_ID )
   , d(nullptr)
   , mTCPIPEnabled( enableTCPIP || isTCPForced() )
{
   TRC_SCOPE( dsi_base, CServer, global );
   DBG_MSG(("DSI::CServer::CServer() %s %d.%d", mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion ));

   mTCPServerID.globalID = 0;
}


DSI::CServer::~CServer()
{
   if (mCommEngine)
   {
      (void)mCommEngine->remove(*this);
   }
}


int DSI::CServer::unblockRequest()
{
   int handle = INVALID_SESSION_ID;

   // MRIES1 only enable this call within response handlers :
   // MR: this is done with (DSI::INVALID_ID != mResponseId)
   // and only allow  them for requests with correlated responses
   // MR: is also done with (DSI::INVALID_ID != mResponseId)
   assert(mCurrentSequenceNr != DSI::INVALID_SEQUENCE_NR);

   if (DSI::INVALID_ID != mResponseId)
   {
      for( int idx=(int)mNotifications.size()-1; idx>=0; idx-- )
      {
         if( mNotifications[idx].notifyID == mResponseId
             && mNotifications[idx].sessionId == DSI::INVALID_SESSION_ID
             && mNotifications[idx].clientID == mClientID )
         {
            handle = DSI::createId();
            mUnblockedSessions[handle] = mNotifications[idx] ;

            mNotifications.erase( mNotifications.begin() + idx );
            break;
         }
      }

      setResponseState( mResponseId, false );
      mResponseId = DSI::INVALID_ID;
      mCurrentSequenceNr = DSI::INVALID_SEQUENCE_NR;
   }

   return handle;
}


void DSI::CServer::prepareResponse(int handle)
{
   //There must not be any pending current response otherwise you will run into an assertion.
   assert(DSI::INVALID_ID == mResponseId);

   unblockedsessionsmap_type::iterator iter = mUnblockedSessions.find( handle );

   if( iter != mUnblockedSessions.end() )
   {
      Notification n = iter->second;
      mUnblockedSessions.erase( iter );

      mResponseId = n.notifyID ;
      mClientID = n.clientID;
      mNotifications.push_back( n );
      setResponseState( n.notifyID, true );
   }
}


void DSI::CServer::registerInterface( int32_t channel )
{
   TRC_SCOPE( dsi_base, CServer, global );
   DBG_MSG(("DSI::CServer::registerInterface() %s %d.%d", mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion ));

   if (CServicebroker::registerInterface( *this, channel, mUserGroup ))
   {
      CTraceManager::add(mServerID, mIfDescription);
   }
   else
   {
      DBG_ERROR(( "Error registering %s %d.%d", mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion ));
   }
}


void DSI::CServer::registerInterfaceTCP( uint32_t address, int32_t port )
{
   TRC_SCOPE( dsi_base, CServer, global );
   DBG_MSG(("DSI::CServer::registerInterfaceTCP() %s %d.%d, enabled = %s", mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion, mTCPIPEnabled ? "true" : "false" ));

   if( mTCPIPEnabled && port > 0 )
   {
      if (CServicebroker::registerInterfaceTCP( *this, address, port ))
      {
         CTraceManager::add(mTCPServerID, mIfDescription);
      }
      else
      {
         DBG_ERROR(( "Error registering %s %d.%d", mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion ));
      }
   }
}


void DSI::CServer::cleanupClientConnection(const CServer::ClientConnection& c)
{
   // clear the client detach notification first
   CServicebroker::clearNotification( c.notificationID );

   // remove all client notifications
   removeNotification( c.clientID );
}


void DSI::CServer::unregisterInterface()
{
   TRC_SCOPE( dsi_base, CServer, global );
   DBG_MSG(("DSI::CServer::unregisterInterface() %s %d.%d", mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion ));

   std::for_each(mClientConnections.begin(), mClientConnections.end(), std::tr1::bind(&DSI::CServer::cleanupClientConnection, this, _1));
   mClientConnections.clear();

   if (mServerID)
   {
      CTraceManager::remove(mServerID);
      CServicebroker::unregisterInterface(mServerID);
      mServerID = 0;
   }

   if (mTCPServerID)
   {
      CTraceManager::remove(mTCPServerID);
      CServicebroker::unregisterInterface(mTCPServerID);
      mTCPServerID = 0;
   }
}


void DSI::CServer::handleDataRequest( Private::CDataRequestHandle &handle )
{
   TRC_SCOPE( dsi_base, CServer, handleDataRequest );
   DBG_MSG(( "DSI::CServer::handleDataRequest() %s %d.%d  %s - %s (0x%08X), seq:%d"
             , mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion
             , DSI::toString( handle.getRequestType() )
             , getUpdateIDString(handle.getRequestId()), handle.getRequestId()
             , handle.getSequenceNumber()));
   ClientConnection* conn = findClientConnection(handle.getClientID());
   uint16_t isProtoMinor = conn ? conn->protoMinor : (uint16_t)DSI_PROTOCOL_VERSION_MINOR;
   if (isProtoMinor != handle.getProtoMinor())
   {
      DBG_ERROR(("handleDataRequest: bad protocol version (expected: %d.%d, received: %d.%d)"
                 , DSI_PROTOCOL_VERSION_MAJOR, isProtoMinor
                 , DSI_PROTOCOL_VERSION_MAJOR, handle.getProtoMinor() ));
      return;
   }



   switch(handle.getRequestType())
   {
      case DSI::REQUEST:
      {
         uint32_t responseId = getResponse(handle.getRequestId());

         if (isResponseDangling(responseId))
         {
            // no response has been send and client was not unblocked
            // so tell the client he should come to a later point in time and try again
            sendErrorToClient(handle.getClientID(), handle.getRequestId(),
                              DSI::RESULT_REQUEST_BUSY, handle.getSequenceNumber());
         }
         else
         {
            mResponseId = responseId;

            // add a notification for the corresponding response - if any
            if (DSI::INVALID_ID != mResponseId)
            {
               Notification n;
               n.clientID = handle.getClientID();
               n.notifyID = mResponseId ;
               n.sequenceNr = handle.getSequenceNumber() ;
               mNotifications.push_back(n);
               setResponseState(mResponseId, true);
            }

            mClientID = handle.getClientID();
            mCurrentSequenceNr = handle.getSequenceNumber() ;

            processRequest(handle);

            if (isResponseDangling(mResponseId))
            {
               DBG_WARNING(( "DSI::CServer::handleDataRequest() %s %s - %s (0x%08X), seq:%d: response is dangling, maybe you better use unblockRequest()"
                , mIfDescription.name
                , getUpdateIDString(handle.getRequestId()), handle.getRequestId()
                , handle.getSequenceNumber()));
            }

            mResponseId = DSI::INVALID_ID;
            mCurrentSequenceNr = DSI::INVALID_SEQUENCE_NR;
            mClientID = 0;
         }
      }
      break;

      case DSI::REQUEST_NOTIFY:
      case DSI::REQUEST_REGISTER_NOTIFY:
      {
         // a cliern set a notification on an attribute
         uint32_t requestId = handle.getRequestId() ;
         SPartyID clientID = handle.getClientID() ;

         // first we check if the notification already exists
         bool found = false ;
         for( int idx=0; !found && idx<(int)mNotifications.size(); idx++ )
         {
            found = mNotifications[idx].clientID == clientID
               && mNotifications[idx].notifyID == requestId
               && (DSI::REQUEST_NOTIFY == handle.getRequestType() || (mNotifications[idx].sequenceNr == handle.getSequenceNumber()));
         }

         if( !found )
         {
            // It's a new notification -> add it
            Notification n ;
            n.clientID = clientID;
            n.notifyID = requestId;
            if(DSI::REQUEST_REGISTER_NOTIFY == handle.getRequestType())
            {
               // client side session id
               n.sequenceNr = handle.getSequenceNumber();

               // server side session id
               SessionData* session = findSession(n.sequenceNr, clientID);

               if (session)
               {
                  n.sessionId = session->sessionId;   
               }
               else
               {
                  SessionData sd ;
                  sd.sessionId = n.sessionId = DSI::createId();
                  sd.clientID = clientID ;
                  sd.sequenceNr = handle.getSequenceNumber();
                  mSessions.push_back( sd );

               }
            }
            mNotifications.push_back( n );
         }

         if (DSI::isAttributeId(requestId))
         {
            if (DSI::DATA_OK == getAttributeState(requestId))
            {
               // Send the attribute back to the client right away.
               ClientConnection* conn = findClientConnection(clientID);
               if (conn && !conn->channel.expired())
               {
                  DBG_MSG(( "DSI::CServer::sendNotification() c:<%d.%d> %s (0x%08X) DATA_OK"
                          , conn->clientID.s.extendedID, conn->clientID.s.localID
                          , getUpdateIDString(requestId), requestId ));

                  CRequestWriter writer(*conn->channel.lock()
                                       , DSI::RESULT_DATA_OK
                                       , DSI::DataResponse
                                       , requestId
                                       , DSI::INVALID_SEQUENCE_NR
                                       , conn->clientID
                                       , conn->serverID
                                       , conn->protoMinor);
                  COStream ostream(writer);

                  writeAttribute(requestId, ostream, DSI::UPDATE_COMPLETE, -1, -1);
                  (void)writer.flush();   // FIXME should handle return code here
               }
               else
               {
                  assert(!conn->channel.expired());
               }
            }
            else if (DSI::DATA_INVALID == getAttributeState(requestId))
            {
               sendNotification((uint32_t)requestId);
            }
         }
      }
      break;

      case DSI::REQUEST_STOP_NOTIFY:
      case DSI::REQUEST_STOP_REGISTER_NOTIFY:
         removeNotification( handle.getClientID(), handle.getRequestId() );
         break;

      case DSI::REQUEST_STOP_ALL_NOTIFY:
         removeNotification( handle.getClientID() );
         break;

      case DSI::REQUEST_STOP_ALL_REGISTER_NOTIFY:
      {
         // this is currently never sent by a normal client, just MoCCA clients send this
         SessionData* session = findSession( handle.getSequenceNumber(), handle.getClientID() );
         if (session)
         {
            removeSessionNotifications(session->sessionId);
         }
      }
      break;

      default:
         break;
   }
}


DSI::CServer::SessionData* DSI::CServer::findSession(int32_t seqNr, const SPartyID &clientID)
{
   SessionData* session = nullptr;

   if (seqNr != DSI::INVALID_SEQUENCE_NR)
   {
      for(sessionlist_type::reverse_iterator riter = mSessions.rbegin(); riter != mSessions.rend(); ++riter)
      {
         if( riter->clientID == clientID && riter->sequenceNr == seqNr )
         {
            session = &*riter;
            break;
         }
      }
   }

   return session;
}


void DSI::CServer::removeSession( int32_t sessionId )
{
   removeSessionNotifications( sessionId );

   for(sessionlist_type::iterator iter = mSessions.begin(); iter != mSessions.end(); ++iter)
   {
      if( iter->sessionId == sessionId )
      {
         mSessions.erase(iter);
         break;
      }
   }
}


void DSI::CServer::removeSessionNotifications( int32_t sessionId )
{
   std::remove_if(mNotifications.begin(), mNotifications.end(), SessionHandleComparator(sessionId));
}


void DSI::CServer::addActiveSession( int32_t sessionId )
{
   mActiveSessions.insert(sessionId);
}


void DSI::CServer::clearActiveSessions()
{
   mActiveSessions.clear();
}


bool DSI::CServer::isSessionActive(int32_t sessionId)
{
   return mActiveSessions.find(sessionId) != mActiveSessions.end();
}


void DSI::CServer::sendNotification( uint32_t id, DSI::UpdateType type, int16_t position, int16_t count )
{
   TRC_SCOPE( dsi_base, CServer, sendNotification );
   for( int idx=0; idx<(int)mNotifications.size(); idx++ )
   {
      if( mNotifications[idx].notifyID == id )
      {
         ClientConnection* conn = findClientConnection(mNotifications[idx].clientID);
         if (conn && !conn->channel.expired())
         {
            DSI::ResultType rtyp = DSI::DATA_OK == getAttributeState( id ) ? DSI::RESULT_DATA_OK : DSI::RESULT_DATA_INVALID ;

            DBG_MSG(( "DSI::CServer::sendNotification() c:<%d.%d> %s (0x%08X) %s"
                              , conn->clientID.s.extendedID, conn->clientID.s.localID
                              , getUpdateIDString(id), id
                              , DSI::toString( rtyp )));

            CRequestWriter writer( *conn->channel.lock()
                                 , rtyp
                                 , DSI::DataResponse
                                 , id
                                 , DSI::INVALID_SEQUENCE_NR
                                 , conn->clientID
                                 , conn->serverID
                                 , conn->protoMinor) ;

            if( rtyp == DSI::RESULT_DATA_OK )
            {
               COStream ostream(writer);
               writeAttribute(id, ostream, type, position, count);
            }

            (void)writer.flush();   // FIXME should handle return value here
         }
         else
         {
            assert(!conn->channel.expired());
         }
      }
   }
}

void DSI::CServer::sendResponse(uint32_t responseId, uint32_t id, DSI::ResultType typ, uint32_t* err)
{

   for( int32_t idx=mNotifications.size()-1; idx>=0; idx-- )
   {
      if(  mNotifications[idx].notifyID == (uint32_t) responseId
        && (mNotifications[idx].sessionId == DSI::INVALID_SESSION_ID
        || isSessionActive( mNotifications[idx].sessionId )))
      {
         ClientConnection* conn = findClientConnection(mNotifications[idx].clientID);
         if (conn && !conn->channel.expired())
         {
            CRequestWriter writer( *conn->channel.lock()
                                 , typ
                                 , DSI::DataResponse
                                 , id
                                 , mNotifications[idx].sequenceNr
                                 , mNotifications[idx].clientID
                                 , conn->serverID
                                 , conn->protoMinor);
            if (err)
            {
               COStream ostream(writer);
               ostream << *err;
            }

            (void)writer.flush();

            if( DSI::INVALID_SEQUENCE_NR != mNotifications[idx].sequenceNr
             && DSI::INVALID_SESSION_ID == mNotifications[idx].sessionId )
            {
               // one shot notification
               mNotifications.erase( mNotifications.begin() + idx );
            }
         }
         else
         {
            assert(!conn->channel.expired());
         }
      }
   }
   if (mResponseId == responseId)
   {
      mResponseId = DSI::INVALID_ID;
   }

}

DSI::CServer::ClientConnection* DSI::CServer::findClientConnection(const SPartyID& clientID)
{
   ClientConnection* rc = nullptr;

   clientconnectionlist_type::iterator iter = mClientConnections.find(ClientConnection(clientID));
   if (iter != mClientConnections.end())
   {
      rc = const_cast<CServer::ClientConnection*>(&*iter);      // it's safe here
   }
   else
   {
      // the client is not connected. here we make sure that
      // there is no notification pending to the client that
      // does not exist.
      removeNotification( clientID );
   }

   return rc;
}


void DSI::CServer::removeNotification( const SPartyID &clientID, uint32_t id )
{
   TRC_SCOPE( dsi_base, CServer, removeNotification );

   for( int idx=(int)mNotifications.size()-1; idx>=0; idx-- )
   {
      if(( mNotifications[idx].clientID == clientID )
         && ( DSI::INVALID_ID == id || mNotifications[idx].notifyID == id ))
      {
         DBG_MSG(("DSI::CServer::removeNotification() %s %d.%d - c:<%d.%d> %s"
                  , mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion
                  , clientID.s.extendedID, clientID.s.localID
                  , getUpdateIDString(mNotifications[idx].notifyID)));

         mNotifications.erase( mNotifications.begin() + idx );
      }
   }
}


void DSI::CServer::handleConnectRequest(CConnectRequestHandle &handle)
{
   TRC_SCOPE( dsi_base, CServer, global );
   DBG_MSG(("DSI::CServer::handleConnectRequest() %s %d.%d - s:<%d.%d> c:<%d.%d>"
            , mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion
            , handle.getServerID().s.extendedID, handle.getServerID().s.localID
            , handle.getClientID().s.extendedID, handle.getClientID().s.localID ));

   DSI::ConnectRequestInfo rci = handle.info();
   ClientConnection conn;

   // do not reuse the given socket connection since it is temporary only!
   conn.channel = mCommEngine->attach(handle.info().pid, handle.info().channel);
   uint16_t isprotoMinor = (uint16_t)DSI_PROTOCOL_VERSION_MINOR;
   uint16_t protoMinor = (handle.getProtoMinor() < isprotoMinor) ? handle.getProtoMinor() : isprotoMinor;

   conn.protoMinor = protoMinor;
   conn.id = DSI::createId() ;
   conn.clientID = handle.getClientID();
   conn.serverID = handle.getServerID();
   conn.notificationID = CServicebroker::setClientDetachNotification( handle.getClientID(), mCommEngine->getNotificationSocketChid(), conn.id );

   // add the connection to the client connection list
   mClientConnections.insert(conn);

   // send back the reply to the client - with this data he will not reconnect anything
   rci.pid = ::getpid();
   rci.channel = mCommEngine->getLocalChannel();

   DSI::MessageHeader msg(handle.getServerID(), handle.getClientID(), DSI::ConnectResponse, conn.protoMinor, sizeof(rci));

   iov_t iov[2] = {
      { &msg, sizeof(msg) },
      { &rci, sizeof(rci) }
   };

   if (!handle.getChannel()->sendAll(iov, 2))
   {
      DBG_ERROR(("Error sending ConnectRequest reply back to client" ));
   }
}


void DSI::CServer::handleConnectRequestTCP(CTCPConnectRequestHandle &handle)
{
   TRC_SCOPE( dsi_base, CServer, global );
   DBG_MSG(("DSI::CServer::handleConnectRequestTCP() %s %d.%d - s:<%d.%d> c:<%d.%d>"
            , mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion
            , handle.getServerID().s.extendedID, handle.getServerID().s.localID
            , handle.getClientID().s.extendedID, handle.getClientID().s.localID ));

   IPv4::Endpoint ep = handle.getTCPChannel().getPeerName();

   char buf[32];
   DBG_MSG(("Peer Address %s", ep.toString(buf)));

   DSI::TCPConnectRequestInfo rci = handle.info();
   ClientConnection conn;

   if( !mTCPIPEnabled )
   {
      DBG_ERROR(("TCP/IP not available" ));
      rci.ipAddress = rci.port = 0 ;
   }
   else
   {
      // do not reuse the given socket connection since it is temporary only!
      conn.channel = mCommEngine->attachTCP(handle.info().ipAddress, handle.info().port);
      uint16_t isprotoMinor = (uint16_t)DSI_PROTOCOL_VERSION_MINOR;
      uint16_t protoMinor = (handle.getProtoMinor() < isprotoMinor) ? handle.getProtoMinor() : isprotoMinor;

      conn.protoMinor = protoMinor;
      conn.id = DSI::createId() ;
      conn.clientID = handle.getClientID();
      conn.serverID = handle.getServerID();
      conn.notificationID = CServicebroker::setClientDetachNotification( handle.getClientID(), mCommEngine->getNotificationSocketChid(), conn.id );

      // add the connection to the client connection list
      mClientConnections.insert(conn);

   // send back the reply to the client - with this data he will not reconnect anything
      IPv4::Endpoint ep = handle.getTCPChannel().getLocalName();

      // send back the reply to the client
      if (!ep)
      {
         DBG_ERROR(("Error retrieving local IP address" ));
         rci.ipAddress = rci.port = 0 ;
      }
      else
      {
         rci.ipAddress = ep.getIP();
         rci.port = mCommEngine->getIPPort();

         DSI::MessageHeader msg(handle.getServerID(), handle.getClientID(), DSI::ConnectResponse, conn.protoMinor, sizeof(rci));

         iov_t iov[2] = {
            { &msg, sizeof(msg) },
            { &rci, sizeof(rci) }
         };
         if (!handle.getChannel()->sendAll(iov, 2))
         {
            DBG_ERROR(("Error sending ConnectRequest reply back to client" ));
         }
      }
   }
}

void DSI::CServer::handleLegacyConnectRequestTCP(CTCPConnectRequestHandle& handle)
{
   TRC_SCOPE( dsi_base, CServer, global );
   DBG_MSG(("DSI::CServer::handleLegacyConnectRequestTCP() %s %d.%d - s:<%d.%d> c:<%d.%d>"
            , mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion
            , handle.getServerID().s.extendedID, handle.getServerID().s.localID
            , handle.getClientID().s.extendedID, handle.getClientID().s.localID ));

   IPv4::Endpoint ep = handle.getTCPChannel().getPeerName();

   char buf[32];
   DBG_MSG(("Peer Address %s", ep.toString(buf)));

   DSI::TCPConnectRequestInfo rci = handle.info();

   ClientConnection conn ;

   if( !mTCPIPEnabled )
   {
      DBG_ERROR(("TCP/IP not available" ));
      rci.ipAddress = rci.port = 0 ;
   }
   else
   {
      // do not reuse the given socket connection since it is temporary only!
      conn.channel = mCommEngine->attachTCP(handle.info().ipAddress, handle.info().port);

      uint16_t isprotoMinor = (uint16_t)DSI_PROTOCOL_VERSION_MINOR;
      uint16_t protoMinor = (handle.getProtoMinor() < isprotoMinor) ? handle.getProtoMinor() : isprotoMinor;
      conn.id = DSI::createId();
      conn.clientID = handle.getClientID();
      conn.serverID = handle.getServerID();
      conn.protoMinor = protoMinor;
      conn.notificationID = CServicebroker::setClientDetachNotification( handle.getClientID(), mCommEngine->getNotificationSocketChid(), conn.id );

      // add the connection to the client connection list
      mClientConnections.insert(conn);

      IPv4::Endpoint ep = handle.getTCPChannel().getLocalName();

      // send back the reply to the client
      if (!ep)
      {
         DBG_ERROR(("Error retrieving local IP address" ));
         rci.ipAddress = rci.port = 0 ;
      }
      else
      {
         rci.ipAddress = ep.getIP();
         rci.port = mCommEngine->getIPPort();
      }
   }

   if (!handle.getChannel()->sendAll(&rci, sizeof(rci)))
   {
      DBG_ERROR(("Error sending ConnectRequest reply back to client" ));
   }
}


void DSI::CServer::handleDisconnectRequest( const SPartyID &clientID )
{
   TRC_SCOPE( dsi_base, CServer, global );

   clientconnectionlist_type::iterator iter = mClientConnections.find(ClientConnection(clientID));
   if (iter != mClientConnections.end())
   {
      DBG_MSG(( "DSI::CServer::handleDisconnectRequest() %s %d.%d - s:<%d.%d> c:<%d.%d>"
                , mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion
                , iter->serverID.s.extendedID, iter->serverID.s.localID
                , iter->clientID.s.extendedID, iter->clientID.s.localID ));

      // remove the client detach notification if still available
      if( iter->notificationID )
      {
         CServicebroker::clearNotification( iter->notificationID );
      }

      // remove all client notifications
      removeNotification( iter->clientID );

      // remove all currently opened register sessions
      removeAllSessions(iter->clientID);

      // remove all unblocked sessions
      removeUnblockedSessions(iter->clientID);
      
      // remove the client connection from the list
      mClientConnections.erase(iter);
   } 
}


bool DSI::CServer::handleClientDetached(int id)
{
   TRC_SCOPE( dsi_base, CServer, global );
   bool retval = false;

   clientconnectionlist_type::iterator iter = std::find_if(mClientConnections.begin(), mClientConnections.end(), FindById(id));
   if (iter != mClientConnections.end())
   {
      DBG_MSG(( "DSI::CServer::handleClientDetached() %s %d.%d - s:<%d.%d> c:<%d.%d>"
                , mIfDescription.name, mIfDescription.version.majorVersion, mIfDescription.version.minorVersion
                , iter->serverID.s.extendedID, iter->serverID.s.localID
                , iter->clientID.s.extendedID, iter->clientID.s.localID ));

      // remove all client notifications
      removeNotification( iter->clientID );

      // remove all currently opened register sessions
      removeAllSessions(iter->clientID);

      // remove all unblocked sessions
      removeUnblockedSessions(iter->clientID);
      
      // remove the client connection from the list
      mClientConnections.erase(iter);
      
      retval = true;
   }
   
   return retval;
}


void DSI::CServer::removeUnblockedSessions(const SPartyID& clientID)
{
   for(unblockedsessionsmap_type::iterator iter = mUnblockedSessions.begin(); iter != mUnblockedSessions.end();)
   {
      if (iter->second.clientID == clientID)
      {  
         // where to continue
         unblockedsessionsmap_type::iterator next = iter;
         ++next;
       
         // remove it since the client will never get a response back       
         mUnblockedSessions.erase(iter);
         iter = next;
      }
      else
         ++iter;
   }
}


void DSI::CServer::sendErrorToClient( const SPartyID &clientID, uint32_t id, DSI::ResultType type, uint32_t seqNr )
{
   ClientConnection* conn = findClientConnection(clientID);
   if (conn && !conn->channel.expired())
   {
      CRequestWriter writer( *conn->channel.lock()
                           , type
                           , DSI::DataResponse
                           , id
                           , seqNr
                           , conn->clientID
                           , conn->serverID
                           , conn->protoMinor) ;

      COStream ostream(writer);
      ostream.write(0);
      (void)writer.flush();   // FIXME should handle return value here
   }
   else
   {
      assert(!conn->channel.expired());
   }
}
