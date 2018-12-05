/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
//#include "dsi/servicebroker.h"

#include <errno.h>

#include "FileLock.hpp"

#include <syslog.h>
#include <memory>
#include <signal.h>

#include "Notification.hpp"
#include "Servicebroker.hpp"
#include "Log.hpp"
#include "util.hpp"
#include "config.h"
#include "ServicebrokerServer.hpp"

ServicebrokerServer* pServer;


namespace /* anonymous */
{

/**
 * Write a file containing the port number of the HTTP port in order to allow sbcat to parse it.
 */
class HTTPPortFile
{
public:

   explicit
   HTTPPortFile(const char* mountpoint)   
   {         
      assert(strlen(mountpoint) + strlen(FND_SERVICEBROKER_ROOT) + 5/*=.http*/ < sizeof(mFilename));
      sprintf(mFilename, "%s%s.http", FND_SERVICEBROKER_ROOT, mountpoint);
      
      int fd = ::open(mFilename, O_WRONLY|O_TRUNC|O_CREAT, 0644);
      if (fd >= 0)
      {
         char buf[16];
         sprintf(buf, "%d", getRealPort(SB_HTTP_PORT));
                  
         size_t total = 0;
         while (total < strlen(buf))
         {
            ssize_t rc = write(fd, buf + total, strlen(buf) - total);
            if (rc < 0)
            {
               if (errno != EINTR)
               {
                  Log::warning(1, "Cannot open HTTP port file '%s'", mFilename);                  
               }
            }
            else
               total += rc;
         }         
         
         while(::close(fd) && errno == EINTR);
      }
      else
      {
         mFilename[0] = '\0';
         Log::warning(1, "Cannot open HTTP port file '%s'", mFilename);
      }
   }
   
   
   ~HTTPPortFile()
   {
      if (strlen(mFilename) > 0)
         (void)::unlink(mFilename);
   }

   
private:
   
   char mFilename[128];
};

         
void sigHandler( int /*signo*/ )
{
   pServer->stop();
}


int nullTranslator(int, int)
{
   return 0;
}


inline
void initSignals()
{
   (void)::signal(SIGTERM, sigHandler);
   (void)::signal(SIGINT, sigHandler);
   (void)::signal(SIGQUIT, sigHandler);
}


void printUsage()
{
   printf("Usage: servicebroker [-v] [-p <path>] -d -c [-m <masterpath>] -f <file>\n\n");
   printf("   -v           Verbosity level (mayne given multiple times. At most one -v is advisable.\n");
   printf("   -p <path>    The servicebroker \"mountpoint\" (= unix stream socket path in filesystem).\n");
   printf("   -d           Do not run servicebroker in daemon mode.\n");
   printf("   -c           Copy debug messages to console (stderr).\n");
   printf("   -t           Enable TCP/IP master support (listen on port 3746).\n");
   printf("   -b           Like -t but extended with a comma separated list of IPs to bind.\n");
   printf("   -m <ip:port> Address specifier to master servicebroker, e.g. 127.0.0.1:3740.\n");   
   printf("   -f <file>    Loads config from <file> (default: /etc/servicebroker.cfg).\n");   
   printf("   -i <id>      Sets the servicebroker id (tree mode)\n");   
#ifdef USE_SERVER_CACHE
   printf("   -a           Asynchronous mode: unblock clients on interface attach, available only when using caching\n");
   printf("   -C           Enable server caching, available only in tree mode\n");
#endif
   printf("\n");
}

}   // namespace anonymous


int main(int argc, char *argv[])
{
   bool enable_tcp = false;
   int sbverbose = 0 ;
   bool bNextOpt = true ;
   bool bDaemonize = true ;
   bool asyncMode = false;
   bool useServerCache = false;   

   int id = -1 ;   

   const char* masterAddress = 0 ;
   const char* mountpoint = FND_SERVICEBROKER_PATHNAME ;
   const char* config = 0 ;
   const char* bindIPs = 0 ;

   // Scan command line
   while( bNextOpt )
   {
      switch(getopt( argc, argv, "hdvp:m:n:csf:ti:k:q:Cab:" ))
      {
      case 'v':
         sbverbose++;
         break;

      case 'f':
         config = optarg ;
         break;

      case 'p':
         mountpoint = optarg ;
         break;

      case 'c':
         Log::setLogConsole(true);
         break;

      case 't':
         enable_tcp = true;
         break;

      case 'b':
         enable_tcp = true;
         bindIPs = optarg;
         break;

      case 'm':
         masterAddress = optarg ;
         break;
            
      case 'd':
         bDaemonize = false;
         break;

      case 'h':
         printUsage(); 
         return 0;
      
      case 'i':
         id = atoi(optarg);
         if( id < 1 )
         {
            Log::error("Invalid servicebroker identifier: %d, must be >= 1", id);
            return -1 ;
         }
         break;
      
      case 'a':
         asyncMode = true;
         break;

      case 'C':
         useServerCache = true;
         break;

      case -1:
         bNextOpt = false ;
         break;

      default:
         Log::error("Improper Usage");
         return -1 ;
      }
   }
   
   Log::setType( masterAddress ? 'S' : 'M' );
   Log::setLevel( sbverbose );
   
#ifndef USE_SERVER_CACHE
   if (useServerCache)
   {
      Log::error("Server caching not built-in, caching will be disabled");
      useServerCache = false;
   }
#endif
   if (id == -1 && useServerCache)
   {
      Log::error("Server caching working only in tree mode, caching will be disabled");
      useServerCache = false;
   }
   if (!useServerCache && asyncMode)
   {
      Log::error("Asnyc mode available only when using caching, async mode will be disabled");
      asyncMode = false;
   }

   if (bDaemonize)
      doDaemonize();

   ::openlog("servicebroker", LOG_CONS, LOG_USER);

   // master adapter is optional -> a master does not have it :)
   std::unique_ptr<MasterAdapter> masterAdapter(nullptr);
   
   if (masterAddress)   
      masterAdapter.reset(new MasterAdapter(masterAddress));           // is slave                     
   ServicebrokerServer server;

   // link it for signal handler
   pServer = &server;
   
   // now initialize signals
   initSignals();

   Log::message( 1, "servicebroker %s started", masterAddress ? "slave" : "master" );
   Log::message( 1, "version: %d.%d.%d", Servicebroker::MAJOR_VERSION
                                       , Servicebroker::MINOR_VERSION
                                       , Servicebroker::BUILD_VERSION);
   Log::message( 1, "build: %s", Servicebroker::BUILD_TIME );
   Log::message( 1, "mountpoint: %s", mountpoint );

   if( id != -1 )
      Log::message( 1, "id: %d", id );      

   // init the main event dispatcher
   Servicebroker::getInstance().init(masterAdapter.get(), nullTranslator, config, id, useServerCache, asyncMode
   );

   int retval = EXIT_FAILURE;

   // lock the mountpoint and start the server
   char abs_lockfile[128];
   assert(strlen(mountpoint) + strlen(FND_SERVICEBROKER_ROOT) + 4/*=.lck*/ < sizeof(abs_lockfile));
   sprintf(abs_lockfile, "%s%s.lck", FND_SERVICEBROKER_ROOT, mountpoint);

   FileLock f(abs_lockfile);
   if (f)
   {
      HTTPPortFile httpf(mountpoint);
      
      unsigned int flags = ServicebrokerServer::eEnableHTTP;
      
      if (enable_tcp)
         flags |= ServicebrokerServer::eEnableTcpMaster;
         
      if (masterAdapter.get())
         flags |= ServicebrokerServer::eEnableTcpSlave;

      if (server.init(mountpoint, masterAdapter.get(), flags, bindIPs))
         retval = server.run();
   }
   else
      Log::error("Lock failed, will shutdown.");   

   return retval;
}
