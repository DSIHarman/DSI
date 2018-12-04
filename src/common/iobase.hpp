/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_COMMON_IOBASE_HPP
#define DSI_COMMON_IOBASE_HPP


#include "dsi/private/CNonCopyable.hpp"

/*
  Policies: do not loop on EINTR on blocking _some functions, but loop on asynchronous function calls since there data
  should be ready to read so we do not block infinitely if we return to the action due to an EINTR.

  Desired behaviour of the Dispatcher (not yet fully implemented):

  It should be possible to arm for read and write simultaneously.
  When arming a device multiple times with the same policy the last wins, all previous armed io events will be deleted.
  When arming a device during a callback on the same fd that is currently under action a callback return value of 'true'
  should remove the user set callback and restore the original one.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/timerfd.h>

// unix sockets
#include <sys/un.h>

// ip sockets
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define IO_DEFAULT_BACKLOG 16

namespace DSI
{

   // forward decl
   class Dispatcher;

   template<typename DeviceT>
   struct IOImpl;

   template<typename AddressT, typename TraitsT>
   struct Socket;
   struct DescriptorDevice;


   namespace io
   {
      enum error_code
      {
         unset       = -3,
         unspecified = -2,   ///< unclassified error occurred
         eof         = -1,   ///< not an errno value

         ok          = 0,    ///< operation success, no error

         access_denied       = EACCES,
         would_block         = EAGAIN,
         invalid_argument    = EINVAL,
         too_many_open_files = EMFILE,
         bad_filedescriptor  = EBADF,
         connection_reset    = ECONNRESET,
         connection_refused  = ECONNREFUSED,
         is_connected        = EISCONN,
         not_connected       = ENOTCONN,
         connection_closed   = EPIPE,
         in_progress         = EINPROGRESS,
         interrupted         = EINTR
      };

      inline
      int getLastError()
      {
         return errno;
      }

      static /* inline? */
      io::error_code to_error_code(int errno_value)
      {
         io::error_code rc = unspecified;

         switch(errno_value)
         {
            case EACCES:          // fall-through
            case EAGAIN:          // fall-through
            case EINVAL:          // fall-through
            case EBADF:           // fall-through
            case EMFILE:          // fall-through
            case ECONNRESET:      // fall-through
            case ECONNREFUSED:    // fall-through
            case EISCONN:         // fall-through
            case ENOTCONN:        // fall-through
            case EPIPE:           // fall-through
            case EINPROGRESS:     // fall-through
            case EINTR:
               rc = (error_code)errno_value;
               break;
            default:
               /* NOOP */
               break;
         }

         return rc;
      }


      namespace detail
      {
         size_t calculate_total_length(const iovec* iov, size_t len);

         void adopt_iov_pointers(iovec*& iov, size_t& iovlen, size_t rc);

         inline
         error_code get_write_some_error_code(int rc)
         {
            return rc >= 0 ? io::ok : io::to_error_code(io::getLastError());
         }

         inline
         error_code get_write_all_error_code(int rc, int expected)
         {
            return rc == expected ? io::ok : io::to_error_code(io::getLastError());
         }

         inline
         error_code get_read_some_error_code(int rc)
         {
            return rc > 0 ? io::ok : (rc == 0 ? io::eof : io::to_error_code(io::getLastError()));
         }

         inline
         error_code get_read_all_error_code(int rc, int total, int expected)
         {
            return total == expected ? io::ok : (rc == 0 ? io::eof : io::to_error_code(io::getLastError()));
         }
      }

   }   // namespace io

   template<int Level, int Option>
   struct SocketOption
   {
      enum
      {
         eLevel = Level,
         eOption = Option
      };
   };

   template<int Option>
   struct SocketTimeoutOption : public SocketOption<SOL_SOCKET, Option>
   {
      inline
      SocketTimeoutOption(unsigned int timeoutMs)
         : timeoutMs_(timeoutMs)
      {
         // NOOP
      }

      const struct timeval* value() const
      {
         tv_.tv_sec = timeoutMs_ / 1000;
         tv_.tv_usec = (timeoutMs_ % 1000) * 1000;

         return &tv_;
      }

      enum { eSize = sizeof(struct timeval) };

      mutable struct timeval tv_;

      unsigned int timeoutMs_;
   };

   typedef SocketTimeoutOption<SO_SNDTIMEO> SocketSendTimeoutOption;
   typedef SocketTimeoutOption<SO_RCVTIMEO> SocketReceiveTimeoutOption;


   template<bool On>
   struct SocketReuseAddressOption : public SocketOption<SOL_SOCKET, SO_REUSEADDR>
   {
   public:

      enum { eSize = sizeof(int) };

      inline
      const char* value() const
      {
         reuse_ = On ? 1 : 0;
         return (const char*)&reuse_;
      }

   private:

      mutable int reuse_;
   };


   template<bool On>
   struct SocketTcpNoDelayOption : public SocketOption<IPPROTO_TCP, TCP_NODELAY>
   {
   public:

      enum { eSize = sizeof(int) };

      inline
      const char* value() const
      {
         nodelay_ = On ? 1 : 0;
         return (const char*)&nodelay_;
      }

   private:

      mutable int nodelay_;
   };


   template<int flags>
   struct FlagsFileControl
   {
   public:

      enum
      {
         eComplexControl = true
      };

      io::error_code eval(int fd) const
      {
         int rc = ::fcntl(fd, F_GETFL);
         if (rc >= 0)
         {
            rc = ::fcntl(fd, F_SETFL, rc | flags);
         }

         return rc == 0 ? io::ok : io::to_error_code(io::getLastError());
      }
   };
   
   template<int flags>
   struct ResetFlagsFileControl
   {
   public:

      enum
      {
         eComplexControl = true
      };

      io::error_code eval(int fd) const
      {
         int rc = ::fcntl(fd, F_GETFL);
         if (rc >= 0)
         {
            rc = ::fcntl(fd, F_SETFL, rc & ~flags);
         }

         return rc == 0 ? io::ok : io::to_error_code(io::getLastError());
      }
   };

   // set the non-blocking flag on the file descriptor
   typedef FlagsFileControl<O_NONBLOCK> NonBlockingFileControl;
   typedef ResetFlagsFileControl<O_NONBLOCK> ResetNonBlockingFileControl;


   /// simple control launcher
   template<bool ControlType>
   struct FileControlLauncher
   {
   public:

      template<typename ControlT>
      static inline
      io::error_code eval(int fd, const ControlT& control)
      {
         return ::fcntl(fd, ControlT::eCommand, control.value()) == 0 ? io::ok : io::to_error_code(io::getLastError());
      }
   };

   /// complex control launcher
   template<>
   struct FileControlLauncher<true>
   {
   public:

      template<typename ControlT>
      static inline
      io::error_code eval(int fd, const ControlT& control)
      {
         return control.eval(fd);
      }
   };


   struct NormalDescriptorTraits
   {
      typedef int fd_type;
      enum { Invalid = -1 };
   };




   struct StreamTraits
   {
      enum { socket_type = SOCK_STREAM };
   };


   struct SeqPacketTraits
   {
      enum { socket_type = SOCK_SEQPACKET };
   };


   struct DatagramTraits
   {
      enum { socket_type = SOCK_DGRAM };
   };

   typedef struct sockaddr sockaddr_t;
}//namespace DSI


#endif   // DSI_COMMON_IOBASE_HPP
