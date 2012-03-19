/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
/**
 * Utility program to display servicebroker statistics.
 */

#include <string>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "dsi/private/servicebroker.h"

#include "io.hpp"


using namespace DSI;


/// Checks the servicebrokers '.http' file for the HTTP port the sb is listening on.
int getSBPort(const char* mountpoint)
{
   int rc = -1;
      
   std::string completeMountpoint(FND_SERVICEBROKER_ROOT);
   completeMountpoint += mountpoint;
   completeMountpoint += ".http";   
   
   std::ifstream ifs(completeMountpoint.c_str());
   if (ifs)
   {
      ifs >> rc;
   }
   
   return rc;
}


/// Send a trivial HTTP request to the loopback device.
bool sendHTTPRequest(IPv4::StreamSocket& sock, const char* page)
{
   std::ostringstream oss;
   oss << "GET " << page << "HTTP/1.1\r\n"
       << "Host: 127.0.0.1\r\n"
       << "\r\n";
       
   io::error_code ec;
   sock.write_all(oss.str().c_str(), oss.str().size(), ec);
   
   return ec == io::ok;
}


/// Trivial dump of the HTTP response body.
bool receiveAndWriteHTTPResult(IPv4::StreamSocket& sock, std::ostream& os)
{
   char buf[1024];   
   
   io::error_code ec;      
   bool inHeader = true;
   
   do
   {
      memset(buf, 0, sizeof(buf));
      sock.read_some(buf, sizeof(buf), ec);
      
      if (ec == io::ok)
      {
         char* body = buf;
         
         // this algorithm is not bullet-proof, but we keep it as long
         // as we have no trouble, or?
         if (inHeader)
         {
            body = strstr(buf, "\r\n\r\n");
            if (body)    
            {            
               body += 4;            
               inHeader = false;
            }               
         }
         
         os << body << std::endl;         
      }                  
   }
   while(ec == io::ok);
   
   return ec == io::ok || ec == io::eof;
}


void printUsage(std::ostream& os)
{
   os << "Utility program to display servicebroker statistics." << std::endl 
      << std::endl
      << "Usage: sbcat [<mountpoint>]" << std::endl 
      << std::endl
      << "<mountpoint>      mountpoint of servicebroker, e.g. '/master', defaults to '/servicebroker'" << std::endl
      << std::endl;
}


int main(int argc, char** argv)
{
   const char* mountpoint = FND_SERVICEBROKER_PATHNAME;    // default servicebroker mountpoint
   
   if (argc > 1)
   {
      if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
      {
         printUsage(std::cout);
         return EXIT_SUCCESS;
      }
      else
         mountpoint = argv[1];      
   }
         
   IPv4::StreamSocket sock;
   if (sock.open() == io::ok)
   {
      if (sock.connect(IPv4::Endpoint("127.0.0.1", getSBPort(mountpoint))) == io::ok)
      {
         if (sendHTTPRequest(sock, "/"))         
            receiveAndWriteHTTPResult(sock, std::cout);
         
         return EXIT_SUCCESS;
      }
   }
   
   return EXIT_FAILURE;   
}