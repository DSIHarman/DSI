/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "io.hpp"



/*static*/
bool DSI::Pipe::create(fd_type fds[2])
{
   return ::pipe(fds) == 0;
}


/*static*/
void DSI::Pipe::destroy(fd_type fds[2])
{
   while(::close(fds[0]) && errno == EINTR);
   while(::close(fds[1]) && errno == EINTR);
}

namespace DSI
{
   namespace io
   {
      namespace detail
      {

         size_t calculate_total_length(const iovec* iov, size_t len)
         {
            size_t rc = 0;

            for (size_t i=0; i<len; ++i)
            {
               rc += iov[i].iov_len;
            }

            return rc;
         }


         void adopt_iov_pointers(iovec*& iov, size_t& iovlen, size_t rc)
         {
            int skip = 0;
            for(size_t i=0; i<iovlen && rc > 0; ++i)
            {
               if (rc >= iov[i].iov_len)
               {
                  ++skip;
                  rc -= iov[i].iov_len;
               }
               else
               {
                  iov[i].iov_base = (char*)iov[i].iov_base + rc;
                  iov[i].iov_len -= rc;
               }
            }

            iov += skip;
            iovlen -= skip;
         }

      }   /* namespace detail */

   }   /* namespace io */
}   /* namespace DSI */
