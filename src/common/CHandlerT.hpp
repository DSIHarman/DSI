/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_HANDLERT_HPP
#define DSI_HANDLERT_HPP


namespace DSI
{
   template<typename IOCompletionRoutineT, typename IOFuncsT>
   bool  ReadAllEvent<IOCompletionRoutineT,IOFuncsT>::doEval(fd_type fd)
   {
      ssize_t len = 0;

      do
      {
         len = IOFuncsT::read(fd, this->cur_, this->len_ - this->total_);
      }
      while (len < 0 && io::getLastError() == EINTR);

      if (len > 0)
      {
         this->cur_ = ((char*)this->buf_) + len;
         this->total_ += len;
      }

      if (this->total_ == this->len_ || len <= 0)
      {
         return this->func_(this->total_, len == 0 ? io::eof : (len > 0 ? io::ok : io::to_error_code(io::getLastError()))) && this->reset();
      }
      else
         return true;   // automatic rearm
   }


   template<typename IOCompletionRoutineT, typename IOFuncsT>
   bool  WriteAllEvent<IOCompletionRoutineT,IOFuncsT>::doEval(fd_type fd)
   {
      ssize_t len = 0;

      do
      {
         len = IOFuncsT::write(fd, this->cur_, this->len_ - this->total_);
      }
      while (len < 0 && io::getLastError() == EINTR);

      if (len > 0)
      {
         this->cur_ = ((const char*)this->buf_) + len;
         this->total_ += len;
      }

      if (this->total_ == this->len_ || len <= 0)
      {
         return this->func_(this->total_, len >= 0 ? io::ok : io::to_error_code(io::getLastError())) && this->reset();
      }
      else
         return true;   // automatic rearm
   }

   template<typename DeviceT, typename AcceptCompletionRoutineT>
   bool  AcceptEvent<DeviceT,AcceptCompletionRoutineT>::eval(int fd, short revents)
   {
      bool rearm = false;
      typename DeviceT::endpoint_type address;

      if (revents & POLLERR)
      {
         rearm = func_(address, io::unspecified);
      }
      else if (revents & (POLLHUP|POLLRDHUP))
      {
         rearm = func_(address, io::eof);
      }
      else if (revents & POLLNVAL)
      {
         rearm = func_(address, io::bad_filedescriptor);
      }
      else
      {
         socklen_t len = sizeof(typename DeviceT::endpoint_type::traits_type::addr_type);
         do
         {
            dev_.fd_ = ::accept(fd, (struct sockaddr*)&address.address_, &len);
         }
         while(dev_.fd_ == device_type::Invalid && io::getLastError() == EINTR);

         // short cut: rearm automatically if the other side closed the connection before we could accept it
         if (dev_.fd_ == device_type::Invalid && io::getLastError() == ECONNABORTED)
         {
            rearm = true;
         }
         else
            rearm = func_(address, dev_.fd_ != device_type::Invalid ? io::ok : io::to_error_code(io::getLastError()));
      }

      return rearm;
   }
}//namespace DSI

#endif   // DSI_HANDLERT_HPP
