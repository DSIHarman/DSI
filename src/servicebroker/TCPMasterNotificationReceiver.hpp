/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_TCPMASTERNOTIFICATIONRECEIVER_HPP
#define DSI_SERVICEBROKER_TCPMASTERNOTIFICATIONRECEIVER_HPP

#include "config.h"
#include "dsi/private/servicebroker.h"


#include "io.hpp"


// forward decl
class Notifier;


/**
 * The slave receives notifications from the master via this client interface.
 */
class TCPMasterNotificationReceiver
{
public:

   TCPMasterNotificationReceiver(DSI::IPv4::StreamSocket& sock, Notifier* notifier = nullptr);

   void start();

   bool handleRead(size_t amount, DSI::io::error_code err);

private:

   sb_pulse_t mBuf;
   DSI::IPv4::StreamSocket mSock;
};


#endif   // DSI_SERVICEBROKER_TCPMASTERNOTIFICATIONRECEIVER_HPP
