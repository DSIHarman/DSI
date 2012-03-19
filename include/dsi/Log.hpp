/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_LOG_HPP
#define DSI_LOG_HPP


#include "dsi/config.h"
#include "dsi/DSI.hpp"


namespace DSI
{
   /**
    * DSI Logging subsystem for internal use. The only end-user access is to initialize
    */
   namespace Log
   {
      /**
       * Severity level of logging message
       */
      enum Level
      {
         Critical = 0,   ///< critical error situation, application probably about to abort
         Error,          ///< error message, application potentially shows ill behaviour
         Warning,        ///< warning message
         Info,           ///< informational message
         Debug           ///< debugging only message
      };


      /**
       * Device where to send the log message to.
       */
      enum Device
      {
         SystemLog = (1 << 0),
         Console   = (1 << 1)
      };
      
      /**
       * Set the verbosity level filter. Only messages whose level is lower equal than @c level
       * will afterwards produce logging output - if a device is set.
       */
      void setLevel(int level);
      
      /**
       * Set the logging device. The given argument is interpreted as bit mask as given in the
       * @c Device enumerator. The default setting is to log to the system log.
       */
      void setDevice(int device);            
      
   }   // namespace Log
   
}   // namespace DSI


#include "dsi/private/Log.hpp"

#endif // DSI_LOG_HPP
