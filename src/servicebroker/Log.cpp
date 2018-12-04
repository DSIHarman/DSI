/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "Log.hpp"

#include <cstdio>
#include <unistd.h>
#include <syslog.h>


int Log::sSBLogLevel = 0 ;
char Log::sSBType = 'U' ;
bool Log::sLogConsole = false ;


namespace /*anonymous*/
{
   int syslog_type[] = { LOG_NOTICE, LOG_WARNING, LOG_ERR };
}


void Log::setLevel( int level )
{
   sSBLogLevel = level ;
}


int Log::getLevel()
{
   return sSBLogLevel ;
}


void Log::setType( char c )
{
   sSBType =  c ;
}


void Log::setLogConsole( bool b )
{
   sLogConsole = b ;
}


void Log::printMessage( eSBLogType type, int level, const char *format, va_list arglist )
{
   char buffer[512];
   const char* typeStr = "";

   (void)vsnprintf( buffer, sizeof(buffer)-1, format, arglist );

   switch( type )
   {
   case SBLOG_MESSAGE: typeStr = "MSG"; break ;
   case SBLOG_WARNING: typeStr = "WRN"; break ;
   case SBLOG_ERROR:   typeStr = "ERR"; break ;
   }

   if( sLogConsole )
   {
      (void)fflush( stdout );
      (void)fprintf( stderr, "%06d %d%c %s: %s\n", getpid(), level, sSBType, typeStr, buffer );
      (void)fflush(stderr);
      (void)fflush(stdout);
   }
      
   (void)syslog(syslog_type[type], "%05d %d%c %s: %s\n", getpid(), level, sSBType, typeStr, buffer);   
}

