/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "util.hpp"
#include "config.h"
#include "Log.hpp"
#include "Notification.hpp"
#include "ServicebrokerServer.hpp"

extern ServicebrokerServer* pServer;


bool setup(const char* command)
{
  bool rc = true;

  char option[32];
  int iarg = 0;

  // check for verbose command
  if (command)
  {
    if( 1 == sscanf( command, "verbose=%d", &iarg ))
    {
      Log::setLevel( iarg );
      Log::message( 0, "SETTING VERBOSE LEVEL TO %d", iarg );      
    }
    else if( 1 == sscanf( command, "console=%d", &iarg ))
    {
      Log::setLogConsole( 0 != iarg );
      Log::message( 0, "SETTING CONSOLE MODE %s", 0 != iarg ? "ON" : "OFF" );      
    }
    else if( 0 == strncmp( command, "shutdown", 8 ))
    {
      Log::message( 1, "RECEIVED SHUTDOWN COMMAND" );
      shutdown();
    }
    else if( 1 == sscanf( command, "disconnect=%s", option ))
    {
      Log::message( 1, "DISCONNECTING %s", option );

      if (option[0] == 'm')
      {            
        pServer->stopWorkerThread();          
      }
      else
        rc = false;
    }
    else
      rc = false;
  }

  return rc;
}


/**
 * Retrieve environment variable name for given default port number.
 */
const char* getPortEnvName(int port)
{
  const char* envName = nullptr;

  switch(port)
  {
    case SB_MASTER_PORT:
      envName = "SB_MASTER_PORT";
      break;

    case SB_SLAVE_PORT:
      envName = "SB_SLAVE_PORT";
      break;

    case SB_HTTP_PORT:
      envName = "SB_HTTP_PORT";
      break;

    default:
      return nullptr;
  }

  return ::getenv(envName);
}


int getRealPort(int port)
{
  const char* s_port = getPortEnvName(port);
  if (s_port)
    port = ::atoi(s_port);

  return port;
}
