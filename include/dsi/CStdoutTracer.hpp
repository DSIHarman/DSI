/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_CSTDOUTTRACER_HPP
#define DSI_CSTDOUTTRACER_HPP


#include "dsi/Trace.hpp"


namespace DSI
{

namespace Trace
{

/**
 * Tracing implementation to trace all input and output DSI requests to console (stdout).
 * The current implementation does not support payload tracing since there is no generic
 * demarshalling mechanism implemented. Also, there is no filtering available for 
 * certain DSI requests, so all requests will be displayed (which may slow down the 
 * application dramatically).
 */
class CStdoutTracer : public IChannel
{
public: 

   /// @param enablePayload Enable the display of payload. Currently not implemented.
   explicit
   CStdoutTracer(bool enablePayload = false);
   
   ~CStdoutTracer();
   
   int open(const SFNDInterfaceDescription& iface, Direction dir, uint32_t updateId = DSI::INVALID_ID);
   void close(int handle);
   
   /// @return always true.
   bool isActive(int handle);   
   
   /// @return true if payload should be displayed, else false. 
   bool isPayloadEnabled(int handle);
   
   /// Writes one DSI request frame to the console. Currently without payload.
   void write(int handle, const DSI::MessageHeader* hdr, const DSI::EventInfo* info, const void* payload, size_t len);   
   
private:
   
   /// pimpl implementation detail
   struct Private;
   Private* d;   
};

}   // namespace Trace

}   // namespace DSI
   
   
#endif   // DSI_CSTDOUTTRACER_HPP