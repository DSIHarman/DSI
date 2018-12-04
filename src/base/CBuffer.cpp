/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/private/CBuffer.hpp"


/*static*/
size_t DSI::Private::CBuffer::one2one(size_t newCapacity)
{
   return newCapacity;
}


/*static*/
size_t DSI::Private::CBuffer::powerOf2(size_t newCapacity)
{
   size_t rc = sizeof(mBuffer);   // minimum is double size of static buffer

   while(rc < newCapacity)
      rc <<= 1;

   return rc;
}


bool DSI::Private::CBuffer::setCapacity(size_t newCapacity)
{
   bool rc = true;

   // only if not within static memory CBuffer
   if (newCapacity > sizeof(mBuffer))
   {
      // only expand, never shrink
      if (newCapacity > mCapacity)
      {
         bool needMemcpy = false;
         if (mBuf == mBuffer)
         {
            mBuf = 0;
            needMemcpy = true;
         }

         size_t actualNewCapacity = mCalculator(newCapacity);

         mBuf = (char*)::realloc(mBuf, actualNewCapacity);
         if (mBuf)
         {
            mCapacity = actualNewCapacity;

            if (needMemcpy)
               memcpy(mBuf, mBuffer, size());
         }
         else
            rc = false;
      }
   }

   return rc;
}
