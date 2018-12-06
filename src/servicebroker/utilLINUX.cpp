/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "util.hpp"
#include "config.h"

#include <unistd.h>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>
#include <cstdlib>
#include <syslog.h>
#include <cctype>
#include <signal.h>
#include <netinet/in.h>

#include "Log.hpp"


int GetProcessName( pid_t pid, char* name, size_t length )
{
  int retval = 0 ;

  char buf[128];
  sprintf(buf, "/proc/%d/status", pid);
  int fd = ::open(buf, O_RDONLY);
  if (fd >= 0)
  {
    ::memset(buf, 0, sizeof(buf));
    while(::read(fd, buf, sizeof(buf)) < 0 && errno == EINTR);

    // hopefully read enough: The line should look like this: 
    //      Name:   <the process name><CR, LF or CRLF>
    char* ptr = buf;

    while(*ptr != ':' && *ptr != '\0')
      ++ptr;
    if (*ptr)   // read over ':'
      ++ptr;
    while(isspace(*ptr) && *ptr != '\0')
      ++ptr;

    // ptr is the name now
    char* ptr2 = ptr;

    while(*ptr2 != '\r' && *ptr2 != '\n' && *ptr != '\0')
      ++ptr2;

    *ptr2 = '\0';

    // the name is stripped now
    ::strncpy(name, ptr, length);
    name[length-1] = '\0';

    while(::close(fd) && errno == EINTR);      
  }

  return retval;
}


void doDaemonize()
{
  Log::setLogConsole(false);

  pid_t childPid = ::fork();

  if (childPid < 0)
  {
    ::exit(EXIT_FAILURE);
  }
  else if (childPid > 0)
  {
    ::exit(EXIT_SUCCESS);
  }

  // child continues
  setsid();

  int i;
  for (i=getdtablesize(); i>=0; --i)
  {
    while(::close(i) && errno == EINTR);
  }

  i = ::open("/dev/null", O_RDWR); /* open stdin */
  (void)::dup(i); /* stdout */
  (void)::dup(i); /* stderr */
}


uint32_t getLocalIPForMasterConnection(uint32_t masterIP)
{
  // connect an ip socket and extract the local socket address (ping)
  int32_t rc = SB_UNKNOWN_IP_ADDRESS;

  int sock = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock >= 0)
  {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1025);
    addr.sin_addr.s_addr = masterIP;

    socklen_t len = sizeof(addr);
    if (::connect(sock, (struct sockaddr*)&addr, len) == 0)         
    {
      if (::getsockname(sock, (struct sockaddr*)&addr, &len) == 0)         
        rc = addr.sin_addr.s_addr;                     
    }

    while(::close(sock) && errno == EINTR);
  }   

  if (rc == SB_UNKNOWN_IP_ADDRESS)
    Log::warning(1, "Cannot retrieve local IP address");

  return rc;
}   


void shutdown()
{
  (void)::kill(getpid(), SIGTERM);      
}
