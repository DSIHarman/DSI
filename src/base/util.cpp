/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/private/util.hpp"


namespace /*anonymous*/
{
   volatile uint32_t NextID = 1 ;
}


uint32_t DSI::createId()
{
   return __sync_fetch_and_add( &NextID, 1 );
}


