/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_AUTOBUFFER_HPP
#define DSI_SERVICEBROKER_AUTOBUFFER_HPP

#include <cassert>

#include "dsi/private/CNonCopyable.hpp"

#include "frames.h"
#include "string.h"


// lint -e1509 -save

/**
 * A buffer with a file like interface (read/write) with one 'file'-pointer, i.e.
 * the same pointer is used for reading and writing.
 * You may use the buffer access functions like ptr() or begin() in order to
 * store things in the buffer and the bump() function to move the pointer appropriately.
 * You may also use the read() and write() functions which provide a offset-specific access.
 * The later two do not make use of the positioning pointer.
 */
template<size_t Size>
class AutoBuffer : public DSI::Private::CNonCopyable
{
public:

   inline
   AutoBuffer()
      : buf_((char*)internal_)
      , capa_(Size)
      , ptr_(buf_)
   {
      // NOOP
   }

   inline
   ~AutoBuffer()
   {
      reset();
   }

   /// current 'file' pointer
   inline
   void* ptr()
   {
      return ptr_;
   }

   inline
   void* begin()
   {
      return buf_;
   }

   inline
   size_t pos() const
   {
      return ptr_ - buf_;
   }

   inline
   void set(size_t off)
   {
      ptr_ = buf_ + off;
   }

   inline
   size_t capacity() const
   {
      return capa_;
   }

   void reserve(size_t len)
   {
      // FIXME make this smoother for big data amounts
      size_t off = ptr_ - buf_;

      if (len > capa_)
      {
         if (len > Size)
         {
            char* buf = new char[len];

            ::memcpy(buf, buf_, capa_);

            if (buf_ != (char*)internal_)
               delete[] buf_;

            buf_ = buf;
            capa_ = len;

            set(off);
         }
      }
   }

   /// bump up the positioning pointer with the given value
   inline
   void bump(size_t off)
   {
      ptr_ += off;
   }

   void reset()
   {
      if (buf_ != (char*)internal_)
         delete[] buf_;

      buf_ = (char*)internal_;
      capa_ = Size;
      ptr_ = buf_;
   }

   /// another possible access to the buffer
   int read(void* buf, size_t len, size_t offset)
   {
      int rc = -1;

      if ((int)(len + offset) <= request().fr_envelope.fr_size + (int)sizeof(sb_request_frame_t))
      {
         rc = len;
      }
      else
         rc = request().fr_envelope.fr_size + sizeof(sb_request_frame_t) - offset;

      if (rc > 0)
      {
         assert(buf);
         memcpy(buf, buf_ + offset, rc);
      }
      else
         rc = -1;

      return rc;
   }

   /// another possible access to the buffer
   int write(const void* buf, size_t len, size_t offset)
   {
      assert(buf);

      reserve(len + offset);
      memcpy(buf_ + offset, buf, len);

      return len;
   }

   /// look at the beginning of the buffer as if it was a request
   inline
   sb_request_frame_t& request()
   {
      sb_request_frame_t* f = (sb_request_frame_t*)buf_;
      return *f;
   }

   /// look at the beginning of the buffer as if it was a response
   inline
   sb_response_frame_t& response()
   {
      sb_response_frame_t* f = (sb_response_frame_t*)buf_;
      return *f;
   }

private:

   char internal_[Size] __attribute__((aligned(8)));

   char* buf_;
   size_t capa_;
   char* ptr_;
};

// lint -restore


// TODO make typelist and get largest input/output entry from list
typedef AutoBuffer<512> tAutoBufferType;


#endif   // DSI_SERVICEBROKER_AUTOBUFFER_HPP
