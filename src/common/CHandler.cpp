/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "CHandler.hpp"


DSI::io::error_code DSI::ConnectEventBase::doEval(int fd)
{
  if (ec_ == io::in_progress)
  {
    // retrieve error value for last socket operation...
    int success;
    socklen_t len = sizeof(success);

    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &success, &len) == 0 && len == sizeof(success))
    {
      ec_ = success == 0 ? io::ok : io::to_error_code(success);
    }
    else
      ec_ = io::unspecified;
  }

  // reset non-blocking flag
  int rc = ::fcntl(fd, F_GETFL);   
  if (rc >= 0)
    (void)::fcntl(fd, F_SETFL, rc & ~O_NONBLOCK);   

  return ec_;
}

