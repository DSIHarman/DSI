/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/CIStream.hpp"
#include "dsi/Log.hpp"

#include "utf8.hpp"

#include <cassert>
#include <iostream>
#include <errno.h>


TRC_SCOPE_DEF( dsi_base, CIStream, receive );


DSI::CIStream::CIStream(const char* payload, size_t len)
 : mData(payload)
 , mSize(len)
 , mOffset(0)
 , mError(0)
{
   // NOOP
}


void DSI::CIStream::read( std::wstring& str )
{
   if( 0 == mError )
   {
      uint32_t numOfBytes = 0;
      read(numOfBytes);

      // use a local buffer so that a single read() suffices
      if( 0 == mError && numOfBytes > 0 )
      {
         if(numOfBytes <= (mSize-mOffset))
         {
            std::string tmp(mData + mOffset, numOfBytes-1);
            str = fromUTF8(tmp);

            mOffset += numOfBytes ;
         }
         else
         {
            mError = ERANGE ;
         }
      }
      else
         str.clear();
   }
}


void DSI::CIStream::read( std::string& buf )
{
   if( 0 == mError )
   {
      uint32_t numOfBytes = 0;
      read( numOfBytes );

      // use a local buffer so that a single read() suffices
      if( 0 == mError && numOfBytes > 0 )
      {
         if(numOfBytes <= (mSize-mOffset))
         {
            std::string tmp(mData + mOffset, numOfBytes);   // STL string will store any data, even 0's
            buf = tmp;

            mOffset += numOfBytes ;
         }
         else
            mError = ERANGE ;
      }
      else
         buf.clear();
   }
}
