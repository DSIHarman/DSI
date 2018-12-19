/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_UTIL_HPP
#define DSI_SERVICEBROKER_UTIL_HPP


#include <unistd.h>
#include <stdint.h>
#include <cstring>
#include <string>


/**
 * Retrieve process name for given pid.
 */
int GetProcessName( pid_t pid, char* name, size_t length );

/**
 * Daemonize the process.
 */
void doDaemonize();

/**
 * Extract the local ip over which the servicebroker connects to the master.
 * All arguments and return value in network byte order.
 */
uint32_t getLocalIPForMasterConnection(uint32_t masterIP);

/**
 * Setup the server via a command which looks like "name=value".
 * See sourcecode for valid commands.
 */
bool setup(const char* command);

/**
 * Shutdown the application. 
 */
void shutdown();


inline
bool endsWith(const std::string& str, const char* needle)
{
   return str.rfind(needle) == str.size()-strlen(needle);
}


/**
 * Return given translated port in case of environment variable setting.
 */
int getRealPort(int port);


#endif   // DSI_SERVICEBROKER_UTIL_HPP
