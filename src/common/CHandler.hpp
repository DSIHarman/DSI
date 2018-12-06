/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_CALLBACKHANDLER_HPP
#define DSI_CALLBACKHANDLER_HPP


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
#include "iobase.hpp"

namespace DSI
{
   template<typename IOCompletionRoutineT>
   struct IOEventTraits
   {
      typedef IOCompletionRoutineT completion_routine_type;
   };

   template<typename IOCompletionRoutineT>
   struct ReadEventTraits : public IOEventTraits<IOCompletionRoutineT>
   {
      typedef void* buffer_type;
      enum { poll_mask = POLLIN };
   };


   template<typename IOCompletionRoutineT>
   struct WriteEventTraits : public IOEventTraits<IOCompletionRoutineT>
   {
      typedef const void* buffer_type;
      enum { poll_mask = POLLOUT };
   };


   struct AcceptEventTraits
   {
      enum { poll_mask = POLLIN };
   };
   
   struct ConnectEventTraits
   {
      enum { poll_mask = POLLOUT };
   };

   struct TimerEventTraits
   {
      enum { poll_mask = POLLIN };
   };



   class EventBase
   {
   public:
      virtual ~EventBase()
      {
         // NOOP
      }

      /// @return true if the event should be rearmed, else false
      virtual bool eval(int fd, short revents) = 0;
   };

   class GenericEventBase : public EventBase
   {
   public:

      /// result values depend on DataFlowDirection settings.
      enum Result
      {
         DataAvailable = 0,
         CanWriteNow,
         DeviceHungup,

         InvalidFileDescriptor,
         GenericError
      };

   protected:

      static
      Result toResultEnum(short revents)
      {
         if (revents & POLLERR)
            return GenericError;

         if (revents & (POLLHUP|POLLRDHUP))
            return DeviceHungup;

         if (revents & POLLNVAL)
            return InvalidFileDescriptor;

         if (revents & POLLIN)
            return DataAvailable;

         if (revents & POLLOUT)
            return CanWriteNow;

         return GenericError;
      }
   };


   template<typename HandlerT>
   class GenericEvent : public GenericEventBase
   {
   public:

      /// result values depend on DataFlowDirection settings.
      enum Result
      {
         DataAvailable = 0,
         CanWriteNow,
         DeviceHungup,

         InvalidFileDescriptor,
         GenericError
      };

      explicit
      GenericEvent(const HandlerT& handler)
         : handler_(handler)
      {
         // NOOP
      }

      bool eval(int /*fd*/, short revents)
      {
         return handler_(toResultEnum(revents));
      }

   private:

      HandlerT handler_;
   };


   template <typename TraitsT, typename ImplementorT>
   class IOEventBase : public EventBase, public TraitsT
   {
   public:
      typedef TraitsT traits_type;

      IOEventBase(typename TraitsT::buffer_type buf, size_t len, typename TraitsT::completion_routine_type func)
         : buf_(buf)
         , len_(len)
         , func_(func)
      {
         // NOOP
      }

      bool evalPollErrors(short revents)
      {
         bool rearm = false;

         if (revents & POLLERR)
         {
            rearm = func_(0, io::unspecified);
         }
         else if (revents & (POLLHUP|POLLRDHUP))
         {
            rearm = func_(0, io::eof);
         }
         else if (revents & POLLNVAL)
         {
            rearm = func_(0, io::bad_filedescriptor);
         }

         return rearm;
      }


      bool eval(int fd, short revents)
      {
         if (revents & TraitsT::poll_mask)
         {
            // FIXME remove this curiously recurring template pattern - it's too curious
            return ((ImplementorT*)this)->doEval(fd);
         }
         else
            return evalPollErrors(revents);
      }


      typename traits_type::buffer_type buf_;
      size_t len_;

      typename traits_type::completion_routine_type func_;
   };


   // Wozu!!!!!!
   template<typename TraitsT, typename ImplementorT>
   class IOEvent : public IOEventBase<TraitsT, ImplementorT>
   {
   public:
      IOEvent(typename TraitsT::buffer_type buf, size_t len, typename TraitsT::completion_routine_type func)
         : IOEventBase<TraitsT, ImplementorT>(buf, len, func)
      {
         // NOOP
      }
   };


   class ConnectEventBase : public EventBase
   {
   public:
      inline
      ConnectEventBase(io::error_code ec)
         : ec_(ec)
      {
         // NOOP
      }

   protected:

      io::error_code doEval(int fd);
   private:

      io::error_code ec_;
   };

   template<typename ConnectCompletionRoutineT>
   class ConnectEvent : public ConnectEventBase
   {
   public:
   
      typedef ConnectEventTraits traits_type;
      typedef int fd_type;
      
      inline
      ConnectEvent(io::error_code ec, ConnectCompletionRoutineT func)
         : ConnectEventBase(ec)
         , func_(func)
      {
         // NOOP
      }

      bool eval(int fd, short /*revents*/)
      {
         func_(doEval(fd));
         return false;   // don't rearm the command
      }

      ConnectCompletionRoutineT func_;
   };


   template<typename IOCompletionRoutineT, typename IOFuncsT>
   class ReadSomeEvent : public IOEvent<ReadEventTraits<IOCompletionRoutineT>, ReadSomeEvent<IOCompletionRoutineT, IOFuncsT> >
   {
   public:
      typedef IOEvent<ReadEventTraits<IOCompletionRoutineT>, ReadSomeEvent<IOCompletionRoutineT, IOFuncsT> > base_type;
      typedef typename IOFuncsT::fd_type fd_type;

      ReadSomeEvent(typename base_type::buffer_type buf, size_t len, IOCompletionRoutineT func)
         : base_type(buf, len, func)
      {
         assert(len > 0);
      }

      bool doEval(fd_type fd)
      {
         ssize_t len = 0;
         do
         {
            len = IOFuncsT::read(fd, this->buf_, this->len_);
         }
         while (len < 0 && io::getLastError() == EINTR);

         return this->func_(len, len == 0 ? io::eof : (len > 0 ? io::ok : io::to_error_code(io::getLastError())));
      }
   };


   template<typename IOCompletionRoutineT, typename IOFuncsT>
   class WriteSomeEvent : public IOEvent<WriteEventTraits<IOCompletionRoutineT>, WriteSomeEvent<IOCompletionRoutineT, IOFuncsT> >
   {
   public:
      typedef IOEvent<WriteEventTraits<IOCompletionRoutineT>, WriteSomeEvent<IOCompletionRoutineT, IOFuncsT> > base_type;
      typedef typename IOFuncsT::fd_type fd_type;

      WriteSomeEvent(typename base_type::buffer_type buf, size_t len, IOCompletionRoutineT func)
         : base_type(buf, len, func)
      {
         // NOOP
      }

      bool doEval(fd_type fd)
      {
         ssize_t len = 0;

         do
         {
            len = IOFuncsT::write(fd, base_type::buf_, base_type::len_);
         }
         while (len < 0 && io::getLastError() == EINTR);

         return this->func_(len, len >= 0 ? io::ok : io::to_error_code(io::getLastError()));
      }
   };


   template<typename TraitsT, typename InheriterT>
   class IOAllEvent : public IOEventBase<TraitsT, InheriterT>
   {
   public:
      typedef TraitsT traits_type;

      IOAllEvent(typename TraitsT::buffer_type buf, size_t len,  typename TraitsT::completion_routine_type func)
         : IOEventBase<TraitsT, InheriterT>(buf, len, func)
         , cur_(buf)
         , total_(0)
      {
         // NOOP
      }

      bool reset()
      {
         cur_ = this->buf_;
         total_ = 0;

         return true;
      }

      typename traits_type::buffer_type cur_;
      size_t total_;
   };


   template<typename IOCompletionRoutineT, typename IOFuncsT>
   class ReadAllEvent : public IOAllEvent<ReadEventTraits<IOCompletionRoutineT>, ReadAllEvent<IOCompletionRoutineT, IOFuncsT> >
   {
   public:
      typedef IOAllEvent<ReadEventTraits<IOCompletionRoutineT>, ReadAllEvent<IOCompletionRoutineT, IOFuncsT> > base_type;
      typedef typename IOFuncsT::fd_type fd_type;

      ReadAllEvent(typename base_type::buffer_type buf, size_t len, IOCompletionRoutineT func)
         : base_type(buf, len, func)
      {
         assert(len > 0);
      }

      bool doEval(fd_type fd);
   };


   template<typename IOCompletionRoutineT, typename IOFuncsT>
   class WriteAllEvent : public IOAllEvent<WriteEventTraits<IOCompletionRoutineT>, WriteAllEvent<IOCompletionRoutineT, IOFuncsT> >
   {
   public:
      typedef IOAllEvent<WriteEventTraits<IOCompletionRoutineT>, WriteAllEvent<IOCompletionRoutineT, IOFuncsT> > base_type;
      typedef typename IOFuncsT::fd_type fd_type;

      WriteAllEvent(typename base_type::buffer_type buf, size_t len, IOCompletionRoutineT func)
         : base_type(buf, len, func)
      {
         // NOOP
      }

      bool doEval(fd_type fd);
   };


   template<typename DeviceT, typename AcceptCompletionRoutineT>
   class AcceptEvent : public EventBase
   {
   public:
      typedef AcceptEventTraits traits_type;
      typedef DeviceT device_type;
      typedef typename DeviceT::fd_type fd_type;

      inline
      AcceptEvent(DeviceT& dev, AcceptCompletionRoutineT func)
         : dev_(dev)
         , func_(func)
      {
         // NOOP
      }


      bool eval(int fd, short revents);

      DeviceT& dev_;
      AcceptCompletionRoutineT func_;
   };

   template<typename TimerTimeoutRoutineT>
   class NativeTimerEvent : public EventBase
   {
   public:
      typedef TimerEventTraits traits_type;
      typedef NormalDescriptorTraits::fd_type fd_type;    // need fd_type here only so we can use the base class

      inline
      NativeTimerEvent(unsigned int timeout_ms, TimerTimeoutRoutineT func)
         : timeout_ms_(timeout_ms)
         , func_(func)
      {
         // NOOP
      }


      bool arm(int fd)
      {
         struct itimerspec spec;
         ::memset(&spec, 0, sizeof(spec));
         spec.it_value.tv_sec = timeout_ms_ / 1000;
         spec.it_value.tv_nsec = (timeout_ms_ % 1000) * 1000000;

         return ::timerfd_settime(fd, 0, &spec, nullptr) == 0;
      }


      bool eval(int fd, short revents)
      {
         bool rc = false;

         if (revents & (POLLERR|POLLHUP|POLLRDHUP))
         {
            rc = func_(io::unspecified);
         }
         else if (revents & POLLNVAL)
         {
            rc = func_(io::bad_filedescriptor);
         }
         else
         {
            char buf[8];
            rc = func_(::read(fd, buf, sizeof(buf)) == sizeof(buf) ? io::ok : io::to_error_code(errno));
         }

         if (rc)
            rc = arm(fd);

         return rc;
      }

      unsigned int timeout_ms_;
      TimerTimeoutRoutineT func_;
   };
}//namespace DSI

#include "CHandlerT.hpp"
#endif   // DSI_CALLBACKHANDLER_HPP
