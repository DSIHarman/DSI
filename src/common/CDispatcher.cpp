/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "CDispatcher.hpp"



DSI::Dispatcher::Dispatcher(size_t capa )
  : max_(0)
  , capa_(capa)
  , fds_table_(new pollfd[capa_])
  , events_table_(new EventBase*[capa_])
{
  for (unsigned int i=0; i<capa_; ++i)
  {
    fds_table_[i].fd = -1;
  }
}


DSI::Dispatcher::~Dispatcher()
{
  delete[] fds_table_;

  for (unsigned int i=0; i<max_; ++i)
    delete events_table_[i];

  delete[] events_table_;
}


int DSI::Dispatcher::poll(int timeout_ms)
{
  int ret = ::poll(fds_table_, max_, timeout_ms);

  if (ret > 0)
  {
    for(unsigned int i=0; i<max_ && !finished_; ++i)
    {
      if (fds_table_[i].fd >= 0 && (fds_table_[i].revents != 0))
      {
        if (!events_table_[i]->eval(fds_table_[i].fd, fds_table_[i].revents))
        {
          // make sure to not delete a slot entry which was modified by the user,
          // e.g. by calling the sequence close() and then registering a new IO operation
          // which then could reuse this slot but with new revents mask set to zero.
          if (fds_table_[i].revents != 0)
            clearSlot(i);
        }
      }
    }
  }
  else if (ret < 0)
  {
    if (io::getLastError() == EINTR)
    {
      ret = 0;
    }
  }

  return ret;
}


void DSI::Dispatcher::doEnqueueEvent(int fd, EventBase* evt, short pollmask)
{
  int pos = -1;

  for(unsigned int i=0; i<max_; ++i)
  {
    if (fds_table_[i].fd == -1)
    {
      pos = i;
      break;
    }
  }

  if (pos == -1)
  {
    assert(capa_ >= max_);
    pos = max_;
    ++max_;
  }

  fds_table_[pos].fd = fd;
  fds_table_[pos].events = pollmask;
  fds_table_[pos].revents = 0;    // zero it so we won't get into trouble during iteration over fds
  events_table_[pos] = evt;
}


void DSI::Dispatcher::doRemoveAll(int fd)
{
  for (unsigned int i=0; i<max_; ++i)
  {
    if (fds_table_[i].fd == fd)
    {
      clearSlot(i);
      break;
    }
  }
}


void DSI::Dispatcher::clearSlot(int idx)
{
  fds_table_[idx].fd = -1;
  delete events_table_[idx];
  events_table_[idx] = nullptr;
}



