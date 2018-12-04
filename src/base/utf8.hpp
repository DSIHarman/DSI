/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_BASE_UTF8_HPP
#define DSI_BASE_UTF8_HPP


#include <string>

namespace DSI
{

   /**
    * Convert a string containing UTF-8 encoded characters to a wide string.
    *
    * @param src UTF-8 encoded string.
    * @return wide string containing converted string.
    */
   std::wstring fromUTF8(const std::string& src);

   /**
    * Encode a wide string in UTF-8 and store the result in a normal string object.
    *
    * @param src The wide string to encode.
    * @return the UTF-8 encoded string as given by @c src.
    */
   std::string toUTF8(const std::wstring& src);

}//namespace DSI
#endif   // DSI_BASE_UTF8_HPP
