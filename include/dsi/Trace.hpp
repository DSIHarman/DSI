/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_TRACE_HPP
#define DSI_TRACE_HPP


#include "dsi/DSI.hpp"
#include "dsi/CChannel.hpp"

#include <sys/uio.h>


namespace DSI
{

namespace Trace
{

   enum Direction
   {
      In,
      Out
   };
   
   /**
    * Trace channel for DSI-Tracing. The user-provided implementation must 
    * implement
    */
   class IChannel
   {
   public:
      /**
       * Virtual destructor, actually does nothing.
       */
      virtual ~IChannel();
      
      /**
       * Open an output stream for the given interface.
       *
       * @return -1 if opening a stream for the handle does not work or any positive integer
       *         in case of success.
       */
      virtual int open(const SFNDInterfaceDescription& iface, Direction dir, uint32_t updateId = DSI::INVALID_ID) = 0;
      
      /**
       * Close the outputstream as described by @c handle.
       */
      virtual void close(int handle) = 0;
      
      /**
       * @return true if input streaming is enabled for the correspondant 
       *         interface and direction for which the handle was opened, else false.
       */
      virtual bool isActive(int handle) = 0;
            
      /**
       * @return true if payload streaming is enabled for the correspondant 
       *         interface, else false.
       */
      virtual bool isPayloadEnabled(int handle) = 0;
      
      /**
       * Write a complete DSI frame. The @c info pointer may be null in case
       * of non-data (dis-/connect request) or further-data requests (in case of large DSI requests).
       */
      virtual void write(int handle, const DSI::MessageHeader* hdr, 
                         const DSI::EventInfo* info, const void* payload, size_t len) = 0;      
   };

   
   /**
    * Initialize the trace subsystem with the given implementation channel.
    * You may only call this function once after application startup, calling it
    * multiple times will yield an assertion. 
    */
   void init(IChannel& chnl);
   
}   // namespace Trace

}   // namespace DSI


#endif   // DSI_TRACE_HPP
