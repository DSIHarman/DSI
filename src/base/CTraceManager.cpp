/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include <map>
#include <cassert>

#include "CTraceManager.hpp"
#include "RecursiveMutex.hpp"
#include "LockGuard.hpp"


struct DSI::CTraceManager::Private
{
   std::map<SPartyID, SFNDInterfaceDescription> mRegistry;
   RecursiveMutex mMutex;
   
   DSI::Trace::IChannel* mChannel;   
};


/*static*/
DSI::CTraceManager::Private* DSI::CTraceManager::d = nullptr;


// ---------------------------------------------------------------------------------------------


/*static*/
void DSI::CTraceManager::init(DSI::Trace::IChannel& channel)
{
   assert(d == nullptr);
   
   d = new DSI::CTraceManager::Private;
   d->mChannel = &channel;   
}

   
/*static*/ 
void DSI::CTraceManager::add(const SPartyID& id, const SFNDInterfaceDescription& iface)   
{
   if (d)
   {
      LockGuard<> lock(d->mMutex);
      
      d->mRegistry[id] = iface;
   }
}   


/*static*/
void DSI::CTraceManager::remove(const SPartyID& id)
{
   if (d)
   {
      LockGuard<> lock(d->mMutex);
      
      d->mRegistry.erase(id);
   }
}


/*static*/
bool DSI::CTraceManager::resolve(const SPartyID& clientId, const SPartyID& serverId, 
                                 SFNDInterfaceDescription& iface)
{
   if (d)
   {
      LockGuard<> lock(d->mMutex);
      
      std::map<SPartyID, SFNDInterfaceDescription>::const_iterator iter = d->mRegistry.find(clientId);
      if (iter == d->mRegistry.end())
      {
         iter = d->mRegistry.find(serverId);
         if (iter != d->mRegistry.end())
         {
            iface = iter->second;
            return true;
         }
      }
      else
      {
         iface = iter->second;
         return true;
      }
   }
   
   return false;
}
   

// -------------------------------------------------------------------------------------


DSI::CTraceSession::CTraceSession(const SFNDInterfaceDescription& iface, DSI::Trace::Direction dir)
 : mHandle(-1) 
{
   if (DSI::CTraceManager::d)
   {
      mHandle = DSI::CTraceManager::d->mChannel->open(iface, dir);
   }
}


DSI::CTraceSession::CTraceSession(const SFNDInterfaceDescription& iface, DSI::Trace::Direction dir, uint32_t updateId)
 : mHandle(-1) 
{
   if (DSI::CTraceManager::d)
   {
      mHandle = DSI::CTraceManager::d->mChannel->open(iface, dir, updateId);
   }
}


DSI::CTraceSession::~CTraceSession()
{
   if (mHandle >= 0)
   {
      DSI::CTraceManager::d->mChannel->close(mHandle);
      mHandle = -1;
   }
}


bool DSI::CTraceSession::isPayloadEnabled() const
{
   return mHandle >= 0 && DSI::CTraceManager::d->mChannel->isPayloadEnabled(mHandle);   
}


bool DSI::CTraceSession::isActive() const
{
   return mHandle >= 0 && DSI::CTraceManager::d->mChannel->isActive(mHandle);      
}


void DSI::CTraceSession::write(const DSI::MessageHeader* hdr, const DSI::EventInfo* info, const void* payload, size_t len)
{
   if (mHandle >= 0)
   {
      DSI::CTraceManager::d->mChannel->write(mHandle, hdr, info, payload, len);
   }
}


// -------------------------------------------------------------------------------


DSI::CInputTraceSession::CInputTraceSession(const SFNDInterfaceDescription& iface, uint32_t updateId)
 : DSI::CTraceSession(iface, DSI::Trace::In, updateId)
{
   // NOOP
}


DSI::CInputTraceSession::CInputTraceSession(const SFNDInterfaceDescription& iface)
 : DSI::CTraceSession(iface, DSI::Trace::In, DSI::INVALID_ID)
{
   // NOOP
}


// -------------------------------------------------------------------------------


DSI::COutputTraceSession::COutputTraceSession(const SFNDInterfaceDescription& iface, uint32_t updateId)
 : DSI::CTraceSession(iface, DSI::Trace::Out, updateId)
{
   // NOOP
}
