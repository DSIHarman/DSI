/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_PRIVATE_LOG_HPP
#define DSI_PRIVATE_LOG_HPP


#define DSI_LOG_STRINGIFY(a) #a


namespace DSI
{   
   namespace Log
   {
   
      void syslog(Level level, const char* format, ...);

      void info(const char* format, ...);
      void warning(const char* format, ...);
      void error(const char* format, ...);
         
      class TraceScope
      {
      public:

         inline
         TraceScope(const char* scope)
            : mScope(scope)
         {
            Log::syslog(Log::Debug, "Entering %s", scope);
         }

         inline
         ~TraceScope()
         {
            Log::syslog(Log::Debug, "Leaving %s", mScope);
         }

      private:

         const char* mScope;
      };
      
   }   // namespace Log
   
}   // namespace DSI


#ifndef TRC_SCOPE_DEF
#   define TRC_SCOPE_DEF(a,b,c)
#endif

#ifndef TRC_SCOPE
#   define TRC_SCOPE(a,b,c) DSI::Log::TraceScope __scope(DSI_LOG_STRINGIFY (a ## _ ## b ## _ ## c))
#endif

#ifndef DBG_MSG
#   define DBG_MSG(a) Log::info a
#endif

#ifndef DBG_WARNING
#   define DBG_WARNING(a) Log::warning a
#endif

#ifndef DBG_ERROR
#   define DBG_ERROR(a) Log::error a
#endif

#endif   // DSI_PRIVATE_LOG_HPP
