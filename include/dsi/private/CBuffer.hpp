/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_PRIVATE_CBUFFER_HPP
#define DSI_PRIVATE_CBUFFER_HPP


#include <cstdlib>

#include "dsi/DSI.hpp"

#include "dsi/private/CNonCopyable.hpp"

namespace DSI
{

   namespace Private
   {

      /** 
       * @internal 
       *
       * Simple non-auto increasing CBuffer
       */
      class CBuffer : public CNonCopyable
      {
      public:

         /// calculate new capacity for stream
         typedef size_t(*IncrementFunctionType)(size_t newCapacity);

         /// @return a new calculated capacity equal to @c newCapacity.
         static size_t one2one(size_t newCapacity);

         /// @return a new calculated capacity as next power of two greater than @c newCapacity.
         static size_t powerOf2(size_t newCapacity);

         /// Constructor with a certain increment function for the buffer capacity management.
         explicit CBuffer(IncrementFunctionType func = &one2one);

         /// Deletes the data currently allocated (if any).
         ~CBuffer();

         /// @return the buffer base pointer (get pointer).
         inline
         const char* gptr() const
         {
            return mBuf;
         }

         /// @return the put pointer of the buffer, i.e. the current pointer for appending data.
         inline
         char* pptr()
         {
            return mBuf + mSize;
         }

         /**
          * Sets a new capacity on the CBuffer. The CBuffer will never shrink, it is expected
          * to be dropped after the operation (which is done in the destructor).
          */
         bool setCapacity(size_t newCapacity);

         /// Bump the buffer size by the given amount @c count of bytes.         
         inline
         void pbump(size_t count)
         {
            mSize += count;
         }

         /// @return the size of the data actually written to the buffer
         inline
         size_t size() const
         {
            return mSize;
         }

         /// @return the current capacity of the buffer object.
         inline
         size_t capacity() const
         {
            return mCapacity;
         }

      private:

         size_t mCapacity;

         char* mBuf;
         size_t mSize;

         IncrementFunctionType mCalculator;

         // stack based data buffer
         char mBuffer[DSI_PAYLOAD_SIZE] __attribute__((aligned(8)));
      };


      // --------------------------------------------------------------------------------


      inline
      CBuffer::CBuffer(IncrementFunctionType func /*= &one2one*/)
         : mCapacity(sizeof(mBuffer))
         , mBuf(mBuffer)
         , mSize(0)
         , mCalculator(func)
      {
         // NOOP
      }


      inline
      CBuffer::~CBuffer()
      {
         if (mBuf != mBuffer)
            ::free(mBuf);
      }


   }   // namespace Private
} // namespace DSI


#endif   // DSI_PRIVATE_CBUFFER_HPP
