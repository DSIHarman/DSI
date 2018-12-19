/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/Log.hpp"

#include <syslog.h>
#include <cstdarg>
#include <cstdio>


namespace /*anonymous*/
{
  DSI::Log::Level sLogLevel = DSI::Log::Critical;
  int sDevice = DSI::Log::SystemLog;
}

namespace DSI
{
  namespace Log
  {
    void setLevel(int level)
    {
      if (level < 0)
      {
        level = 0;
      }
      else if (level > static_cast<int>(Debug))
      {
        level = static_cast<int>(Debug);
      }

      sLogLevel = static_cast<Level>(level);
    }


    void setDevice(int device)
    {
      sDevice = device;
    }


    static
    int mapping[] =
    {
      LOG_CRIT,
      LOG_ERR,
      LOG_WARNING,
      LOG_INFO,
      LOG_DEBUG
    };


    void syslog(Level level, const char* format, ...)
    {
      if (level <= sLogLevel)
      {
        va_list args;
        va_start(args, format);

        if (sDevice & Log::SystemLog)
        {
          ::vsyslog(LOG_USER|mapping[level], format, args);
        }

        if (sDevice & Log::Console)
        {
          ::vprintf(format, args);
          ::printf("\n");
        }
      }
    }


    void info(const char* format, ...)
    {
      if (sLogLevel >= Log::Info)
      {
        va_list args;
        va_start(args, format);

        if (sDevice & Log::SystemLog)
        {
          ::vsyslog(LOG_USER|LOG_INFO, format, args);
        }

        if (sDevice & Log::Console)
        {
          ::vprintf(format, args);
          ::printf("\n");
        }
      }
    }


    void warning(const char* format, ...)
    {
      if (sLogLevel >= Log::Warning)
      {
        va_list args;
        va_start(args, format);

        if (sDevice & Log::SystemLog)
        {
          ::vsyslog(LOG_USER|LOG_WARNING, format, args);
        }

        if (sDevice & Log::Console)
        {
          ::vprintf(format, args);
          ::printf("\n");
        }
      }
    }


    void error(const char* format, ...)
    {
      if (sLogLevel >= Log::Error)
      {
        va_list args;
        va_start(args, format);

        if (sDevice & Log::SystemLog)
        {
          ::vsyslog(LOG_USER|LOG_ERR, format, args);
        }

        if (sDevice & Log::Console)
        {
          ::vprintf(format, args);
          ::printf("\n");
        }
      }
    }
  }
}
