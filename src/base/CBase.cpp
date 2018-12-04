/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/CBase.hpp"

#include "DSI.hpp"
#include "dsi/private/util.hpp"

#include <cstdio>


DSI::CBase::CBase( const char* ifname, const char* rolename, int majorVerion, int minorVersion )
   : mId( DSI::createId() )
   , mCommEngine( 0 )
   , mCurrentSequenceNr( DSI::INVALID_SEQUENCE_NR )
{
   mClientID = 0 ;
   mServerID = 0 ;
   mIfDescription.version.majorVersion = majorVerion ;
   mIfDescription.version.minorVersion = minorVersion ;
   sprintf( mIfDescription.name, "%s.%s", rolename, ifname );
}


DSI::CBase::~CBase()
{
   mCommEngine = 0 ;
}


const char* DSI::CBase::getUpdateIDString(uint32_t /*updateid*/) const
{
   return "" ;
}
