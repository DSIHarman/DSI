/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/CStdoutTracer.hpp"

#include <iostream>
#include <cassert>
#include <map>

#include "RecursiveMutex.hpp"
#include "LockGuard.hpp"


namespace /*anonymous*/
{

  const char* commandToString(uint32_t cmd)
  {
    static const char* str[] = {
      "Invalid",
      "Unknown",
      "Unknown",
      "Unknown",
      "Unknown",
      "Unknown",
      "Unknown",
      "DataRequest",
      "DataResponse",
      "ConnectRequest",
      "DisconnectRequest",
      "ConnectResponse"
    };

    return cmd < sizeof(str)/sizeof(str[0]) ? str[cmd] : str[1];
  }

  inline
  std::ostream& operator<<(std::ostream& os, const SPartyID& id)
  {
    return os << id.s.localID << '.' << id.s.extendedID;   
  }   

  struct STraceHandle
  {      
    SFNDInterfaceDescription iface;
    DSI::Trace::Direction dir;
  };

}   // namespace


// ---------------------------------------------------------------------------------------


struct DSI::Trace::CStdoutTracer::Private
{   
  explicit
  Private(bool enablePayload)
   : mWithPayload(enablePayload)
   , mCurrent(0)
  {
    // NOOP
  }


  ~Private()
  {
    // NOOP
  }


  int open(const SFNDInterfaceDescription& iface, Direction dir, uint32_t /*updateId*/)
  {   
    LockGuard<> lock(mMutex);

    ++mCurrent;

    mHandles[mCurrent].iface = iface;
    mHandles[mCurrent].dir = dir;

    return mCurrent;
  }


  void close(int handle)
  {
    LockGuard<> lock(mMutex);

    mHandles.erase(handle);      
  }


  const STraceHandle* find(int handle)
  {
    std::map<int, STraceHandle>::const_iterator iter = mHandles.find(handle);
    return iter == mHandles.end() ? nullptr : &iter->second;
  }

  void printHeader(const DSI::MessageHeader& hdr, const STraceHandle& thdl)
  {
    std::cout 
      << std::endl << (thdl.dir == DSI::Trace::In ? "==>" : "<==")
      << " DSI v" << hdr.protoMajor << '.' << hdr.protoMinor << ' ' << commandToString(hdr.cmd) << std::endl
      << "    serverId     : " << hdr.serverID << std::endl
      << "    clientId     : " << hdr.clientID << std::endl         
      << "    interface    : " << thdl.iface.name << ':' << thdl.iface.version.majorVersion << '.' << thdl.iface.version.minorVersion << std::endl;
  }

  const bool mWithPayload;

private:

  RecursiveMutex mMutex;

  std::map<int, STraceHandle> mHandles;   
  unsigned int mCurrent;      
};


// ---------------------------------------------------------------------------------------


DSI::Trace::CStdoutTracer::CStdoutTracer(bool enablePayload)
 : d(new Private(enablePayload)) 
{
  // NOOP
}


DSI::Trace::CStdoutTracer::~CStdoutTracer()
{
  delete d;
}


int DSI::Trace::CStdoutTracer::open(const SFNDInterfaceDescription& iface, DSI::Trace::Direction dir, uint32_t updateId)
{   
  return d->open(iface, dir, updateId);   
}


void DSI::Trace::CStdoutTracer::close(int handle)
{
  d->close(handle);
}


bool DSI::Trace::CStdoutTracer::isActive(int /*handle*/)
{
  return true;
}


bool DSI::Trace::CStdoutTracer::isPayloadEnabled(int /*handle*/)
{
  return d->mWithPayload;
}


void DSI::Trace::CStdoutTracer::write(int handle, const DSI::MessageHeader* hdr, const DSI::EventInfo* info, const void* /*buf*/, size_t /*len*/)
{
  assert(hdr);   

  const STraceHandle* thdl = d->find(handle);
  if (thdl)
  {
    d->printHeader(*hdr, *thdl);

    switch(hdr->cmd)
    {
    case DataRequest:     
      {      
        assert(info);            
        std::cout 
          << "    requestType  : " << DSI::toString(info->requestType) << std::endl 
          << "    requestId    : " << info->requestID << std::endl
          << "    sequenceNr   : " << info->sequenceNumber << std::endl;         
      }
      break;

    case DataResponse:
      {
        assert(info);            
        std::cout 
          << "    responseType : " << DSI::toString(info->responseType) << std::endl 
          << "    requestId    : " << info->requestID << std::endl
          << "    sequenceNr   : " << info->sequenceNumber << std::endl;         
      }

    default:
      // NOOP
      break;
    }      
  }   
}
