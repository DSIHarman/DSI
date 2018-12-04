/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_BASE_CREQUESTREADER_HPP
#define DSI_BASE_CREQUESTREADER_HPP


#include "dsi/DSI.hpp"
#include "dsi/private/CBuffer.hpp"

namespace DSI
{

   class CChannel;


   /// scatter gather reader for reading complete DSI requests
   class CRequestReader
   {
   public:

      CRequestReader(const DSI::MessageHeader& hdr, CChannel& chnl);

      /// receive all data for this request
      bool receiveAll();

      /// pointer to first byte after MessageHeader
      inline
      const char* buffer() const
      {
         return mBuf.gptr();
      }

      /// complete request size (including EventInfo and data payload)
      inline
      size_t size() const
      {
         return mBuf.size();
      }

   private:

      DSI::MessageHeader* mCurrent;
      CChannel& mChnl;

      Private::CBuffer mBuf;                  ///< buffer for payload data
      DSI::MessageHeader mHdr;      ///< temporary buffer for multi-frame messages
   };


   // -----------------------------------------------------------------------------------


   inline
   CRequestReader::CRequestReader(const DSI::MessageHeader& hdr, CChannel& chnl)
      : mCurrent(const_cast<DSI::MessageHeader*>(&hdr))
      , mChnl(chnl)
   {
      // NOOP
   }
}//namespace DSI

#endif   // DSI_BASE_CREQUESTREADER_HPP
