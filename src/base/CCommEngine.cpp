/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/CCommEngine.hpp"
#include "dsi/CClient.hpp"
#include "dsi/CServer.hpp"
#include "dsi/CIStream.hpp"
#include "dsi/COStream.hpp"
#include "dsi/CRequestWriter.hpp"
#include "dsi/Log.hpp"

#include "dsi/private/static_assert.hpp"
#include "dsi/private/CNonCopyable.hpp"

#include "CTraceManager.hpp"
#include "CServicebroker.hpp"
#include "io.hpp"
#include "bind.hpp"
#include "CLocalChannel.hpp"
#include "CTCPChannel.hpp"
#include "CRequestReader.hpp"
#include "CConnectRequestHandle.hpp"
#include "DSI.hpp"
#include "CClientConnectSM.hpp"

#include <algorithm>
#include <tr1/functional>
#include <memory>
#include <cassert>
#include <errno.h>
#include <cxxabi.h>
#include <arpa/inet.h>
#include <sys/syscall.h>

#define gettid() syscall(SYS_gettid)

TRC_SCOPE_DEF( dsi_base, CCommEngine, global );
TRC_SCOPE_DEF( dsi_base, CCommEngine, loop );
TRC_SCOPE_DEF( dsi_base, CCommEngine, handleMessage );
TRC_SCOPE_DEF( dsi_base, CCommEngine, handlePulse );


using namespace std::tr1::placeholders;
using std::tr1::ref;


namespace /*anonymous*/
{

   struct Recv
   {
      static inline const char* env() { return "DSI_RECV_TIMEOUT"; }
   };

   struct Send
   {
      static inline const char* env() { return "DSI_SEND_TIMEOUT"; }
   };

   template<typename T>
   int getTimeoutMs()
   {
      static int gbl_timeout_ms = -1;

      if (gbl_timeout_ms < 0)
      {
         const char* env = ::getenv(T::env());
         gbl_timeout_ms = env ? atoi(env) : 0;
      }

      return gbl_timeout_ms;
   }


   const char* makeLocalPath(char* buf, size_t len, int pid, int chid)
   {
      if (buf && len > 23)
      {
         ::memset(buf, 0, len);
         ::sprintf(buf+1, "dsi/%d/%d", pid, chid);
         return buf;
      }

      return 0;
   }


   template<typename MapT, typename ValueT>
   void removeFromCache(MapT& map, ValueT* value)
   {
      for(typename MapT::iterator iter = map.begin(); iter != map.end(); ++iter)
      {
         if (iter->second == value)
         {
            map.erase(iter);
            break;
         }
      }
   }

}   // namespace anonymous


// --------------------------------------------------------------------------------------
namespace DSI
{

   class ConnectionResetter
   {
   public:

      template<typename T>
      inline
      void operator()(T t)
      {
         t->mCommEngine = 0;
      }
   };


   struct FindByPartyID
   {
      explicit inline
      FindByPartyID(const SPartyID& id)
         : mId(id)
      {
         // NOOP
      }

      inline
      bool operator()(CClient* t)
      {
         return t->mClientID.globalID == mId.globalID;
      }

      inline
      bool operator()(CServer* t)
      {
         return t->mServerID == mId || t->mTCPServerID == mId;
      }

      const SPartyID& mId;
   };


   // --------------------------------------------------------------------------------


   class CNotificationClient
   {
   public:

      inline
      CNotificationClient(const Unix::StreamSocket& sock, CCommEngine::Private& commEngineImp)
         : mSock(sock)
         , mCommEngineImp(commEngineImp)
      {
         mSock.async_read_all(&mPulse, sizeof(mPulse), bind3(&CNotificationClient::handlePulse, this, _1, _2));
      }

   private:

      bool handlePulse(size_t /*len*/, io::error_code ec);

      Unix::StreamSocket mSock;
      sb_pulse_t mPulse;

      CCommEngine::Private& mCommEngineImp;
   };


   // ---------------------------------------------------------------------------------------


   class CClientConnection : public Private::CNonCopyable
   {
      static inline
      CChannel* createChannel(Unix::StreamSocket& sock)
      {
         return new CLocalChannel(sock);
      }

      static inline
      CChannel* createChannel(IPv4::StreamSocket& sock)
      {
         return new CTCPChannel(sock);
      }

   public:
      ~CClientConnection();

      template<typename SocketT>
      CClientConnection(SocketT& sock, CCommEngine::Private& commEngine);

      bool handleMessageHeader(size_t len, io::error_code ec);

      inline
      std::tr1::shared_ptr<CChannel> channel()
      {
         return mChnl;
      }

   private:

      // currently to be received message
      DSI::MessageHeader mBuf;

      std::tr1::shared_ptr<CChannel> mChnl;
      CCommEngine::Private& mCommEngineImp;
   };


   // ---------------------------------------------------------------------------------------


   struct CCommEngine::Private : public DSI::Private::CNonCopyable
   {
      typedef std::vector<CClient*> clientlist_type;
      typedef std::vector<CServer*> serverlist_type;

      typedef std::map<SPartyID, CBase*> partycache_type;

      Private();
      ~Private();

      int loop();

      int run();
      void stop(int rc);
      void startTCP();

      void cleanupChannel(std::tr1::shared_ptr<CChannel> channel);

      void registerInterface(CServer* server);

      bool handleMessage(const DSI::MessageHeader& header, std::tr1::shared_ptr<CChannel> channel);

      bool handleNewNotificationConnection(Unix::Endpoint& address, io::error_code err);
      bool handleNewLocalConnection(Unix::Endpoint& address, io::error_code err);
      bool handleNewTCPConnection(IPv4::Endpoint& address, io::error_code err);

      void add( CClient &client );
      void remove( CClient &client );
      void add( CServer &server );
      void remove( CServer &server );

      int32_t getIPPort();

      // handles incomming pulses and fowards them to the receiver stub/proxy.
      void handlePulse( sb_pulse_t &pulse );

      CServer* findServer( int32_t id );
      CServer* findServer( const SPartyID& serverID );

      CClient* findClient( int32_t id );
      CClient* findClient( const SPartyID& clientID );

      int              mSBNotifyChid;
      int              mLocalChid;
      int              mSenderTid;
      bool             mActive;

      clientlist_type  mClientList;
      serverlist_type  mServerList;

      /// fast lookup caches for clients and servers. These are necessary on top
      /// of the client and server lists because in the point of time when adding
      /// clients and servers to the connection their party ids are not yet
      /// initialized <0.0>
      partycache_type mServerCache;
      partycache_type mClientCache;

      Dispatcher mDispatch;

      // notification socket handling
      Unix::Acceptor mNotificationAcceptor;
      Unix::StreamSocket mNextNotificationSocket;

      // TCP acceptor
      IPv4::Acceptor mTCPAcceptor;
      IPv4::StreamSocket mNextTCPSocket;

      // normal unix acceptor (standard DSI)
      Unix::Acceptor mLocalAcceptor;
      Unix::StreamSocket mNextLocalSocket;

      std::map<Unix::Endpoint, CClientConnection*> mLocalChannels;
      std::map<IPv4::Endpoint, CClientConnection*> mTCPChannels;
   };

}//namespace DSI

// --------------------------------------------------------------------------------


bool DSI::CNotificationClient::handlePulse(size_t /*len*/, io::error_code ec)
{
   if (ec == io::ok)
   {
      mCommEngineImp.handlePulse(mPulse);
      return true;
   }

   delete this;
   return false;
}


// --------------------------------------------------------------------------------


template<typename SocketT>
DSI::CClientConnection::CClientConnection(SocketT& sock, DSI::CCommEngine::Private& commEngine)
 : mChnl()
 , mCommEngineImp(commEngine)
{
   if (getTimeoutMs<Send>() > 0)
      sock.setSocketOption(SocketSendTimeoutOption(getTimeoutMs<Send>()));

   if (getTimeoutMs<Recv>() > 0)
      sock.setSocketOption(SocketReceiveTimeoutOption(getTimeoutMs<Recv>()));

   sock.async_read_all(&mBuf, sizeof(mBuf), bind3(&CClientConnection::handleMessageHeader, this, _1, _2));
   mChnl.reset(createChannel(sock));
}


DSI::CClientConnection::~CClientConnection()
{
   mCommEngineImp.cleanupChannel(mChnl);
}


bool DSI::CClientConnection::handleMessageHeader(size_t /*len*/, io::error_code ec)
{
   bool rc = false;

   if (ec == io::ok)
      rc = mCommEngineImp.handleMessage(mBuf, mChnl);

   if (!rc)
      delete this;

   return rc;
}


// --------------------------------------------------------------------------------


DSI::CCommEngine::Private::Private()
   : mSBNotifyChid(SBOpenNotificationHandle())
   , mLocalChid(0)
   , mSenderTid(0)
   , mActive(false)
   , mDispatch()
   , mNotificationAcceptor(mDispatch, mSBNotifyChid)
   , mNextNotificationSocket(mDispatch)
   , mTCPAcceptor(mDispatch, IPv4::Acceptor::traits_type::Invalid)
   , mNextTCPSocket(mDispatch)
   , mLocalAcceptor(mDispatch, Unix::Acceptor::traits_type::Invalid)
   , mNextLocalSocket(mDispatch)
{
   (void)mNotificationAcceptor.listen();
   mNotificationAcceptor.async_accept(mNextNotificationSocket, bind3(&Private::handleNewNotificationConnection, this,
                                                                     _1, _2));
}


DSI::CCommEngine::Private::~Private()
{
   std::for_each(mServerList.begin(), mServerList.end(), ConnectionResetter());
   std::for_each(mClientList.begin(), mClientList.end(), ConnectionResetter());
}


bool DSI::CCommEngine::Private::handleNewNotificationConnection(Unix::Endpoint& /*address*/, io::error_code err)
{
   if (err == io::ok)
   {
      // FIXME performance issue: does SB disconnect the sb interface for each notification?
      CNotificationClient* newClient = new CNotificationClient(mNextNotificationSocket, *this);
      // FIXME set recv timeout
      (void)newClient;    // we leave him alone
   }

   return true;
}


bool DSI::CCommEngine::Private::handleNewTCPConnection(IPv4::Endpoint& address, io::error_code err)
{
   if (err == io::ok)
   {
      mNextTCPSocket.setSocketOption(SocketTcpNoDelayOption<true>());

      mTCPChannels[address] = new CClientConnection(mNextTCPSocket, *this);
   }

   return true;
}


bool DSI::CCommEngine::Private::handleNewLocalConnection(Unix::Endpoint& address, io::error_code err)
{
   if (err == io::ok)
   {
      mLocalChannels[address] = new CClientConnection(mNextLocalSocket, *this);
   }

   return true;
}


void DSI::CCommEngine::Private::add(CClient &client)
{
   mClientList.push_back(&client);

   if( mActive )
      client.setServerAvailableNotification( mSBNotifyChid ) ;
}


void DSI::CCommEngine::Private::remove(CClient &client)
{
   std::remove(mClientList.begin(), mClientList.end(), &client);
   removeFromCache(mClientCache, &client);

   if (mActive)
   {
      client.detachInterface( false ) ;
   }
}


void DSI::CCommEngine::Private::add(CServer &server)
{
   mServerList.push_back( &server );

   if( mActive )
   {
      server.registerInterface(mLocalAcceptor.fd()) ;

      if( server.isTCPIPEnabled() )
         server.registerInterfaceTCP( DSI::getLocalIpAddress(), getIPPort() ) ;

      server.registerInterface(mLocalAcceptor.fd()) ;
   }
}


void DSI::CCommEngine::Private::remove(CServer &server)
{
   std::remove(mServerList.begin(), mServerList.end(), &server);
   removeFromCache(mServerCache, &server);

   if( mActive )
      server.unregisterInterface() ;
}


void DSI::CCommEngine::Private::registerInterface(CServer* server)
{
   if( server->isTCPIPEnabled() )
      server->registerInterfaceTCP(DSI::getLocalIpAddress(), getIPPort()) ;

   server->registerInterface(mLocalAcceptor.fd()) ;
}


int DSI::CCommEngine::Private::run()
{
   TRC_SCOPE( dsi_base, CCommEngine, global );
   DBG_MSG(( "CCommEngine::run()" ));

   int retval = -1 ;
   if (!mLocalAcceptor)
   {
      if (mLocalAcceptor.open() == io::ok)
      {
         mLocalChid = mLocalAcceptor.fd();

         char buf[24];
         if(mLocalAcceptor.bind(makeLocalPath(buf, sizeof(buf), ::getpid(),
                                              mLocalChid)) == io::ok && mLocalAcceptor.listen() == io::ok)
         {
            mSenderTid = gettid();

            // Servicebroker registration for servers and clients
            std::for_each(mServerList.begin(), mServerList.end(), std::tr1::bind(
                             &CCommEngine::Private::registerInterface, this, _1));
            std::for_each(mClientList.begin(), mClientList.end(), std::tr1::bind(
                             &CClient::setServerAvailableNotification, _1, mSBNotifyChid));

            mActive = true;

            mLocalAcceptor.async_accept(mNextLocalSocket, bind3(
                                           &CCommEngine::Private::handleNewLocalConnection, this, _1, _2));

            // enter sender message loop
            retval = loop();

            mActive = false ;

            std::for_each(mClientList.begin(), mClientList.end(), std::tr1::bind(&CClient::detachInterface, _1, false));
            std::for_each(mServerList.begin(), mServerList.end(), std::tr1::bind(&CServer::unregisterInterface, _1));

            mSenderTid = 0;
         }

         mLocalAcceptor.close();
         mTCPAcceptor.close();
      }
      else
         DBG_ERROR(( "Error creating the communication engine" ));
   }

   return retval;
}


void DSI::CCommEngine::Private::stop(int exitcode)
{
   TRC_SCOPE( dsi_base, CCommEngine, global );
   DBG_MSG(( "CCommEngine::stop()" ));

   mDispatch.stop(exitcode);
}


void DSI::CCommEngine::Private::startTCP()
{
   if (!mTCPAcceptor)
   {
      io::error_code ec = io::unspecified;

      ec = mTCPAcceptor.open();
      assert(ec == io::ok);

      const char* port = ::getenv("DSI_COMMENGINE_PORT");
      if (port)
      {
         int iport = ::atoi(port);
         assert(iport > 0 && iport < 65536);

         ec = mTCPAcceptor.setSocketOption(SocketReuseAddressOption<true>());
         assert(ec == io::ok);

         ec = mTCPAcceptor.bind(IPv4::Endpoint(iport));
         assert(ec == io::ok);
      }

      ec = mTCPAcceptor.listen();
      assert(ec == io::ok);

      mTCPAcceptor.async_accept(mNextTCPSocket, bind3(&CCommEngine::Private::handleNewTCPConnection, this, _1, _2));
   }
}


int DSI::CCommEngine::Private::loop()
{
   TRC_SCOPE( dsi_base, CCommEngine, loop );
   DBG_MSG(( "CCommEngine::loop()" ));

   try
   {
      return mDispatch.run();
   }
   catch (abi::__forced_unwind&)
   {
      throw;
   }
   catch(std::exception& ex)
   {
      // do not catch CppUnit Exceptions since then CppUnit would show a wrong error;
      // instead of pointing to the correct error it just shows engine.run() == -1
      if (strstr(typeid(ex).name(), "CppUnit") != 0)
         throw;
      
      Log::syslog(Log::Critical, "Unhandled exception in CCommEngine: %s", ex.what());      
   }
   catch(...)
   {
      Log::syslog(Log::Critical, "Unhandled unknown exception in CCommEngine");
   }

   return -1;
}


bool DSI::CCommEngine::Private::handleMessage(const DSI::MessageHeader& hdr, std::tr1::shared_ptr<CChannel> chnl)
{
   TRC_SCOPE( dsi_base, CCommEngine, handleMessage );

   CRequestReader reader(hdr, *chnl);
   bool rc = false;

   DBG_MSG(( "CCommEngine::handleMessage() %s c:<%d.%d> s:<%d.%d>"
             , DSI::commandToString(hdr.cmd)
             , hdr.clientID.s.extendedID, hdr.clientID.s.localID
             , hdr.serverID.s.extendedID, hdr.serverID.s.localID ));

   if (hdr.protoMajor != DSI_PROTOCOL_VERSION_MAJOR )
   {
      DBG_ERROR(("CRequestReader: bad protocol version (expected: %d.%d, received: %d.%d) %d"
                 , DSI_PROTOCOL_VERSION_MAJOR, DSI_PROTOCOL_VERSION_MINOR
                 , hdr.protoMajor, hdr.protoMinor ,hdr.cmd));
   }
   else
   {
      uint16_t isprotoMinor = (uint16_t)DSI_PROTOCOL_VERSION_MINOR;

      if( hdr.protoMinor >  isprotoMinor)
      {
         DBG_WARNING(("CRequestReader:  minor protocol version differs (expected: %d.%d, received: %d.%d) %d"
                    , DSI_PROTOCOL_VERSION_MAJOR, DSI_PROTOCOL_VERSION_MINOR
                    , hdr.protoMajor, hdr.protoMinor ,hdr.cmd));
      }
      else  if( hdr.protoMinor <  isprotoMinor)
      {
         DBG_WARNING(("CRequestReader:  minor protocol version differs (expected: %d.%d, received: %d.%d) %d"
                    , DSI_PROTOCOL_VERSION_MAJOR, DSI_PROTOCOL_VERSION_MINOR
                    , hdr.protoMajor, hdr.protoMinor, hdr.cmd ));
      }
      rc = reader.receiveAll();

      if (rc)
      {
         switch(hdr.cmd)
         {
            case DSI::ConnectRequest:
            {
               CServer* server = findServer(hdr.serverID);

               if (server)
               {
                  if (dynamic_cast<CTCPChannel*>(chnl.get()) != 0)
                  {
                     CTCPConnectRequestHandle handle(hdr, chnl, *(DSI::TCPConnectRequestInfo*)reader.buffer());
                     if (hdr.packetLength == sizeof(DSI::TCPConnectRequestInfo))
                     {
                        server->handleLegacyConnectRequestTCP(handle);
                     }
                     else
                     {
                        server->handleConnectRequestTCP(handle);
                     }
                  }
                  else
                  {
                     CConnectRequestHandle handle(hdr, chnl, *(DSI::ConnectRequestInfo*)reader.buffer());
                     server->handleConnectRequest(handle);
                  }
               }
               else
               {
                  // send back an invalid message as error message
                  DSI_STATIC_ASSERT(sizeof(DSI::ConnectRequestInfo) == sizeof(DSI::TCPConnectRequestInfo));

                  if (dynamic_cast<CTCPChannel*>(chnl.get()) != 0)
                  {                     
                     // legacy stuff
                     DSI::TCPConnectRequestInfo cri = { 0, 0 };
                     if (!chnl->sendAll(&cri, sizeof(cri)))
                        return false;
                  }
                  else
                  {
                     CRequestWriter writer(*chnl, DSI::ConnectResponse, hdr.clientID, hdr.serverID, DSI_PROTOCOL_VERSION_MINOR);
                     writer.sbrk(sizeof(DSI::ConnectRequestInfo));
                     memset(writer.pptr(), 0, sizeof(DSI::ConnectRequestInfo));
                     writer.pbump(sizeof(DSI::ConnectRequestInfo));
                     
                     if (!writer.flush())
                        return false;
                  }
               }
            }
            break;

            case DSI::DisconnectRequest:
            {            
               CServer* server = findServer(hdr.serverID);
               if (server)
               {             
                  CInputTraceSession session(server->mIfDescription);
                  if (session.isActive())
                     session.write(&hdr);                  

                  server->handleDisconnectRequest(hdr.clientID);
               }
            }
            break;

            case DSI::ConnectResponse:
            {
               CClient* client = findClient(hdr.clientID);

               if (client)
               {
                  CConnectRequestHandle handle(hdr, chnl, *(DSI::ConnectRequestInfo*)reader.buffer());
                  client->handleConnectResponse(handle);
               }
            }
            break;

            case DSI::DataRequest:
            {
               CServer* server = findServer( hdr.serverID );

               if (server)
               {
                  DSI::Private::CDataRequestHandle handle(hdr, chnl, reader.buffer(), reader.size());
                  server->handleDataRequest(handle);
               }
               else
               {
                  DBG_ERROR(( "CCommEngine: Unknown Server: s:<%d.%d> (c:<%d.%d>)"
                              , hdr.serverID.s.extendedID, hdr.serverID.s.localID
                              , hdr.clientID.s.extendedID, hdr.clientID.s.localID ));
               }
            }
            break;

            case DSI::DataResponse:
            {
               CClient* client = findClient( hdr.clientID );

               if (client)
               {
                  DSI::Private::CDataResponseHandle handle(hdr, chnl, reader.buffer(), reader.size());
                  client->handleDataResponse(handle);
               }
               else
               {
                  DBG_ERROR(( "CCommEngine: Unknown Client: c:<%d.%d> (s:<%d.%d>)"
                              , hdr.clientID.s.extendedID, hdr.clientID.s.localID
                              , hdr.serverID.s.extendedID, hdr.serverID.s.localID ));
               }
            }
            break;

            default:
               break;
         }
      }
   }

   return rc;
}


void DSI::CCommEngine::Private::handlePulse( sb_pulse_t &pulse )
{
   TRC_SCOPE( dsi_base, CCommEngine, handlePulse );
   DBG_MSG(( "CCommEngine::handlePulse() %s", DSI::toString((DSI::PulseCode)pulse.code)));

   switch( pulse.code )
   {
      case DSI::PULSE_SERVER_AVAILABLE:
      {
         CClient* client = findClient( pulse.value ) ;
         if (client)
            client->attachInterface();
      }

      break;

      case DSI::PULSE_SERVER_DISCONNECT:
      {
         CClient* client = findClient( pulse.value ) ;
         if (client)
         {
            removeFromCache(mClientCache, client);
            client->detachInterface( true );
         }
      }

      break;

      case DSI::PULSE_CLIENT_DETACHED:
         for(serverlist_type::iterator iter = mServerList.begin(); iter != mServerList.end(); ++iter)
         {
            if( (*iter)->handleClientDetached( pulse.value ))
               break;
         }

         break;

      default:
         break;
   }
}


void DSI::CCommEngine::Private::cleanupChannel(std::tr1::shared_ptr<CChannel> channel)
{
   for(serverlist_type::iterator serverIt = mServerList.begin(); serverIt != mServerList.end(); ++serverIt)
   {
      for(CServer::clientconnectionlist_type::iterator clientConnIt = (*serverIt)->mClientConnections.begin();
          clientConnIt != (*serverIt)->mClientConnections.end(); ++clientConnIt)
      {
         if ( !clientConnIt->channel.expired() && clientConnIt->channel.lock() == channel)
         {
            (*serverIt)->handleClientDetached( clientConnIt->id );
         }
      }
   }

   for(clientlist_type::iterator clientIt = mClientList.begin(); clientIt != mClientList.end(); ++clientIt)
   {
      if ( !(*clientIt)->mChannel.expired() && (*clientIt)->mChannel.lock() == channel)
      {
         (*clientIt)->detachInterface( true );
      }
   }
}


int32_t DSI::CCommEngine::Private::getIPPort()
{
   if (!mTCPAcceptor)
      startTCP();

   return mTCPAcceptor.getEndpoint().getPort();
}


DSI::CServer* DSI::CCommEngine::Private::findServer( const SPartyID& serverID )
{
   CServer* rc = 0;
   partycache_type::iterator iter = mServerCache.find(serverID);

   if (iter != mServerCache.end())
   {
      rc = static_cast<CServer*>(iter->second);
   }
   else
   {
      serverlist_type::iterator iter = std::find_if(mServerList.begin(), mServerList.end(), FindByPartyID(serverID));

      if (iter != mServerList.end())
      {
         mServerCache[(*iter)->mServerID] = *iter;
         rc = *iter;
      }
   }

   return rc;
}


DSI::CClient* DSI::CCommEngine::Private::findClient( const SPartyID& clientID )
{
   CClient* rc = 0;
   partycache_type::iterator iter = mClientCache.find(clientID);

   if (iter != mClientCache.end())
   {
      rc = static_cast<CClient*>(iter->second);
   }
   else
   {
      clientlist_type::iterator iter = std::find_if(mClientList.begin(), mClientList.end(), FindByPartyID(clientID));

      if (iter != mClientList.end())
      {
         mClientCache[(*iter)->mClientID] = *iter;
         rc = *iter;
      }
   }

   return rc;
}


DSI::CClient* DSI::CCommEngine::Private::findClient( int32_t id )
{
   clientlist_type::iterator iter = std::find_if(mClientList.begin(), mClientList.end(), FindById(id));
   return iter != mClientList.end() ? *iter : 0;
}


DSI::CServer* DSI::CCommEngine::Private::findServer( int32_t id )
{
   serverlist_type::iterator iter = std::find_if(mServerList.begin(), mServerList.end(), FindById(id));
   return iter != mServerList.end() ? *iter : 0;
}


// ----------------------------------------------------------------------------------------


DSI::CCommEngine::CCommEngine()
   : d(new CCommEngine::Private())
{
   // NOOP
}


DSI::CCommEngine::~CCommEngine()
{
   delete d;
}


int32_t DSI::CCommEngine::getNotificationSocketChid()
{
   return d->mSBNotifyChid;
}


int DSI::CCommEngine::run()
{
   return d->run();
}


void DSI::CCommEngine::stop(int exitcode)
{
   d->stop(exitcode);
}


bool DSI::CCommEngine::add( CClient &client )
{
   TRC_SCOPE( dsi_base, CCommEngine, global );
   DBG_MSG(("CCommEngine::add() client %s %d.%d", client.mIfDescription.name,
            client.mIfDescription.version.majorVersion, client.mIfDescription.version.minorVersion));

   if( !client.mCommEngine )
   {
      client.mCommEngine = this ;
      d->add(client);

      return true ;
   }

   return false ;
}


bool DSI::CCommEngine::remove( CClient &client )
{
   TRC_SCOPE( dsi_base, CCommEngine, global );
   DBG_MSG(("CCommEngine::remove() client %s %d.%d", client.mIfDescription.name,
            client.mIfDescription.version.majorVersion, client.mIfDescription.version.minorVersion));

   if (this == client.mCommEngine)
   {
      d->remove(client);
      client.mCommEngine = 0 ;

      return true ;
   }

   return false ;
}


bool DSI::CCommEngine::add( CServer &server )
{
   TRC_SCOPE( dsi_base, CCommEngine, global );
   DBG_MSG(("CCommEngine::add() server %s %d.%d", server.mIfDescription.name,
            server.mIfDescription.version.majorVersion, server.mIfDescription.version.minorVersion));

   if (!server.mCommEngine)
   {
      server.mCommEngine = this ;
      d->add(server);

      return true ;
   }

   return false ;
}


bool DSI::CCommEngine::remove( CServer &server )
{
   TRC_SCOPE( dsi_base, CCommEngine, global );
   DBG_MSG(("CCommEngine::remove() server %s %d.%d", server.mIfDescription.name,
            server.mIfDescription.version.majorVersion, server.mIfDescription.version.minorVersion));

   if (this == server.mCommEngine)
   {
      d->remove(server);
      server.mCommEngine = 0 ;

      return true ;
   }

   return false ;
}


int32_t DSI::CCommEngine::getLocalChannel()
{
   return d->mLocalChid;
}


int32_t DSI::CCommEngine::getIPPort()
{
   return d->getIPPort();
}


std::tr1::shared_ptr<DSI::CChannel> DSI::CCommEngine::attach(int32_t pid, int32_t chid)
{
   TRC_SCOPE( dsi_base, CCommEngine, global );

   std::tr1::shared_ptr<CChannel> rc;

   char buf[32];
   Unix::Endpoint ep(makeLocalPath(buf, sizeof(buf), pid, chid));

   std::map<Unix::Endpoint, CClientConnection*>::iterator iter = d->mLocalChannels.find(ep);
   if (iter != d->mLocalChannels.end())
   {
      DBG_MSG(("Found local channel '%s' in cache", buf+1));

      rc = iter->second->channel();
   }

   if (!rc)
   {
      Unix::StreamSocket sock(d->mDispatch);
      if (sock.open() == io::ok && sock.connect(ep) == io::ok)
      {
         DBG_MSG(("Connected to local channel '%s'...", buf+1));

         CClientConnection* conn = new CClientConnection(sock, *d);
         rc = conn->channel();

         d->mLocalChannels[ep] = conn;
      }
      else
      {
         int32_t err = errno;
         DBG_ERROR(("Failed to connect to local channel '%s' %d", buf+1, err));
      }
   }

   return rc;
}


std::tr1::shared_ptr<DSI::CChannel> DSI::CCommEngine::attachTCP(uint32_t host, uint32_t port, bool private_)
{
   TRC_SCOPE( dsi_base, CCommEngine, global );

   std::tr1::shared_ptr<CChannel> rc;

   IPv4::Endpoint ep(host, port);
   char buf[24];

   // search in connections cache
   if (!private_)
   {
      std::map<IPv4::Endpoint, CClientConnection*>::iterator iter = d->mTCPChannels.find(ep);
      if (iter != d->mTCPChannels.end())
      {
         DBG_MSG(("Found TCP channel '%s' in cache", ep.toString(buf)));

         rc = iter->second->channel();
      }
   }

   if (!rc)
   {
      IPv4::StreamSocket sock(d->mDispatch);
      if (sock.open() == io::ok && sock.connect(ep, 5000/*5 seconds timeout*/) == io::ok)
      {
         DBG_MSG(("Connected to TCP channel '%s'...", ep.toString(buf)));

         (void)sock.setSocketOption(SocketTcpNoDelayOption<true>());

         if (!private_)
         {
            CClientConnection* conn = new CClientConnection(sock, *d);
            rc = conn->channel();

            d->mTCPChannels[ep] = conn;
         }
         else
         {
            if (getTimeoutMs<Send>() > 0)
               sock.setSocketOption(SocketSendTimeoutOption(getTimeoutMs<Send>()));

            rc.reset(new CTCPChannel(sock));
         }
      }
      else
         DBG_ERROR(("Failed to connect to TCP channel '%s'", ep.toString(buf)));
   }

   return rc;
}


void DSI::CCommEngine::removeGenericDevice(int fd)
{
   d->mDispatch.removeAll(fd);
}


DSI::CClient* DSI::CCommEngine::findClient(int32_t id)
{
   return d->findClient(id);
}


DSI::CServer* DSI::CCommEngine::findServer(int32_t id)
{
   return d->findServer(id);
}


// ------------------------------------------------------------------------------------------


class CFunctionForwarder
{
public:

   explicit inline
   CFunctionForwarder(const std::tr1::function<bool(DSI::CCommEngine::IOResult)>& func)
      : mFunc(func)
   {
      // NOOP
   }

   inline
   bool operator()(DSI::GenericEventBase::Result result)
   {
      DSI_STATIC_ASSERT((int)DSI::CCommEngine::DataAvailable == (int)DSI::GenericEventBase::DataAvailable);

      DSI_STATIC_ASSERT((int)DSI::CCommEngine::CanWriteNow == (int)DSI::GenericEventBase::CanWriteNow);
      DSI_STATIC_ASSERT((int)DSI::CCommEngine::DeviceHungup == (int)DSI::GenericEventBase::DeviceHungup);

      DSI_STATIC_ASSERT((int)DSI::CCommEngine::InvalidFileDescriptor == (int)DSI::GenericEventBase::InvalidFileDescriptor);
      DSI_STATIC_ASSERT((int)DSI::CCommEngine::GenericError == (int)DSI::GenericEventBase::GenericError);

      return mFunc(static_cast<DSI::CCommEngine::IOResult>(result));
   }

private:

   std::tr1::function<bool(DSI::CCommEngine::IOResult)> mFunc;
};


/*static*/
void DSI::CCommEngine::pimplCCommEngineAddGenericDevice(
   void* d, int fd, DSI::CCommEngine::DataFlowDirection dir,
   const std::tr1::function<bool(DSI::CCommEngine::IOResult)>& handler)
{
   static short toPoll[] = { POLLIN, POLLOUT, POLLIN|POLLOUT };
   ((DSI::CCommEngine::Private*)d)->mDispatch.enqueueEvent(
      fd, new GenericEvent<CFunctionForwarder>(CFunctionForwarder(handler)), (short)toPoll[dir - 1]);
}
