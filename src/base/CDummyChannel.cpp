/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "CDummyChannel.hpp"

#include <cassert>

#include "dsi/CServer.hpp"
#include "dsi/CIStream.hpp"


/*static*/
std::tr1::shared_ptr<DSI::CChannel> DSI::CDummyChannel::getInstancePtr()
{
  static std::tr1::shared_ptr<CChannel> chnl(new CDummyChannel);
  return chnl;
}


bool DSI::CDummyChannel::isOpen() const
{
  return false;
}


bool DSI::CDummyChannel::sendAll(const void* /*data*/, size_t /*len*/)
{
  assert(false);
  return false;
}


bool DSI::CDummyChannel::sendAll(const iov_t* /*iov*/, size_t /*iov_len*/)
{
  assert(false);
  return false;
}


bool DSI::CDummyChannel::recvAll(void* /*buf*/, size_t /*len*/)
{
  assert(false);
  return false;
}


void DSI::CDummyChannel::asyncRead(CClientConnectSM* /*sm*/)
{
  assert(false);   
}