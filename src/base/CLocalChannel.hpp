/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_BASE_CLOCALCHANNEL_HPP
#define DSI_BASE_CLOCALCHANNEL_HPP


#include <cassert>

#include "dsi/CServer.hpp"
#include "dsi/CIStream.hpp"
#include "dsi/CChannel.hpp"

#include "io.hpp"
#include "CClientConnectSM.hpp"


namespace DSI
{

   /// local channel
   template<typename SocketT>
   struct ConnectRequestReader
   {
      static inline
      void eval(SocketT& /*sock*/, CClientConnectSM& /*sm*/)
      {
         // never called, but virtual interface so code is generated
         assert(false);
      }
   };

   /// TCP/IP channel specialization
   template<>
   struct ConnectRequestReader<IPv4::StreamSocket>
   {
      static inline
      void eval(IPv4::StreamSocket& sock, CClientConnectSM& sm)
      {
         sm.asyncRead(sock);
      }
   };
   
   
   /// real implementation for UNIX or TCP/IP channel 
   template<typename SocketT>
   class CBaseChannel : public CChannel
   {
   public:

      inline
      CBaseChannel(SocketT& sock)
       : mSock(sock)
      {
         // NOOP
      }


      inline
      CBaseChannel(typename SocketT::endpoint_type& ep)
       : mSock()
      {
         mSock.open(ep);
      }


      bool isOpen() const
      {
         return mSock.is_open();
      }


      bool sendAll(const void* data, size_t len)
      {
         io::error_code ec;
         mSock.write_all(data, len, ec);

         return ec == io::ok;
      }

      bool sendAll(const iov_t* iov, size_t iov_len)
      {
         assert(iov && iov_len);

         io::error_code ec = io::ok;
         mSock.write_all(iov, iov_len, ec);

         return ec == io::ok;
      }

      bool recvAll(void* buf, size_t len)
      {
         assert(buf && len);

         io::error_code ec;
         mSock.read_all(buf, len, ec);

         return ec == io::ok;
      }
            
      void asyncRead(CClientConnectSM* sm)
      {
         ConnectRequestReader<SocketT>::eval(mSock, *sm);
      }      

      inline
      typename SocketT::endpoint_type getPeerName()
      {
         return mSock.getPeerEndpoint();
      }

      inline
      typename SocketT::endpoint_type getLocalName()
      {
         return mSock.getEndpoint();
      }

   private:

      SocketT mSock;
   };


   typedef CBaseChannel<Unix::StreamSocket> CLocalChannel;
}//namespace DSI


#endif   // DSI_BASE_CLOCALCHANNEL_HPP
