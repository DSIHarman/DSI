/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_LOG_H
#define DSI_SERVICEBROKER_LOG_H


#include <cstdarg>


class Log
{
public:

   enum eSBLogType
   {
      SBLOG_MESSAGE
   ,  SBLOG_WARNING
   ,  SBLOG_ERROR
   } ;
   
   static void message( int level, const char *format, ... );
   static void warning( int level, const char *format, ... );
   static void error( const char *format, ... ) ;

   static void setLevel( int level );
   static int getLevel();
   static void setType( char c );
   static void setLogConsole( bool b );

private:   

   static void printMessage( Log::eSBLogType type, int level, const char *format, va_list arglist );
   static int sSBLogLevel ;
   static char sSBType ;
   static bool sLogConsole ;
};


inline 
void Log::message( int level, const char *format, ... )
{
   va_list arglist;
   if( level <= sSBLogLevel )
   {
      va_start(arglist, format);   
      printMessage( Log::SBLOG_MESSAGE, level, format, arglist );
   }
}


inline 
void Log::warning( int level, const char *format, ... )
{
   va_list arglist;
   if( level <= sSBLogLevel )
   {
      va_start(arglist, format);   
      printMessage( Log::SBLOG_WARNING, level, format, arglist );
   }
}


inline 
void Log::error( const char *format, ... )
{
   va_list arglist;
   va_start(arglist, format);  
   printMessage( Log::SBLOG_ERROR, 1, format, arglist );
}


#endif   // DSI_SERVICEBROKER_LOG_H

