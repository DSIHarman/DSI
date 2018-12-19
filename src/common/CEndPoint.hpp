/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_ENDPOINT_HPP
#define DSI_ENDPOINT_HPP


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

namespace DSI
{
   namespace IPv4
   {

      class EndpointTraits
      {
      public:
         typedef struct sockaddr_in addr_type;

         enum
         {
            Domain = PF_INET,
            AddressFamily = AF_INET
         };

         static inline
         bool equal(const addr_type& lhs, const addr_type& rhs)
         {
            return lhs.sin_addr.s_addr == rhs.sin_addr.s_addr && lhs.sin_port == rhs.sin_port;
         }

         static inline
         bool lower_than(const addr_type& lhs, const addr_type& rhs)
         {
            return lhs.sin_addr.s_addr < rhs.sin_addr.s_addr ||
               (lhs.sin_addr.s_addr == rhs.sin_addr.s_addr && lhs.sin_port < rhs.sin_port);
         }
      };

   }// namespace IPv4

   namespace Unix
   {

      class EndpointTraits
      {
      public:
         typedef struct sockaddr_un addr_type;

         enum
         {
            Domain = PF_UNIX,
            AddressFamily = AF_UNIX
         };

         static inline
         bool equal(const addr_type& lhs, const addr_type& rhs)
         {
            return ::memcmp(lhs.sun_path, rhs.sun_path, sizeof(rhs.sun_path)) == 0;
         }

         static inline
         bool lower_than(const addr_type& lhs, const addr_type& rhs)
         {
            return ::memcmp(lhs.sun_path, rhs.sun_path, sizeof(rhs.sun_path)) < 0;
         }
      };

   }// namespace Unix

   typedef struct sockaddr sockaddr_t;
   template<typename InheriterT, typename TraitsT>
   class Endpoint
   {
   public:
      template<typename DeviceT, typename AcceptCompletionRoutineT>
      friend struct AcceptEvent;

      typedef TraitsT traits_type;

      // do not initialize the endpoint
      Endpoint()
      {
         // NOOP
      }

      // zero out the endpoint
      explicit
      inline
      Endpoint(bool /*zero*/)
      {
         ::memset(&address_, 0, sizeof(address_));
      }

      inline
      operator const sockaddr_t*() const
      {
         return (const struct sockaddr*)&address_;
      }

      inline
      operator sockaddr_t*()
      {
         return (struct sockaddr*)&address_;
      }

      inline
      InheriterT& operator=(const typename traits_type::addr_type& rhs)
      {
         address_ = rhs;
         return *static_cast<InheriterT*>(this);
      }

      inline
      socklen_t getSockAddrSize() const
      {
         return sizeof(address_);
      }

      bool operator==(const Endpoint& ep) const
      {
         return traits_type::equal(address_, ep.address_);
      }

      bool operator<(const Endpoint& ep) const
      {
         return traits_type::lower_than(address_, ep.address_);
      }

   protected:
      typename traits_type::addr_type address_;
   };


   namespace IPv4
   {

      class Endpoint : public DSI::Endpoint<Endpoint, EndpointTraits>, public EndpointTraits
      {
      public:
         // construct an uninitialized (invalid) endpoint
         inline
         Endpoint()
            : DSI::Endpoint<Endpoint, EndpointTraits>(true)
         {
            // NOOP
         }


         /**
          * @param addr Formatted address as in the string 'ip:port'.
          */
         Endpoint(const char* addr)
            : DSI::Endpoint<Endpoint, EndpointTraits>(true)
         {
            // FIXME linux does not need to copy the address since
            // the ':' is good enough for an end of the address, QNX needs it.
            char tmp[32];
            ::memset(tmp, 0, sizeof(tmp));

            assert(addr);
            ::strncpy(tmp, addr, sizeof(tmp)-1);

            char* ptr = ::strchr(tmp, ':');
            if (ptr)
            {
               *ptr = '\0';
               ++ptr;

               address_.sin_family = EndpointTraits::AddressFamily;
               address_.sin_addr.s_addr = inet_addr(tmp);
               address_.sin_port = htons(::atoi(ptr));
            }
         }

         Endpoint(const char* addr, int port)
            : DSI::Endpoint<Endpoint, EndpointTraits>()
         {
            address_.sin_family = EndpointTraits::AddressFamily;
            address_.sin_addr.s_addr = inet_addr(addr);
            address_.sin_port = htons(port);
         }

         // any address
         Endpoint(int port)
            : DSI::Endpoint<Endpoint, EndpointTraits>()
         {
            address_.sin_family = EndpointTraits::AddressFamily;
            address_.sin_addr.s_addr = INADDR_ANY;
            address_.sin_port = htons(port);
         }

         /// ip and port in host byte order
         Endpoint(int ip, int port)
            : DSI::Endpoint<Endpoint, EndpointTraits>()
         {
            address_.sin_family = EndpointTraits::AddressFamily;
            address_.sin_addr.s_addr = htonl(ip);
            address_.sin_port = htons(port);
         }

         /**
          * Return port in host byte order.
          */
         inline
         int getPort() const
         {
            return ntohs(address_.sin_port);
         }

         /**
          * Return IP address in host byte order.
          */
         inline
         int getIP() const
         {
            return ntohl(address_.sin_addr.s_addr);
         }

         /// buf must be large enough to store xxx.xxx.xxx.xxx:xxxxx
         inline
         const char* toString(char* buf) const
         {
            (void)snprintf(buf, 24, "%s:%d", inet_ntoa(address_.sin_addr), ntohs(address_.sin_port));
            return buf;
         }

         /// ip and port in host byte order
         inline
         void set(int ip, int port)
         {
            address_.sin_family = EndpointTraits::AddressFamily;
            address_.sin_addr.s_addr = htonl(ip);
            address_.sin_port = htons(port);
         }

         using DSI::Endpoint<Endpoint, EndpointTraits>::operator=;
      };

   }// namespace IPv4

   namespace Unix
   {

      class Endpoint : public DSI::Endpoint<Endpoint, EndpointTraits>,
                       public EndpointTraits
      {
      public:
         // construct an uninitialized (invalid) endpoint
         inline
         Endpoint()
            : DSI::Endpoint<Endpoint, EndpointTraits>(true)
         {
            // NOOP
         }

         // either a normal file name (terminated 0 string) or a unix socket name in
         // anonymous namespace (0-byte at begin -> *name must be an array of length
         // sizeof(sockaddr_un.sun_path).
         Endpoint(const char* name)
            : DSI::Endpoint<Endpoint, EndpointTraits>(true)
         {
            assert(name);
            address_.sun_family = Unix::EndpointTraits::AddressFamily;

            if (name[0] != '\0')
            {
               memset(address_.sun_path, 0, sizeof(address_.sun_path));
               strncpy(address_.sun_path, name, sizeof(address_.sun_path) - 1);
            }
            else
            {
               size_t len = ::strnlen(name+1 , sizeof(address_.sun_path)-1);
               assert(len < sizeof(address_.sun_path) - 2);
               memcpy(address_.sun_path, name, len + 1);
            }
         }
      };

   }// namespace Unix

}//namespace DSI

//#include "CEndPointT.hpp"
#endif   // DSI_ENDPOINT_HPP
