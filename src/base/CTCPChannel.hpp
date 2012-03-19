/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_BASE_CTCPCHANNEL_HPP
#define DSI_BASE_CTCPCHANNEL_HPP


#include "CLocalChannel.hpp"


namespace DSI
{
   typedef CBaseChannel<IPv4::StreamSocket> CTCPChannel;
}//namespace DSI


#endif   // DSI_BASE_CTCPCHANNEL_HPP
