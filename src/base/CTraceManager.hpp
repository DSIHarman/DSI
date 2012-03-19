/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_BASE_CTRACEMANAGER_HPP
#define DSI_BASE_CTRACEMANAGER_HPP

#include "dsi/DSI.hpp"
#include "dsi/Trace.hpp"
#include "dsi/CChannel.hpp"


namespace DSI
{


class CTraceManager
{
   friend class CTraceSession;
   
public:

   static void init(Trace::IChannel& channel);
      
   static void add(const SPartyID& id, const SFNDInterfaceDescription& iface);
   
   static void remove(const SPartyID& id);
   
   static bool resolve(const SPartyID& clientId, const SPartyID& serverId, 
                       SFNDInterfaceDescription& iface);
   
private:

   struct Private;
   static Private* d;
};


// -------------------------------------------------------------------------------------


/**
 * A trace session for tracing one single DSI request which is not correlated to a 
 * data request, e.g. a connect or disconnect request. 
 */
class CTraceSession
{   
public:

   CTraceSession(const SFNDInterfaceDescription& iface, DSI::Trace::Direction dir = DSI::Trace::Out);   
   ~CTraceSession();
      
   bool isPayloadEnabled() const;      
   bool isActive() const;
   
   void write(const DSI::MessageHeader* hdr, const DSI::EventInfo* info = 0, const void* payload = 0, size_t len = 0);
   
protected:

   CTraceSession(const SFNDInterfaceDescription& iface, DSI::Trace::Direction dir, uint32_t updateId);    
      
   int mHandle;   
   DSI::Trace::Direction mDir;
};


/**
 * A trace session for tracing an incoming DSI request.
 */
class CInputTraceSession : public CTraceSession
{
public: 

   explicit
   CInputTraceSession(const SFNDInterfaceDescription& iface);
   
   CInputTraceSession(const SFNDInterfaceDescription& iface, uint32_t updateId);   
};


/**
 * A trace session for tracing an outgoing DSI request.
 */
class COutputTraceSession : public CTraceSession
{
public:

   COutputTraceSession(const SFNDInterfaceDescription& iface, uint32_t updateId);      
};


}   // namespace DSI


#endif   // DSI_BASE_CTRACEMANAGER_HPP
