/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/CRequestWriter.hpp"
#include "dsi/CChannel.hpp"
#include "dsi/private/util.hpp"

#include "CTraceManager.hpp"
#include "CDummyChannel.hpp"
#include "DSI.hpp"

#include <cassert>


DSI::CRequestWriter::CRequestWriter(DSI::CChannel& channel
                        , DSI::RequestType type
                        , DSI::Command cmd
                        , uint32_t id
                        , const SPartyID &clientID
                        , const SPartyID &serverID
                        , uint16_t proto_minor
                        , int32_t sequenceNr)
 : mChannel(channel)
 , mHeader(serverID, clientID, cmd, proto_minor)
 , mBuf(&Private::CBuffer::powerOf2) 
{
  mInfo.requestID = id;
  mInfo.requestType = type;
  mInfo.sequenceNumber = (sequenceNr == DSI::INVALID_SEQUENCE_NR) ? DSI::createId() : sequenceNr ;                 
}


DSI::CRequestWriter::CRequestWriter(DSI::CChannel& channel
                        , DSI::ResultType result
                        , DSI::Command cmd
                        , uint32_t id
                        , int32_t sequenceNr
                        , const SPartyID &clientID
                        , const SPartyID &serverID
                        , uint16_t proto_minor)
 : mChannel(channel)
 , mHeader(serverID, clientID, cmd, proto_minor)
 , mBuf(&Private::CBuffer::powerOf2) 
{
  mInfo.requestID = id;
  mInfo.responseType = result;
  mInfo.sequenceNumber = sequenceNr;
}      


DSI::CRequestWriter::CRequestWriter(DSI::CChannel& channel, DSI::Command cmd
                        , const SPartyID &clientID, const SPartyID &serverID
                        , uint16_t proto_minor)
 : mChannel(channel)
 , mHeader(serverID, clientID, cmd, proto_minor) 
{
  // NOOP
}


DSI::CRequestWriter::CRequestWriter()
 : mChannel(*DSI::CDummyChannel::getInstancePtr())
 , mBuf(&Private::CBuffer::powerOf2) 
{
  // NOOP
}


bool DSI::CRequestWriter::flush()
{
  assert(mHeader.type != 0);

  mHeader.type = 0;   // marker for EOF

  bool ret = false;

  // set header data
  size_t totalLength = mBuf.size();

  if (haveEventInfo())
    totalLength += sizeof(DSI::EventInfo);

  if (totalLength > DSI_PAYLOAD_SIZE)
  {
    mHeader.packetLength = DSI_PAYLOAD_SIZE ;
    mHeader.flags |= DSI_MORE_DATA_FLAG;
  }
  else
  {
    mHeader.packetLength = totalLength;
    mHeader.flags &= ~DSI_MORE_DATA_FLAG;
  }

  SFNDInterfaceDescription iface;
  uint32_t requestId = 0;

  iov_t iov[3] = {
    { &mHeader, sizeof(mHeader) },
    { nullptr, 0 },
    { const_cast<char*>(mBuf.gptr()), mHeader.packetLength }
  };

  if (CTraceManager::resolve(mHeader.clientID, mHeader.serverID, iface))
  {
    requestId = mInfo.requestID;

    COutputTraceSession session(iface, requestId);
    if (session.isActive())
    {
      if (haveEventInfo())
      {
        // data message
        session.write(&mHeader, &mInfo, 
          havePayload() && session.isPayloadEnabled() ? mBuf.gptr() : nullptr,
          session.isPayloadEnabled() ? mHeader.packetLength - sizeof(DSI::EventInfo) : 0);
      }
      else
      {
        // control message
        session.write(&mHeader, nullptr, havePayload() ? mBuf.gptr() : nullptr, mBuf.size());
      }         
    }
    else
      requestId = 0;
  }

  if (haveEventInfo())
  {      
    iov[1].iov_base = &mInfo;
    iov[1].iov_len = sizeof(mInfo);

    iov[2].iov_len -= sizeof(DSI::EventInfo);
  }

  ret = mChannel.sendAll(iov, 3);

  if( totalLength > DSI_PAYLOAD_SIZE )
  {
    // send remaining data to sub channel
    size_t dataSent = DSI_PAYLOAD_SIZE - sizeof(DSI::EventInfo);
    while(ret && dataSent < totalLength)
    {
      if( (totalLength - dataSent) > DSI_PAYLOAD_SIZE )
      {
        mHeader.packetLength = DSI_PAYLOAD_SIZE;
        mHeader.flags |= DSI_MORE_DATA_FLAG;
      }
      else
      {
        mHeader.packetLength = totalLength - dataSent ;
        mHeader.flags &= ~DSI_MORE_DATA_FLAG;
      }

      iov[1].iov_base = const_cast<char*>(mBuf.gptr()) + dataSent;
      iov[1].iov_len = mHeader.packetLength;

      if (requestId != 0)
      {            
        COutputTraceSession session(iface, requestId);
        if (session.isPayloadEnabled())
        {
          session.write(&mHeader, nullptr, iov[1].iov_base, iov[1].iov_len);
        }            
      }   

      ret = mChannel.sendAll(iov, 2);

      dataSent += DSI_PAYLOAD_SIZE ;
    }
  }

  return ret;
}
