/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_CCOMMENGINE_HPP
#define DSI_CCOMMENGINE_HPP

#include <tr1/functional>

#include "DSI.hpp"
#include "CChannel.hpp"

#include "dsi/private/CNonCopyable.hpp"


// forward decl
class CDSIConnection;


namespace DSI
{
   class CClient;
   class CServer;
   class CIStream;


   /**
    * @class CCommEngine "CCommEngine.hpp" "dsi/CCommEngine.hpp"
    *
    * A communication engine object must run in an I/O dedicated thread. The engine will serve 
    * all DSI servers and clients added to it by distributing the I/O operations correspondingly.
    * It can be seen as the DSI applications main loop.
    *
    * The following example shows how to use the CCommEngine class:
    *
    * @code
    *   CMyServiceDSIStubImpl stub;     // implementation of the stub
    *   CMyServiceDSIProxyImpl proxy;   // implementation of the proxy
    *
    *   CCommEngine commEngine;
    *   commEngine.add(stub);           // adds the server stub to the communication engine
    *   commEngine.add(proxy);          // adds the client proxy to the communication engine
    *   commEngine.run();               // runs the event loop of the communication engine. 
    * @endcode
    *
    * Make sure, the lifetime of your stub and proxy objects added to the communcation
    * engine lasts until the @run() function returns or you actively removed client or server.
    *
    * You can add new clients or servers to the communication engine even after the 
    * engine entered it's event loop. And beware that it is not possible to add the same
    * client or server instance to multiple instances of the communcation engine.
    *
    * @note Due to the current implementation, it is not possible to run a client and server within 
    *       the same communication engine. Such an implementation would run into a blocking system 
    *       call during the synchronous implementation of ConnectAttach and wait forever. 
    *
    * @todo See the above note about ConnectAttach blocking system call which does not support 
    *       clients and servers transparently within the same communication engine.    
    */
   class CCommEngine : public Private::CNonCopyable
   {      
      friend class CNotificationClient;
      friend class CClientConnection;
      friend class CServer;
      friend class CClient;
      friend class CClientConnectSM;
      friend class ::CDSIConnection;

   public:

      /** 
       * How should corresponding generic fd as given to @c addGenericDevice be supervised 
       * by the internal multiplexer?
       */
      enum DataFlowDirection
      {
         In =  1<<0,        ///< Data should be received on the corresponding fd.
         Out = 1<<1,        ///< Data should be transmitted over the corresponding fd.
         
         InOut = In | Out   ///< The device should be used to send and receive data.
      };

      /**
       * Result values depend on DataFlowDirection settings. This is used as an argument 
       * by the generic device interface
       * 
       * @see addGenericDevice
       */
      enum IOResult
      {
         DataAvailable = 0,       ///< Data can now be read.
         CanWriteNow,             ///< Data can now be written.
         DeviceHungup,            ///< Device received a hangup (e.g. peer sockets closed).

         InvalidFileDescriptor,   ///< The internal multiplexer got an invalid file descriptor.
         GenericError             ///< any other error occurred.
      };
            
      /**
       * Creates a new communication engine object for local transport initially. The communication 
       * engine automatically adds TCP/IP transport once a TCP/IP enabled server is added. It then
       * binds to an arbitrary TCP/IP port. If you want to choose a distinct port you may set the
       * @c DSI_COMMENGINE_PORT environment variable pointing to the port number the engine should
       * use for installing the TCP/IP acceptor.
       */
      CCommEngine();

      /**
       * Frees the communication engine object.
       */
      ~CCommEngine();

      /**
       * Starts the main event loop of a DSI application. This call will not return. Clients and server
       * may be added to the communication engine prior to calling @run() or afterwards within a callback.
       */
      int run();

      /**
       * Stops the main event loop, i.e. let @run() return. May be even called from another thread context.
       *
       * @param exitcode The exitcode to be returned by @c run().
       */
      void stop(int exitcode = 0);

      /**
       * Adds a client (proxy) to the communication engine. Once the client is added 
       * it may be removed via @c remove(). It is only possible to add a client
       * to one communication engine at a time. You may call this even after the
       * main event loop is started.
       *       
       * @param client the client
       * @return true if the client has been add to the communication engine.
       */
      bool add(CClient &client);

      /**
       * Removes a client (proxy) from the communication engine.
       *
       * @param client The client to be removed.
       * @return true if the client has been removed from the communication engine.
       */
      bool remove(CClient &client);

      /**
       * Adds a server (stub) to the communication engine. Once the server is added 
       * it may be removed via @c remove(). It is only possible to add a server
       * to one communication engine at a time. You may call this even after the
       * main event loop is started.
       *
       * @param server The server to add.
       * @return true if the server has been add to the communication engine.
       */
      bool add(CServer &server);

      /**
       * Removes a server (stub) from the communication engine.
       *
       * @param server The server to be removed.
       * @return true if the server has been removed from the communication engine.
       */
      bool remove(CServer &server);
      
      /**
       * Add a generic device that will be included into the main event loop for io-event
       * multiplexing (poll). Devices may be everything that is pollable, e.g. sockets,
       * message queues, pipes, ...
       *
       * @c HandlerT must be a functor taking exactly one argument of type @c IOResult returning
       * a boolean value. In case of the boolean value beeing true, the given fd will be rearmed for
       * the former io operation, i.e. not be removed from the pollfd set. 
       * 
       * @note You cannot add the same fd multiple times, even with different @c DataFlowDirection.
       */
      template<typename HandlerT>
      inline
      void addGenericDevice(int fd, DataFlowDirection dir, const HandlerT& handler)
      {
         pimplCCommEngineAddGenericDevice(d, fd, dir, std::tr1::function<bool(IOResult)>(handler));
      }

      /**
       * Remove a previously registered file descriptor from the internal multiplexer.
       */
      void removeGenericDevice(int fd);

   private:

      /**       
       * Returns the unix socket fd used for servicebroker notifications. Don't use this channel id. It
       * is used internally.
       */
      int32_t getNotificationSocketChid();
      
      /**
       * The channel id (fd) of the local transport socket. Will be used for referring services 
       * during the servicebroker registration.
       */
      int32_t getLocalChannel();
      
      /**
       * @return the IP port number currently bound by the TCP/IP acceptor. If no acceptor is running,
       * a new acceptor is initialized on an arbitrary system generated port number or the port number as
       * defined by the environment variable @c DSI_COMMENGINE_PORT.
       */
      int32_t getIPPort();

      /// Attach to or create a new local channel as described by @c pid and @c chid.            
      std::tr1::shared_ptr<CChannel> attach(int32_t pid, int32_t chid);

      /// Attach to or craete a new tcp channel as described by @c host and @c port in host-byte-order.
      /// @param private_ Create a socket only known by the caller, no caching et al.
      std::tr1::shared_ptr<CChannel> attachTCP(uint32_t host, uint32_t port, bool private_ = false);
      
      CClient* findClient(int32_t id);
      CServer* findServer(int32_t id);

      /// Private structure hiding implementation details, not important for the interface
      struct Private;
      Private* d;
      
      /// internal forwarder function.
      static void pimplCCommEngineAddGenericDevice(void* d, int fd, DataFlowDirection dir,
                                                   const std::tr1::function<bool(IOResult)>& handler);
   };

}   //namespace DSI


#endif   // DSI_CCOMMENGINE_HPP

