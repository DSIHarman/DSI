/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_CCHANNEL_HPP
#define DSI_CCHANNEL_HPP

#include <sys/uio.h>

#include <tr1/memory>   // shared_ptr

#include "dsi/DSI.hpp"

#include "dsi/private/CNonCopyable.hpp"

namespace DSI
{
   // forward decl
   class CClientConnectSM;

   
   /// @internal helper typedef
   typedef struct iovec iov_t;

   
   /**
    * @class CChannel "CChannel.hpp" "dsi/CChannel.hpp"
    *
    * CChannel is the end-point of a connnection over which the DSI data protocol messages are
    * flowing. The channel offers the logical connection between a DSI client and a DSI server. 
    * This class does not manage the OS native resource. The implementor within CCommEngine 
    * is responsible for closing the channels if they are no more needed.       
    */
   class CChannel : public Private::CNonCopyable
   {
   public:

      /**
       * The constructor effectively does nothing
       */
      CChannel();

      /**
       * The destructor is a NOOP. Just for polymorphy reasons.
       */
      virtual ~CChannel();

      /**
       * Validity operator.
       *
       * @return a valid non-NULL pointer in case of the channel is open, else 0.
       */
      inline
      operator const void*() const
      {
         return isOpen() ? this : 0;
      }

      /**
       * @return true in case of the channel is open, else false.
       */
      virtual bool isOpen() const = 0;

      /**
       * Blocking send all data given by @c data of length @c len.
       */
      virtual bool sendAll(const void* data, size_t len) = 0;

      /**
       * Blocking send all data given by @c iov of length @c len.
       */
      virtual bool sendAll(const iov_t* iov, size_t iov_len) = 0;
      
      /**
       * Blocking receive all data of length @c len and store it in @c buf.
       */      
      virtual bool recvAll(void* buf, size_t len) = 0;      
      
      /**
       * Internal asynchronous read operation.
       */
      virtual void asyncRead(CClientConnectSM* sm) = 0;
   };
   
}   //namespace DSI


#endif // DSI_CCHANNEL_HPP
