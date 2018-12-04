/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "config.h"
#include "SocketConnectionContext.hpp"

#include <errno.h>

#include "Log.hpp"
#include "ServicebrokerServer.hpp"


SocketConnectionContext::SocketConnectionContext(ClientSpecificData& data, uint32_t nodeAddress, uint32_t localAddress, ServicebrokerClientBase& client)
 : ConnectionContext(data)
 , mClient(client) 
 , mNodeAddress(nodeAddress)
 , mLocalAddress(localAddress)
{
   // NOOP
}

