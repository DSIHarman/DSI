/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "config.h"

#include "TCPMasterNotificationReceiver.hpp"
#include "Log.hpp"
#include "Notifier.hpp"
#include "Servicebroker.hpp"

#include "../common/bind.hpp"

using namespace DSI;

TCPMasterNotificationReceiver::TCPMasterNotificationReceiver(IPv4::StreamSocket& sock, Notifier* notifier)
 : mBuf()
 , mSock(sock)
{
  (void)notifier;   // get rid of warnings and lint
}


void TCPMasterNotificationReceiver::start()
{
  mSock.async_read_all((char*)&mBuf, sizeof(mBuf), bind3(&TCPMasterNotificationReceiver::handleRead,
                                       std::tr1::ref(*this), std::tr1::placeholders::_1,
                                       std::tr1::placeholders::_2));
}


bool TCPMasterNotificationReceiver::handleRead(size_t /*amount*/, io::error_code err)
{
  bool rc = true;

  if (err == io::ok)
  {
    Log::message(3, "Received notification code=%d, value=%d", mBuf.code, mBuf.value);

    // in this case there is only one thread for all socket communication
    Servicebroker::getInstance().dispatch(mBuf.code, mBuf.value);
  }
  else if (err == io::eof)
  {
    Log::message(2, "TCP notification interface closed");

    delete this;
    rc = false;
  }
  else
  {
    Log::warning(1, "Error occurred on device fd=%d: err=%d\n", mSock.fd(), (int)err);

    delete this;
    rc = false;
  }

  return rc;
}

