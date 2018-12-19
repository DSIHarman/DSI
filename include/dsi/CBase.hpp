/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_CBASE_HPP
#define DSI_CBASE_HPP

#include "dsi/DSI.hpp"

#include "dsi/private/CNonCopyable.hpp"


namespace DSI
{   
   // forward decl
   class CCommEngine;


   /**
    * @class CBase "CBase.hpp" "dsi/CBase.hpp"
    *
    * Base class for DSI clients and servers.
    */
   class CBase : public Private::CNonCopyable
   {
      friend class FindByPartyID;

   public:

      /**
       * Returns an application wide unique identifier (mainly for debugging purposes).
       */
      inline      
      int32_t getId() const
      {
         return mId;
      }
      
      /**
       * @return the communication engine the client or server is currently connected to or 0 in case 
       * the client or server is currently not connected to an engine. You may add the client or 
       * server to the communication engine by calling @c add().
       */
      inline
      CCommEngine* engine()
      {
         return mCommEngine;
      }

      /**
       * Virtual destructor.
       */
      virtual ~CBase();

      
   protected:

      /**
       * Constructor.
       * 
       * @param ifname       The interface name of the instance.
       * @param rolename     The rolename of the name instance.
       * @param majorVersion Interface major version       
       * @param minorVersion Interface minor version.
       */
      CBase( const char* ifname, const char* rolename, int majorVerion, int minorVersion );

      /**
       * The unique id of this client within the process. It has nothing to do
       * with exchanged identification tokens but is for debugging purpose only.
       */
      const int32_t mId;

      /**
       * Interface description as given by the constructor.
       */
      SFNDInterfaceDescription mIfDescription;

      /**
       * The client identifier as created by the servicebroker. This member is used differently within 
       * client and server context. On the client it will first be set to a valid value
       * after a successful attach to the interface. 
       * On server side the variable is mutable and will be set for each single request separately 
       * to the currently handled client.
       */
      SPartyID mClientID;

      /** 
       * The server identifier as created by the servicebroker. Therefore, the server id will
       * first be set on server side when the interface is registered at the servicebroker. Before that
       * it is invalid (0). On client side the id will be set first during an interface attach.
       * Moreover, it can point to the "server id" or "tcp server id" 
       * of a tcp enabled server, depending on the connection type the client is connected to the server.
       */
      SPartyID mServerID;

      /** 
       * Pointer to the communication engine the client or server is registered to.
       */
      CCommEngine *mCommEngine;

      /**
       * The current sequence number as transferred to a client/server during requests or responses.
       *
       * @noteBeware this is a mutable member so the object itself is not reentrant!
       */
      int32_t mCurrentSequenceNr;

      /** 
       * Converts an interface specific update id to a readable string for debugging purposes.
       */
      virtual const char* getUpdateIDString(uint32_t updateid) const;
   };
   
} //namespace DSI


#endif // DSI_CBASE_HPP

