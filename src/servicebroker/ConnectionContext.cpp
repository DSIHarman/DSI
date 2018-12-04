/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "ConnectionContext.hpp"

#include <errno.h>

#include "Log.hpp"


namespace
{
   unsigned int nextId = 1 ;
}


ConnectionContext::ConnectionContext(ClientSpecificData& data, pid_t pid)
 : mData(data)
 , mSlave(false)
 , mId(nextId++)
 , mUid(SB_UNKNOWN_USER_ID) 
 //, mNodeAddress(nodeAddress)
 , mPid(pid)
{
   // NOOP
}

