/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "utf8.hpp"


std::wstring DSI::fromUTF8(const std::string& src)
{
   std::wstring dest;
   dest.reserve(src.size());   // ...we may produce some overhead here...

   wchar_t w = 0;
   int bytes = 0;
   wchar_t err = L'?';
   for (size_t i = 0; i < src.size(); i++)
   {
      unsigned char c = (unsigned char)src[i];

      if (c <= 0x7f)   //first byte
      {
         if (bytes)
         {
            dest.push_back(err);
            bytes = 0;
         }
         dest.push_back((wchar_t)c);
      }
      else if (c <= 0xbf)   //second/third/etc byte
      {
         if (bytes)
         {
            w = ((w << 6)|(c & 0x3f));
            bytes--;
            if (bytes == 0)
               dest.push_back(w);
         }
         else
            dest.push_back(err);
      }
      else if (c <= 0xdf)   //2byte sequence start
      {
         bytes = 1;
         w = c & 0x1f;
      }
      else if (c <= 0xef)   //3byte sequence start
      {
         bytes = 2;
         w = c & 0x0f;
      }
      else if (c <= 0xf7)   //3byte sequence start
      {
         bytes = 3;
         w = c & 0x07;
      }
      else
      {
         dest.push_back(err);
         bytes = 0;
      }
   }

   if (bytes)
      dest.push_back(err);

   return dest;
}


std::string DSI::toUTF8(const std::wstring& src)
{
   std::string dest;

   // first calculate new length...
   std::string::size_type newSize = 0;

   for (size_t i = 0; i < src.size(); i++)
   {
      wchar_t w = src[i];
      if (w <= 0x7f)
      {
         ++newSize;
      }
      else if (w <= 0x7ff)
      {
         newSize += 2;
      }
      else if (w <= 0xffff)
      {
         newSize += 3;
      }
      else if (w <= 0x10ffff)
      {
         newSize += 4;
      }
      else
         ++newSize;
   }

   // ...and reserve enough space...
   dest.reserve(newSize);

   // ...then fill the newly created string
   for (size_t i = 0; i < src.size(); i++)
   {
      wchar_t w = src[i];
      if (w <= 0x7f)
      {
         dest.push_back((char)w);
      }
      else if (w <= 0x7ff)
      {
         dest.push_back(0xc0 | ((w >> 6)& 0x1f));
         dest.push_back(0x80| (w & 0x3f));
      }
      else if (w <= 0xffff)
      {
         dest.push_back(0xe0 | ((w >> 12)& 0x0f));
         dest.push_back(0x80| ((w >> 6) & 0x3f));
         dest.push_back(0x80| (w & 0x3f));
      }
      else if (w <= 0x10ffff)
      {
         dest.push_back(0xf0 | ((w >> 18)& 0x07));
         dest.push_back(0x80| ((w >> 12) & 0x3f));
         dest.push_back(0x80| ((w >> 6) & 0x3f));
         dest.push_back(0x80| (w & 0x3f));
      }
      else
         dest.push_back('?');
   }

   return dest;
}
