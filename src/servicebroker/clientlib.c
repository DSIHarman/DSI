#include "dsi/clientlib.h"

#include "config.h"

#include "clientIo.h"
typedef struct iovec iov_t;

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <fcntl.h>
#include <alloca.h>
#include <grp.h>

#define UNIX_PATH_MAX    108


#define INIT_ARGUMENT( arg ) \
   memset( &(arg), 0, sizeof(arg)); \
   (arg).i.sbVersion.majorVersion = DSI_SERVICEBROKER_VERSION_MAJOR; \
   (arg).i.sbVersion.minorVersion = DSI_SERVICEBROKER_VERSION_MINOR;

#define FILL_DESCRIPTION( descr, ifname, major, minor ) \
   strncpy( descr.name, ifname, sizeof(descr.name)); \
   descr.version.majorVersion = major ; \
   descr.version.minorVersion = minor ;

#define FILL_DESCRIPTION_TCP( descr, ifname, major, minor ) \
   strncpy( descr.name, ifname, sizeof(descr.name)-4); \
   strcat( descr.name, "_tcp"); \
   descr.version.majorVersion = major ; \
   descr.version.minorVersion = minor ;


int SBOpen( const char* filename )
{
   int rc = -1 ;

   const char* envpath = getenv( "SB_PATH" );
   if( envpath )
      filename = envpath ;

   errno = EINVAL ;

   if( filename )
   {
      struct sockaddr_un addr;

      if (strlen(filename) + strlen(FND_SERVICEBROKER_ROOT) < UNIX_PATH_MAX)
      {
         // connect via unix stream socket (connection oriented)
         rc = socket(PF_UNIX, SOCK_STREAM, 0);

         if (rc >= 0)
         {
            // allow passing credentials to the servicebroker
            int on = 1;
            (void)setsockopt(rc, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));

            addr.sun_family = AF_UNIX;
            strcpy(addr.sun_path, FND_SERVICEBROKER_ROOT);
            strcat(addr.sun_path, filename);

            if (connect(rc, (struct sockaddr*)&addr, sizeof(addr)) < 0)
            {
               int error = errno;
               while(close(rc) && errno == EINTR);
               rc = -1;
               errno = error;
            }
            else
            {
               if (sendAuthPackage(rc, 1/*=send credentials*/) != 0)
               {
                  int error = errno;
                  while(close(rc) && errno == EINTR);
                  rc = -1;
                  errno = error;
               }
            }
         }
      }
      else
         errno = EINVAL;
   }
   return rc ;
}


int SBOpenNotificationHandle()
{      
   int fd = socket(PF_UNIX, SOCK_STREAM, 0);
   
   if (fd > 0)
   {                        
      struct sockaddr_un addr;
      addr.sun_family = AF_UNIX;
      make_unix_path(addr.sun_path, sizeof(addr.sun_path), getpid(), fd);           
      
      if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
      {
         while(close(fd) && errno == EINTR);
         fd = -1;
      }
   }   
   
   return fd;
}


void SBClose( int handle )
{   
   while(close(handle) < 0 && errno == EINTR);
}


int SBRegisterInterface( int handle, const char* ifName, int majorVersion, int minorVersion,
                         int64_t chid, SPartyID *serverId )
{
   int rc = -1 ;
   errno = EINVAL ;

   if( ifName && serverId )
   {
      union SFNDInterfaceRegisterArg arg;
      INIT_ARGUMENT( arg );
      FILL_DESCRIPTION( arg.i.ifDescription, ifName, majorVersion, minorVersion );
      arg.i.chid = chid ;
      arg.i.pid = getpid();
      if( 0 == sendAndReceive( handle, DCMD_FND_REGISTER_INTERFACE, &arg, sizeof(arg.i), sizeof(arg.o), &rc ) && 0 == rc )
      {
         *serverId =  arg.o.serverID;
      }
      int err= errno;
   }
   return rc ;
}


int SBRegisterInterfaceTCP( int handle, const char* ifName, int majorVersion, int minorVersion,
                           uint32_t ipaddress, uint16_t port, SPartyID *serverId )
{
   int rc = -1 ;
   errno = EINVAL ;

   if( ifName && serverId )
   {
      union SFNDInterfaceRegisterArg arg;
      INIT_ARGUMENT( arg );
      FILL_DESCRIPTION_TCP( arg.i.ifDescription, ifName, majorVersion, minorVersion );
      arg.i.chid = port;
      arg.i.pid = ipaddress;
      if( 0 == sendAndReceive( handle, DCMD_FND_REGISTER_INTERFACE, &arg, sizeof(arg.i), sizeof(arg.o), &rc ) && 0 == rc )
      {
         *serverId =  arg.o.serverID;
      }
   }
   return rc ;
}


int SBRegisterGroupInterface( int handle, const char* ifName, int majorVersion, int minorVersion
                            , int64_t chid, const char* groupName, SPartyID *serverId )
{
   int rc = -1 ;
   errno = EINVAL ;

   if( ifName && serverId )
   {
      union SFNDInterfaceRegisterGroupIDArg arg;
      INIT_ARGUMENT( arg );
      FILL_DESCRIPTION( arg.i.ifDescription, ifName, majorVersion, minorVersion );
      arg.i.chid = chid ;
      arg.i.pid = getpid();
      arg.i.grpid = SB_UNKNOWN_GROUP_ID ;

      if( groupName )
      {
         struct group* gr = getgrnam( groupName );
         if( !gr )
         {
            errno = ENOENT ;
            return -1 ;
         }
         arg.i.grpid = gr->gr_gid ;
      }

      if( 0 == sendAndReceive( handle, DCMD_FND_REGISTER_INTERFACE_GROUPID, &arg, sizeof(arg.i), sizeof(arg.o), &rc ) && 0 == rc )
      {
         *serverId =  arg.o.serverID;
      }
   }
   return rc ;
}


int SBRegisterInterfaceEx( int handle, const struct SFNDInterfaceDescription* ifDescr,
                           int descrCount, int64_t chid, SPartyID *serverId )
{
   int rc = -1 ;
   errno = EINVAL ;

   if( ifDescr && serverId )
   {
      iov_t siov[2], riov[1] ;
      union SFNDInterfaceRegisterExArg arg ;
      INIT_ARGUMENT( arg );
      arg.i.chid = chid ;
      arg.i.pid = getpid();
      arg.i.ifCount = descrCount ;

      siov[0].iov_base = &arg.i;
      siov[0].iov_len = sizeof(arg.i);
      siov[1].iov_base = (void*)ifDescr;
      siov[1].iov_len = descrCount * sizeof(struct SFNDInterfaceDescription);

      riov[0].iov_base = serverId ;
      riov[0].iov_len = descrCount * sizeof(SPartyID) ;

      (void)sendAndReceiveV( handle, DCMD_FND_REGISTER_INTERFACE_EX, 2, 1, siov, riov, &rc ) ;
   }
   return rc ;
}


int SBRegisterInterfaceExTCP( int handle, const struct SFNDInterfaceDescription* ifDescr,
                              int descrCount, uint32_t ipaddress, uint16_t port,
                              SPartyID *serverId )
{
   int rc = -1 ;
   errno = EINVAL ;

   if( ifDescr && serverId && descrCount > 0 )
   {
      iov_t siov[2], riov[1] ;
      struct SFNDInterfaceDescription* desc_tcp;
      int i;
      union SFNDInterfaceRegisterExArg arg ;
      INIT_ARGUMENT( arg );
      arg.i.chid = port;
      arg.i.pid = ipaddress;
      arg.i.ifCount = descrCount ;

      siov[0].iov_base = &arg.i;
      siov[0].iov_len = sizeof(arg.i);

      // copy and append _tcp
      desc_tcp = (struct SFNDInterfaceDescription*)alloca(descrCount * sizeof(struct SFNDInterfaceDescription));
      if (desc_tcp)
      {
         memcpy(desc_tcp, ifDescr, descrCount * sizeof(struct SFNDInterfaceDescription));

         for(i=0; i<descrCount; ++i)
         {
            if (strlen(desc_tcp[i].name) <= NAME_MAX - 4)
            {
               strcat(desc_tcp[i].name, "_tcp");
            }
            else
               desc_tcp[i].name[0] = '\0';   // invalidate
         }

         siov[1].iov_base = (void*)desc_tcp;
         siov[1].iov_len = descrCount * sizeof(struct SFNDInterfaceDescription);

         riov[0].iov_base = serverId ;
         riov[0].iov_len = descrCount * sizeof(SPartyID) ;

         (void)sendAndReceiveV( handle, DCMD_FND_REGISTER_INTERFACE_EX, 2, 1, siov, riov, &rc ) ;
      }
   }
   return rc ;
}


int SBUnregisterInterface( int handle, SPartyID serverId )
{
   int rc = -1;

   union SFNDInterfaceUnregisterArg arg;
   INIT_ARGUMENT( arg );
   arg.i.serverID = serverId ;
   (void)sendAndReceive( handle, DCMD_FND_UNREGISTER_INTERFACE, &arg, sizeof(arg.i), 0, &rc );

   return rc;
}


int SBAttachInterface( int handle, const char* ifName, int majorVersion, int minorVersion,
                      struct SConnectionInfo *connInfo )
{
   int rc = -1 ;
   errno = EINVAL ;
   if( ifName && connInfo )
   {
      union SFNDInterfaceAttachArg arg;
      INIT_ARGUMENT( arg );
      FILL_DESCRIPTION( arg.i.ifDescription, ifName, majorVersion, minorVersion );
      if( 0 == sendAndReceive( handle, DCMD_FND_ATTACH_INTERFACE, &arg, sizeof(arg.i), sizeof(arg.o), &rc ) && rc == 0 )
      {
         *connInfo = arg.o ;
      }
   }
   return rc;
}


int SBAttachInterfaceExtended( int handle, const char* ifName,
                               int majorVersion, int minorVersion,
                               struct SConnectionInfo *connInfo,
                               int64_t chid, int code, int value,
                               notificationid_t* notificationID )
{
   int rc = -1 ;
   errno = EINVAL ;
   if( ifName && connInfo && notificationID )
   {
      union SFNDInterfaceAttachExtendedArg arg;
      INIT_ARGUMENT( arg );
      arg.i.pulse.chid = chid ;
      arg.i.pulse.pid = getpid();
      arg.i.pulse.code = code ;
      arg.i.pulse.value = value ;
      FILL_DESCRIPTION( arg.i.ifDescription, ifName, majorVersion, minorVersion );
      if( 0 == sendAndReceive( handle, DCMD_FND_ATTACH_INTERFACE_EXTENDED, &arg, sizeof(arg.i), sizeof(arg.o), &rc ))
      {
         *connInfo = arg.o.connInfo ;
         *notificationID = arg.o.notificationID;
      }
   }
   return rc;
}


int SBGetServerInformation( int handle, const char* ifName,
                            int majorVersion, int minorVersion,
                            struct SServerInfo *serverInfo)
{
   int rc = -1 ;
   errno = EINVAL ;
   if( ifName && serverInfo)
   {
      union SFNDGetServerInformation arg;

      INIT_ARGUMENT( arg );
      FILL_DESCRIPTION( arg.i.ifDescription, ifName, majorVersion, minorVersion );
      if( 0 == sendAndReceive( handle, DCMD_FND_GET_SERVER_INFORMATION, &arg, sizeof(arg.i), sizeof(arg.o), &rc )
          && rc == 0)
      {
         *serverInfo = arg.o ;
      }
   }
   return rc;
}


int SBAttachInterfaceTCP( int handle, const char* ifName, int majorVersion, int minorVersion,
                         struct STCPConnectionInfo *connInfo )
{
   int rc = -1 ;
   errno = EINVAL ;
   if( ifName && connInfo )
   {
      union SFNDInterfaceAttachArg arg;
      INIT_ARGUMENT( arg );
      FILL_DESCRIPTION_TCP( arg.i.ifDescription, ifName, majorVersion, minorVersion );
      if( 0 == sendAndReceive( handle, DCMD_FND_ATTACH_INTERFACE, &arg, sizeof(arg.i), sizeof(arg.o), &rc ) && rc == 0 )
      {
         connInfo->ifVersion = arg.o.ifVersion;
         connInfo->serverID = arg.o.serverID;
         connInfo->clientID = arg.o.clientID;
         connInfo->socket.ipaddress = arg.o.channel.pid;
         connInfo->socket.port = arg.o.channel.chid;
      }
   }
   return rc;
}


int SBDetachInterface( int handle, SPartyID clientId )
{
   int rc = -1;

   union SFNDInterfaceDetachArg arg;
   INIT_ARGUMENT( arg );
   arg.i.clientID = clientId ;
   (void)sendAndReceive( handle, DCMD_FND_DETACH_INTERFACE, &arg, sizeof(arg.i), 0, &rc );

   return rc;
}


int SBSetClientDetachNotification( int handle, SPartyID clientID
                                 , int64_t chid, int code, int value, notificationid_t* notificationID )
{
   int rc = -1 ;
   errno = EINVAL ;

   if( notificationID )
   {
      union SFNDNotifyClientDetachArg arg;
      INIT_ARGUMENT( arg );
      arg.i.clientID = clientID ;
      arg.i.pulse.chid = chid ;
      arg.i.pulse.pid = getpid();
      arg.i.pulse.code = code ;
      arg.i.pulse.value = value ;
      if( 0 == sendAndReceive( handle, DCMD_FND_NOTIFY_CLIENT_DETACH, &arg, sizeof(arg.i), sizeof(arg.o), &rc ) && 0 == rc )
      {
         *notificationID = arg.o.notificationID ;
      }
   }
   return rc ;
}


int SBSetServerAvailableNotification( int handle,
                                      const char* ifName, int majorVersion, int minorVersion,
                                      int64_t chid, int code, int value,
                                      notificationid_t* notificationID )
{
   int rc = -1 ;
   errno = EINVAL ;

   if( notificationID && ifName)
   {
      union SFNDNotifyServerAvailableArg arg;
      INIT_ARGUMENT( arg );
      FILL_DESCRIPTION( arg.i.ifDescription, ifName, majorVersion, minorVersion );
      arg.i.pulse.chid = chid ;
      arg.i.pulse.pid = getpid();
      arg.i.pulse.code = code ;
      arg.i.pulse.value = value ;
      if( 0 == sendAndReceive( handle, DCMD_FND_NOTIFY_SERVER_AVAILABLE, &arg, sizeof(arg.i), sizeof(arg.o), &rc ) && 0 == rc )
      {
         *notificationID = arg.o.notificationID ;
      }
   }
   return rc ;
}


int SBSetServerDisconnectNotification( int handle, SPartyID serverID, int64_t chid, int code, int value,
                                       notificationid_t* notificationID )
{
   int rc = -1 ;
   errno = EINVAL ;

   if( notificationID )
   {
      union SFNDNotifyServerDisconnectArg arg;
      INIT_ARGUMENT( arg );
      arg.i.serverID = serverID ;
      arg.i.pulse.chid = chid ;
      arg.i.pulse.pid = getpid();
      arg.i.pulse.code = code ;
      arg.i.pulse.value = value ;
      if( 0 == sendAndReceive( handle, DCMD_FND_NOTIFY_SERVER_DISCONNECT, &arg, sizeof(arg.i), sizeof(arg.o), &rc ) && 0 == rc )
      {
         *notificationID = arg.o.notificationID ;
      }
   }
   return rc ;
}


int SBClearNotification( int handle, notificationid_t notificationID )
{
   int rc = -1;

   union SFNDClearNotificationArg arg;
   INIT_ARGUMENT( arg );
   arg.i.notificationID = notificationID ;
   (void)sendAndReceive( handle, DCMD_FND_CLEAR_NOTIFICATION, &arg, sizeof(arg.i), 0, &rc );

   return rc;
}


int SBGetInterfaceList( int handle, struct SFNDInterfaceDescription *ifs, int inCount, int *outCount )
{
   int rc = -1;
   errno = EINVAL;

   if (ifs && inCount > 0 && outCount)
   {
      int opt;
      union SFNDGetInterfaceListArg *arg = (union SFNDGetInterfaceListArg*) ifs ;
      INIT_ARGUMENT( *arg );
      arg->i.inElementCount = inCount ;
      rc = sendAndReceive( handle, DCMD_FND_GET_INTERFACELIST, arg, sizeof(arg->i), sizeof(*ifs) * inCount, &opt );

      if (rc == 0)
      {
         if (opt < 0)
         {
            *outCount = -opt;
         }
         else
            rc = opt;
      }
   }

   return rc;
}


int SBSetInterfaceListChangeNotification( int handle,
                                          int64_t chid, int code, int value,
                                          notificationid_t* notificationID )
{
   int rc = -1 ;
   errno = EINVAL ;

   if( notificationID )
   {
      union SFNDNotifyInterfaceListChangeArg arg;
      INIT_ARGUMENT( arg );
      arg.i.pulse.chid = chid ;
      arg.i.pulse.pid = getpid();
      arg.i.pulse.code = code ;
      arg.i.pulse.value = value ;
      if( 0 == sendAndReceive( handle, DCMD_FND_NOTIFY_INTERFACELIST_CHANGE, &arg, sizeof(arg.i), sizeof(arg.o), &rc ) && 0 == rc )
      {
         *notificationID = arg.o.notificationID ;
      }
   }
   return rc ;
}


int SBMatchInterfaceList( int handle, const char* regExpr, struct SFNDInterfaceDescription *ifs, int inCount, int *outCount )
{
   int rc = -1;
   errno = EINVAL;

   if (ifs && inCount > 0 && outCount)
   {
      int opt;
      union SFNDMatchInterfaceListArg *arg = (union SFNDMatchInterfaceListArg*) ifs ;
      INIT_ARGUMENT( *arg );
      arg->i.inElementCount = inCount ;
      if (regExpr)
      {
         strncpy( arg->i.regExpr, regExpr, sizeof(arg->i.regExpr));
      }
      rc = sendAndReceive( handle, DCMD_FND_MATCH_INTERFACELIST, arg, sizeof(arg->i), sizeof(*ifs) * inCount, &opt );

      if (rc == 0)
      {
         if (opt < 0)
         {
            *outCount = -opt;
         }
         else
            rc = opt;
      }
   }

   return rc;
}


int SBSetInterfaceMatchChangeNotification( int handle, const char* regExpr,
                                           int64_t chid, int code, int value,
                                           notificationid_t* notificationID )
{
   int rc = -1 ;
   errno = EINVAL ;

   if( notificationID )
   {
      union SFNDNotifyInterfaceListMatchArg arg;
      INIT_ARGUMENT( arg );
      arg.i.pulse.chid = chid ;
      arg.i.pulse.pid = getpid();
      arg.i.pulse.code = code ;
      arg.i.pulse.value = value ;
      if (regExpr)
      {
         strncpy( arg.i.regExpr, regExpr, sizeof(arg.i.regExpr));
      }
      if( 0 == sendAndReceive( handle, DCMD_FND_NOTIFY_INTERFACELIST_MATCH, &arg, sizeof(arg.i), sizeof(arg.o), &rc ) && 0 == rc )
      {
         *notificationID = arg.o.notificationID ;
      }
   }
   return rc ;
}


void SBGetDSIVersion(struct SFNDInterfaceVersion *sbVersion )
{
   if (sbVersion)
   {
      sbVersion->majorVersion = DSI_SERVICEBROKER_VERSION_MAJOR;
      sbVersion->minorVersion = DSI_SERVICEBROKER_VERSION_MINOR;
   }
}


