/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_BASE_CSERVICEBROKER_HPP
#define DSI_BASE_CSERVICEBROKER_HPP


#include "dsi/clientlib.h"
#include "dsi/CServer.hpp"

#include <vector>

namespace DSI
{
   class CServicebroker
   {
   public:

      /**
       * @brief  Register an interface on service broker
       *
       * @param server The server that should be registered. Note that the internal field mServerID will be
       *               updated if the call is successful.
       *
       * @return true if registration was set successfully, false else.
       */
      static bool registerInterface( CServer& server, int32_t chid, const std::string& userGroup = "" );

      /**
       * @param serverlist The servers that should be registered. Note that the internal fields mServerID of the
       *                   CServer will be updated if the call is successful.
       */
      static bool registerInterface( std::vector<CServer*>& serverlist, int32_t chid );

      /**
       * @param server The server that should be registered as TCP server. Note that the internal field
       *               mTCPServerID will be updated if the call is successful.
       */
      static bool registerInterfaceTCP( CServer& server, uint32_t address, uint32_t port );

      /**
       * @brief  Unregister an interface on service broker
       * @param  the component address of the component to be unregistered
       * @param  the server's service ID
       */
      static void unregisterInterface( const SPartyID& serverID );

      /**
       * @brief  Set a server available notification
       * @param  The address of the component that is requested
       * @return the notification id on success or 0 on failure
       */
      static notificationid_t setServerAvailableNotification( SFNDInterfaceDescription& ifDescription, int32_t chid, int32_t pulseValue );

      /**
       * @brief  Set a client detach Notificationm
       * @param  The client ID
       * @return The notification ID (0 if request failed)
       */
      static notificationid_t setClientDetachNotification( const SPartyID &clientID, int32_t chid, int32_t pulseValue );

      /**
       * @brief  Set a service broker notification to be informed
       *         when a server is disconnected.
       * @param  the server's service ID
       * @param  the channel id
       * @param  the pulse value
       * @return true if notification was set successfully, false else
       */
      static notificationid_t setServerDisconnectNotification( const SPartyID &serverID, int32_t chid, int32_t pulseValue );

      /**
       * @brief  Attach to an imported interface
       */
      static int attachInterface( SFNDInterfaceDescription& ifDescription, SConnectionInfo &connInfo );
      static int attachInterfaceTCP( SFNDInterfaceDescription& ifDescription, STCPConnectionInfo &connInfo );

      /**
       * @brief  Dettach from an imported interface
       * @param  the ID of the attached client
       */
      static void detachInterface( const SPartyID& clientID );

      /**
       * @brief  Clear a service broker notification (server or client related)
       * @param  The notification ID
       */
      static void clearNotification( notificationid_t notificationID );

      /**
       * @brief Closes the servicebroker handle so that the next call to one of the
       *        servicebroker functions a new handle will be opened. Make sure that
       *        all Proxies and Stubs are inactive before closing the servicebroker.
       */
      static void closeHandle();

      /**
       * @brief returns the handle to the servicebroker. If it is not open this
       *        function opens the handle first.
       *        The function will use the default mountpoint as given by the servicebroker
       *        client lib unless an environment variable DSISERVICEBROKER is set.
       */
      static int GetSBHandle();
   };
}//namespace DSI

#endif // DSI_BASE_CSERVICEBROKER_HPP
