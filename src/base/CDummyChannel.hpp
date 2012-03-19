/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_BASE_CDUMMYCHANNEL_HPP
#define DSI_BASE_CDUMMYCHANNEL_HPP


#include "dsi/CChannel.hpp"


namespace DSI
{
   // forward decl
   class CClientConnectSM;
   
   
   class CDummyChannel : public CChannel
   {
      inline
      CDummyChannel()
      {
         // NOOP
      }

   public:

      static std::tr1::shared_ptr<CChannel> getInstancePtr();

      bool isOpen() const;

      bool sendAll(const void* data, size_t len);

      bool sendAll(const iov_t* iov, size_t iov_len);

      bool recvAll(void* buf, size_t len);
      
      void asyncRead(CClientConnectSM* /*sm*/);
   };

} //namespace DSI


#endif   // DSI_BASE_CDUMMYCHANNEL_HPP
