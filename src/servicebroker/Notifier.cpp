/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include <errno.h>
#include <stdint.h>

#include "Notifier.hpp"
#include "config.h"

#include "JobQueue.hpp"
#include "Log.hpp"
#include "dsi/private/static_assert.hpp"


Notifier::Notifier(int fd)
 : fd_(-1) 
 , prio_(0)
{ 
   fd_ = fd;

   if (fd_ == -1)   
      Log::error( "Notifier: error connecting to servicebroker channel" );   
}


Notifier::~Notifier()
{
   while(::close(fd_) && errno == EINTR);
}


void Notifier::jobFinished(Job& job)
{
   DSI_STATIC_ASSERT(sizeof(&job) == 4);   
   
   InternalNotification n = { PULSE_JOB_EXECUTED, (int32_t)&job };
   while(::write(fd_, &n, sizeof(n)) < 0 && errno == EINTR);  // FIXME loop correctly?   
}
            

void Notifier::masterConnected()
{
   InternalNotification n = { PULSE_MASTER_CONNECTED, 0 };
   while(::write(fd_, &n, sizeof(n)) < 0 && errno == EINTR);
}


void Notifier::masterDisconnected()
{
   InternalNotification n = { PULSE_MASTER_DISCONNECTED, 0 };
   while(::write(fd_, &n, sizeof(n)) < 0 && errno == EINTR);
}


void Notifier::deliverPulse(int code, int value)
{
   InternalNotification n = { code, value };
   while(::write(fd_, &n, sizeof(n)) < 0 && errno == EINTR);
}

