/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_DEVICES_HPP
#define DSI_DEVICES_HPP


#include "dsi/private/CNonCopyable.hpp"

/**
   Policies: do not loop on EINTR on blocking _some functions, but loop on asynchronous function calls since there data
   should be ready to read so we do not block infinitely if we return to the action due to an EINTR.

   Desired behaviour of the Dispatcher (not yet fully implemented):

   It should be possible to arm for read and write simultaneously. When arming a device multiple times with the same
   policy the last wins, all previous armed io events will be deleted.  When arming a device during a callback on the
   same fd that is currently under action a callback return value of 'true' should remove the user set callback and
   restore the original one.
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
#include "CDispatcher.hpp"
#include "CEndPoint.hpp"

namespace DSI
{

   template<typename TraitsT>
   class Device : public TraitsT
   {
   public:
      typedef typename TraitsT::fd_type fd_type;

      explicit
      Device(Dispatcher& dispatcher)
         : dispatcher_(&dispatcher)
         , fd_(TraitsT::Invalid)
      {
         // NOOP
      }


      inline
      Device()
         : dispatcher_(nullptr)
         , fd_(TraitsT::Invalid)
      {
         // NOOP
      }


      inline
      bool is_open() const
      {
         return fd_ != TraitsT::Invalid;
      }


      inline
      fd_type release()
      {
         fd_type rc = fd_;
         fd_ = TraitsT::Invalid;

         return rc;
      }

      inline
      fd_type fd()
      {
         return fd_;
      }

      inline
      Dispatcher* dispatcher()
      {
         return dispatcher_;
      }

      Dispatcher* dispatcher_;
      fd_type fd_;

   protected:

      Device& operator=(fd_type fd)
      {
         // the inheriter must close the original fd if valid
         assert(fd_ == TraitsT::Invalid);

         fd_ = fd;
         return *this;
      }

      inline
      Device(const Device& dev)
         : dispatcher_(dev.dispatcher_)
         , fd_(TraitsT::Invalid)
      {
         Device& that = const_cast<Device&>(dev);
         fd_ = that.release();
      }

      inline
      Device(fd_type fd)
         : dispatcher_(nullptr)
         , fd_(fd)
      {
         // NOOP
      }

      inline
      Device(Dispatcher& dispatcher, fd_type fd)
         : dispatcher_(&dispatcher)
         , fd_(fd)
      {
         // NOOP
      }

      ~Device()
      {
         // NOOP
      }
   };

   /**
    * IO operations for sockets (-> send and recv)
    */
   template<typename TraitsT>
   struct SocketIOFuncs
   {
   public:
      typedef typename TraitsT::fd_type fd_type;

      static inline
      ssize_t read(fd_type fd, void* buf, size_t len)
      {
         return ::recv(fd, buf, len, MSG_NOSIGNAL|MSG_DONTWAIT);
      }

      static inline
      ssize_t write(fd_type fd, const void* buf, size_t len)
      {
         return ::send(fd, buf, len, MSG_NOSIGNAL|MSG_DONTWAIT);
      }

      static inline
      ssize_t blocking_read(fd_type fd, void* buf, size_t len)
      {
         return ::recv(fd, buf, len, MSG_NOSIGNAL);
      }

      static inline
      ssize_t blocking_read_all(fd_type fd, void* buf, size_t len)
      {
         return ::recv(fd, buf, len, MSG_NOSIGNAL|MSG_WAITALL);
      }

      static inline
      ssize_t blocking_write(fd_type fd, const void* buf, size_t len)
      {
         return ::send(fd, buf, len, MSG_NOSIGNAL);
      }

      static inline
      ssize_t blocking_write(fd_type fd, const iovec* vec, size_t veclen)
      {
         struct msghdr hdr;
         ::memset(&hdr, 0, sizeof(hdr));
         hdr.msg_iov = const_cast<iovec*>(vec);
         hdr.msg_iovlen = veclen;

         return ::sendmsg(fd, &hdr, MSG_NOSIGNAL);
      }

      static inline
      ssize_t peeking_read(fd_type fd, void* buf, size_t len)
      {
         return ::recv(fd, buf, len, MSG_NOSIGNAL|MSG_DONTWAIT|MSG_PEEK);
      }

      static inline
      ssize_t peeking_write(fd_type fd, const void* buf, size_t len)
      {
         return ::send(fd, buf, len, MSG_NOSIGNAL|MSG_DONTWAIT|MSG_PEEK);
      }
   };


   /**
    * IO operations for normal devices (-> write and read)
    */
   template<typename TraitsT>
   struct PosixDeviceIOFuncs
   {
   public:
      typedef typename TraitsT::fd_type fd_type;
      static inline
      ssize_t read(fd_type fd, void* buf, size_t len)
      {
         return ::read(fd, buf, len);
      }

      static inline
      ssize_t write(fd_type fd, const void* buf, size_t len)
      {
         return ::write(fd, buf, len);
      }

      static inline
      ssize_t blocking_read(fd_type fd, void* buf, size_t len)
      {
         return ::read(fd, buf, len);
      }

      static inline
      ssize_t blocking_write(fd_type fd, const void* buf, size_t len)
      {
         return ::write(fd, buf, len);
      }
   };


   template<template<typename> class IOFuncsT>
   class IODevice
      : public Device<NormalDescriptorTraits>
   {
   protected:

      IODevice(fd_type fd)
         : base_type(fd)
      {
         // NOOP
      }

      IODevice(Dispatcher& disp, fd_type fd)
         : base_type(disp, fd)
      {
         // NOOP
      }

      IODevice& operator=(fd_type fd)
      {
         this->close();

         (void)base_type::operator=(fd);
         return *this;
      }

   public:

      typedef IOFuncsT<NormalDescriptorTraits> iofuncs_type;
      typedef Device<NormalDescriptorTraits> base_type;
      typedef NormalDescriptorTraits traits_type;


      explicit
      IODevice(Dispatcher& dispatcher)
         : base_type(dispatcher)
      {
         // NOOP
      }


      /**
       * Copy constructor supports move semantic for the file descriptor
       */
      inline
      IODevice(const IODevice& dev)
         : base_type(dev)
      {
         // NOOP
      }


      inline
      IODevice()
         : base_type()
      {
         // NOOP
      }


      inline
      ~IODevice()
      {
         (void)close();
      }


      io::error_code close()
      {
         int rc = 0;

         if (is_open())
         {
            if (this->dispatcher_)
               this->dispatcher_->removeAll(*this);

            do
            {
               rc = ::close(fd_);
            }
            while(rc < 0 && io::getLastError() == EINTR);

            fd_ = Invalid;
         }

         return rc == 0 ? io::ok : io::to_error_code(io::getLastError());
      }


      inline
      operator const void*() const
      {
         return is_open() ? this : nullptr;
      }


      template<typename IOCompletionRoutineT>
      inline
      void async_read_some(void* buf, size_t len, IOCompletionRoutineT func)
      {
         assert(this->dispatcher_);
         this->dispatcher_->enqueueEvent(fd_, new ReadSomeEvent<IOCompletionRoutineT, iofuncs_type>(buf, len, func));
      }


      template<typename IOCompletionRoutineT>
      inline
      void async_write_some(const void* buf, size_t len, IOCompletionRoutineT func)
      {
         assert(this->dispatcher_);
         this->dispatcher_->enqueueEvent(fd_, new WriteSomeEvent<IOCompletionRoutineT, iofuncs_type>(buf, len, func));
      }


      template<typename IOCompletionRoutineT>
      inline
      void async_read_all(void* buf, size_t len, IOCompletionRoutineT func)
      {
         assert(this->dispatcher_);
         this->dispatcher_->enqueueEvent(fd_, new ReadAllEvent<IOCompletionRoutineT, iofuncs_type>(buf, len, func));
      }


      template<typename IOCompletionRoutineT>
      inline
      void async_write_all(const void* buf, size_t len, IOCompletionRoutineT func)
      {
         assert(this->dispatcher_);
         this->dispatcher_->enqueueEvent(fd_, new WriteAllEvent<IOCompletionRoutineT, iofuncs_type>(buf, len, func));
      }


      inline
      void cancel_read()
      {
         assert(this->dispatcher_);
         this->dispatcher_->template removeEvent<ReadEventTraits<void> >(fd_);
      }


      inline
      void cancel_write()
      {
         assert(this->dispatcher_);
         this->dispatcher_->template removeEvent<WriteEventTraits<void> >(fd_);
      }


      inline
      ssize_t read_some(void* buf, size_t len, io::error_code& ec)
      {
         ssize_t rc = iofuncs_type::blocking_read(fd_, buf, len);
         ec = io::detail::get_read_some_error_code(rc);
         return rc;
      }


      void read_all(void* buf, size_t len, io::error_code& ec)
      {
         size_t total = 0;
         ssize_t rc;

         do
         {
            rc = iofuncs_type::blocking_read_all(fd_, (char*)buf + total, len - total);
            if (rc > 0)
               total += rc;
         }
         while ((rc < 0 && io::getLastError() == EINTR) || (rc > 0 && total < len));

         ec = io::detail::get_read_all_error_code(rc, total, len);
      }


      inline
      ssize_t write_some(const void* buf, size_t len, io::error_code& ec)
      {
         ssize_t rc = iofuncs_type::write(fd_, (const char*)buf, len);

         ec = io::detail::get_write_some_error_code(rc);
         return rc;
      }


      void write_all(const void* buf, size_t len, io::error_code& ec)
      {
         size_t total = 0;
         ssize_t rc;

         do
         {
            rc = iofuncs_type::blocking_write(fd_, (const char*)buf + total, len - total);
            if (rc > 0)
               total += rc;
         }
         while ((rc < 0 && io::getLastError() == EINTR) || (rc > 0 && total < len));

         ec = io::detail::get_write_all_error_code(total, len);
      }


      inline
      void write_all(const iovec* iov, size_t iovlen, io::error_code& ec)
      {
         size_t total = 0;
         size_t total_len = io::detail::calculate_total_length(iov, iovlen);

         ssize_t rc;
         iovec* _iov = const_cast<iovec*>(iov);

         do
         {
            rc = iofuncs_type::blocking_write(fd_, _iov, iovlen);

            if (rc < 0)
            {
               if (errno == EINTR)
                  rc = 0;
            }
            else
            {
               total += rc;

               if (total < total_len)
                  io::detail::adopt_iov_pointers(_iov, iovlen, rc);
            }
         }
         while(rc >= 0 && total < total_len);

         ec = io::detail::get_write_all_error_code(total, total_len);
      }


      template<typename ControlT>
      inline
      io::error_code fileControl(const ControlT& option)
      {
         return FileControlLauncher<ControlT::eComplexControl>::eval(fd_, option);
      }
   };


   template<typename AddressT, typename TraitsT>
   struct Socket
      : public IODevice<SocketIOFuncs>
      , public TraitsT
   {
      typedef IODevice<SocketIOFuncs> base_type;
      typedef AddressT endpoint_type;
      typedef base_type::traits_type traits_type;


      explicit
      Socket(Dispatcher& dispatcher)
         : base_type(dispatcher)
      {
         // NOOP
      }


      /**
       * Copy constructor supports move semantic for the file descriptor
       */
      inline
      Socket(const Socket& sock)
         : base_type(sock)
      {
         // NOOP
      }


      inline
      Socket()
         : base_type()
      {
         // NOOP
      }


      inline
      io::error_code open()
      {
         if (fd_ != Invalid)
            (void)close();

         fd_ = ::socket(AddressT::Domain, TraitsT::socket_type, 0);

         return is_open() ? io::ok : io::to_error_code(io::getLastError());
      }


      inline
      void open(io::error_code &ec)
      {
         ec = this->open();
      }


      /**
       * Return local address of socket.
       */
      endpoint_type getEndpoint()
      {
         endpoint_type endpoint;
         socklen_t len = sizeof(endpoint);

         (void)::getsockname(fd_, static_cast<struct sockaddr*>(endpoint), static_cast<socklen_t *>(&len));

         return endpoint;
      }


      /**
       * Return peer address of socket (the remote side).
       */
      endpoint_type getPeerEndpoint()
      {
         endpoint_type endpoint;
         socklen_t len = sizeof(endpoint);

         (void)::getpeername(fd_, static_cast<struct sockaddr*>(endpoint), static_cast<socklen_t *>(&len));

         return endpoint;
      }


      void connect(const AddressT& addr, io::error_code& ec)
      {
         ec = this->connect(addr);
      }
      

      io::error_code connect(const AddressT& addr)
      {
         ssize_t rc = ::connect(fd_, static_cast<const struct sockaddr*>(addr), addr.getSockAddrSize());
         return rc == 0 ? io::ok : io::to_error_code(io::getLastError());
      }


      template<typename OptionT>
      inline
      io::error_code setSocketOption(const OptionT& option)
      {
         return ::setsockopt(fd_, option.eLevel, option.eOption, option.value(), option.eSize) == 0 ?
            io::ok : io::to_error_code(io::getLastError());
      }
   };


   struct DescriptorDevice : public IODevice<PosixDeviceIOFuncs>
   {
      typedef IODevice<PosixDeviceIOFuncs> base_type;

      DescriptorDevice(Dispatcher& dispatcher)
         : base_type(dispatcher)
      {
         // NOOP
      }

      DescriptorDevice()
         : base_type()
      {
         // NOOP
      }

      DescriptorDevice(int fd)
         : base_type(fd)
      {
         // NOOP
      }

      DescriptorDevice(const DescriptorDevice& dev)
         : base_type(dev)
      {
         // NOOP
      }

      DescriptorDevice& operator=(int fd)
      {
         base_type::operator=(fd);
         return *this;
      }
   };


   template<typename AddressT>
   class StreamSocket : public Socket<AddressT, StreamTraits>
   {
      typedef Socket<AddressT, StreamTraits> base_type;

   private:

      io::error_code try_async_connect(const AddressT& addr)
      {
         assert(this->dispatcher_);

         io::error_code ec = base_type::fileControl(NonBlockingFileControl());

         if (ec == io::ok)
            ec = this->connect(addr);

         return ec;
      }

   public:

      using base_type::connect;
      
      explicit
      StreamSocket(Dispatcher& dispatcher)
         : Socket<AddressT, StreamTraits>(dispatcher)
      {
         // NOOP
      }

      StreamSocket()
         : Socket<AddressT, StreamTraits>()
      {
         // NOOP
      }

      /**
       * Connect to given address @c addr but for at maximum @c timeoutMs.
       */
      io::error_code connect(const AddressT& addr, unsigned int timeoutMs)
      {                                    
         io::error_code ec = try_async_connect(addr);

         if (ec == io::in_progress)
         {                             
            pollfd fds[1] = { { this->fd_, POLLOUT, 0 } };            
            
            // for now, we ignore EINTR interrupts
            int ret = ::poll(fds, 1, timeoutMs);
            if (ret == 0)
            {
               // normal timeout, exit
               ec = io::would_block;                  
            }
            else if (ret > 0)
            {
               // socket connected?
               int result = 0;
               socklen_t len = sizeof(result);

               if (::getsockopt(this->fd_, SOL_SOCKET, SO_ERROR, &result, &len) == 0)
               {
                  ec = result == 0 ? io::ok : io::to_error_code(result);                  
               }
               else
                  ec = io::to_error_code(errno);
            }
            else            
               ec = errno == EINTR ? io::would_block : io::to_error_code(errno);                        
         }
                  
         (void)base_type::fileControl(ResetNonBlockingFileControl());
         
         return ec;
      }
      
      
      /**
       * Connecting a socket asynchronously automatically switches the file descriptor to non-blocking mode. After
       * successful connection the flag is restored again so the file descriptor is blocking again.
       */
      template<typename ConnectCompletionRoutineT>
      inline
      void async_connect(const AddressT& addr, ConnectCompletionRoutineT func)
      {
         this->dispatcher_->enqueueEvent(this->fd_, new ConnectEvent<ConnectCompletionRoutineT>(try_async_connect(addr), func));
      }
   };


   template<typename AddressT>
   class DatagramSocket : public Socket<AddressT, DatagramTraits>
   {
   public:
      explicit
      DatagramSocket(Dispatcher& dispatcher)
         : Socket<AddressT, DatagramTraits>(dispatcher)
      {
         // NOOP
      }

      DatagramSocket()
         : Socket<AddressT, DatagramTraits>()
      {
         // NOOP
      }
      ssize_t write_some_to(const AddressT& to, const void* buf, size_t len, io::error_code& ec)
      {
         ssize_t rc = ::sendto(this->fd_, buf, len, MSG_NOSIGNAL, to, to.getSockAddrSize());
         ec = io::detail::get_write_some_error_code(rc);
         return rc;
      }

      ssize_t read_some_from(AddressT& from, void* buf, size_t len, io::error_code& ec)
      {
         socklen_t slen = from.getSockAddrSize();
         ssize_t rc = ::recvfrom(this->fd_, buf, len, MSG_NOSIGNAL, from, &slen);
         ec = io::detail::get_read_some_error_code(rc);
         return rc;
      }
   };


   template<typename AddressT>
   struct SequentialPacketSocket : public Socket<AddressT, SeqPacketTraits>
                                   // FIXME , public SeqPacketSocketMixin<AddressT, SequentialPacketSocket<AddressT> >
   {
      explicit
      SequentialPacketSocket(Dispatcher& dispatcher)
         : Socket<AddressT, SeqPacketTraits>(dispatcher)
      {
         // NOOP
      }

      SequentialPacketSocket()
         : Socket<AddressT, SeqPacketTraits>()
      {
         // NOOP
      }
   };
   template<typename AddressT>
   struct Acceptor : private Socket<AddressT, StreamTraits>
   {
   public:

      typedef Socket<AddressT, StreamTraits> base_type;
      typedef typename base_type::traits_type traits_type;

      // FIXME why is this visible to the public?
      using base_type::fd_;

      using base_type::getEndpoint;
      using base_type::is_open;
      using base_type::open;
      using base_type::close;
      using base_type::setSocketOption;
      using base_type::dispatcher;
      using base_type::operator const void*;
      using base_type::fd;


      Acceptor()
         : base_type()
      {
         (void)base_type::open();
      }

      explicit
      Acceptor(Dispatcher& dispatcher)
         : base_type(dispatcher)
      {
         (void)base_type::open();
      }

      explicit
      Acceptor(Dispatcher& dispatcher, typename base_type::fd_type fd)
         : base_type(dispatcher)
      {
         fd_ = fd;
      }

      void accept(Socket<AddressT, StreamTraits>& sock, io::error_code& ec, AddressT* addr = 0)
      {
         ec = this->accept(sock, addr);
      }


      io::error_code accept(Socket<AddressT, StreamTraits>& sock, AddressT* addr = 0)
      {
         AddressT tmp;
         socklen_t len = sizeof(tmp);

         sock.fd_ = ::accept(fd_, (struct sockaddr*)&tmp, &len);

         if (sock.fd_ != base_type::Invalid && addr)
            (*addr) = tmp;

         return sock.fd_ != base_type::Invalid ? io::ok : io::to_error_code(io::getLastError());
      }


      template<typename DeviceT, typename AcceptCompletionRoutineT>
      void async_accept(DeviceT& dev, AcceptCompletionRoutineT func)
      {
         assert(this->dispatcher_);
         this->dispatcher_->enqueueEvent(fd_, new AcceptEvent<DeviceT, AcceptCompletionRoutineT>(dev, func));
      }


      void cancel_accept()
      {
         assert(this->dispatcher_);
         this->dispatcher_->template removeEventAcceptEventTraits>(fd_);
      }

      void listen(io::error_code& ec, size_t backlog = IO_DEFAULT_BACKLOG)
      {
         ec = this->listen(backlog);
      }

      io::error_code listen(size_t backlog = IO_DEFAULT_BACKLOG)
      {
         ssize_t rc = ::listen(fd_, backlog);
         return rc == 0 ? io::ok : io::to_error_code(io::getLastError());
      }

      void bind(io::error_code& ec, const AddressT& address)
      {
         ec = this->bind(address);
      }

      io::error_code bind(const AddressT& address)
      {
         ssize_t rc = ::bind(fd_, address, address.getSockAddrSize());
         return rc == 0 ? io::ok : io::to_error_code(io::getLastError());
      }
   };
   namespace IPv4
   {
      typedef StreamSocket <IPv4::Endpoint> StreamSocket;
      typedef DatagramSocket<IPv4::Endpoint> DatagramSocket;
   }


   namespace Unix
   {

      typedef StreamSocket<Unix::Endpoint> StreamSocket;
      typedef DatagramSocket<Unix::Endpoint> DatagramSocket;
      typedef SequentialPacketSocket<Unix::Endpoint> SequentialPacketSocket;

   }   // namespace Unix


   namespace IPv4
   {
      typedef Acceptor<Endpoint> Acceptor;
   }


   namespace Unix
   {
      typedef Acceptor<Endpoint> Acceptor;
   }


   /**
    * How does the underlying OS support pipes?
    */
   struct PipeTraits
   {
      // use normal pipe
      typedef int fd_type;
      typedef PosixDeviceIOFuncs<NormalDescriptorTraits> iofuncs_type;
   };


   /**
    * A multiplexable pipe object, implemented in various OS-specific ways.
    */
   struct Pipe
   {
      typedef PipeTraits::fd_type fd_type;
      typedef PipeTraits::iofuncs_type iofuncs_type;

      enum
      {
         ReaderIdx = 0,
         WriterIdx = 1
      };

      /**
       * Create a pipe for reading on fds[0] and writing on fds[1].
       */
      static
      bool create(fd_type fds[2]);

      /**
       * Destroy both, read and write end.
       */
      static
      void destroy(fd_type fds[2]);

      /**
       * Write to the 'pipe'.
       */
      static inline
      ssize_t write(fd_type fd, const void* buffer, size_t len)
      {
         return iofuncs_type::blocking_write(fd, buffer, len);
      }

      /**
       * Read from the 'pipe'.
       */
      static inline
      ssize_t read(fd_type fd, void* buffer, size_t len)
      {
         return iofuncs_type::blocking_read(fd, buffer, len);
      }

   private:

      Pipe();   // no object possible
   };
}//namespace DSI

#include "CDevicesT.hpp"

#endif   // DSI_DEVICES_HPP
