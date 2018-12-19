/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "CTimer.hpp"



DSI::NativeTimer::NativeTimer(Dispatcher& dispatcher)
 : Device<NormalDescriptorTraits>(dispatcher)
{
  assert(&dispatcher);
  fd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC);
}


DSI::NativeTimer::~NativeTimer()
{
  if (is_open())
  {
    this->dispatcher_->removeAll(*this);
    while(::close(fd_) && errno == EINTR);
  }
}



