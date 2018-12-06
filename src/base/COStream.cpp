/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/COStream.hpp"
#include "dsi/Log.hpp"
#include "dsi/Streaming.hpp"
#include "utf8.hpp"


DSI::COStream& DSI::COStream::write(const std::wstring& str)
{   
  if (0 != str.size())
  {      
    std::string temp = toUTF8(str);
    write((uint32_t)temp.size()+1);   // include trailing 0 byte
    (void)write(temp.c_str(), temp.size()+1);
  }
  else
    write((uint32_t)0);

  return *this;
}
