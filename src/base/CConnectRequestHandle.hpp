/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_BASE_CCONNECTREQUESTHANDLE_HPP
#define DSI_BASE_CCONNECTREQUESTHANDLE_HPP


#include "dsi/private/CRequestHandle.hpp"

#include "CTCPChannel.hpp"
#include "DSI.hpp"


namespace DSI
{

   class CConnectRequestHandle : public Private::CRequestHandle<CChannel>
   {
   public:

      inline
      CConnectRequestHandle(const DSI::MessageHeader& hdr, std::tr1::shared_ptr<CChannel> chnl, const DSI::ConnectRequestInfo& info)
         : Private::CRequestHandle<CChannel>(hdr, chnl)
         , mInfo(info)
      {
         // NOOP
      }

      inline
      const DSI::ConnectRequestInfo& info() const
      {
         return mInfo;
      }
            
   private:

      const DSI::ConnectRequestInfo& mInfo;
   };


   class CTCPConnectRequestHandle : public Private::CRequestHandle<CChannel>  // FIXME drop template characteristics of base class
   {
   public:

      inline
      CTCPConnectRequestHandle(const DSI::MessageHeader& hdr, std::tr1::shared_ptr<CChannel> chnl, const DSI::TCPConnectRequestInfo& info)
         : Private::CRequestHandle<CChannel>(hdr, chnl)
         , mInfo(info)
      {
         // NOOP
      }

      inline
      const DSI::TCPConnectRequestInfo& info() const
      {
         return mInfo;
      }
      
      /// Explicitely cast the channel to TCP.
      inline
      CTCPChannel& getTCPChannel()
      {
         return *static_cast<CTCPChannel*>(mChnl.get());
      }

   private:

      const DSI::TCPConnectRequestInfo& mInfo;
   };
      
}   //namespace DSI


#endif   // DSI_BASE_CCONNECTREQUESTHANDLE_HPP
