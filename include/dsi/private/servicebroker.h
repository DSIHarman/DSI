#ifndef DSI_PRIVATE_SERVICEBROKER_H
#define DSI_PRIVATE_SERVICEBROKER_H

#include "dsi/private/platform.h"
#include "dsi/version.h"


#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus defined */


#define FND_SERVICEBROKER_ROOT "/var/run/servicebroker/"

/** @brief The service broker pathname. */
#define FND_SERVICEBROKER_PATHNAME    "/servicebroker"


#define FOUNDATION_INVALID_PARTY_ID 0xffffffff


#ifdef __cplusplus
   /**
    * @brief Describes partyID used for server and client ID.
    */
   struct SPartyID
   {
      union
      {
         struct
         {
            uint32_t localID ;
            uint32_t extendedID ;
         } s ;
         uint64_t globalID ;
      } __attribute__((aligned(8)));

      // assignment operator
      SPartyID& operator= ( uint64_t _globalID );

      // equality operator
      bool operator== ( const SPartyID& rhs ) const ;

      // cast to uint64_t
      operator uint64_t() const;
   } ;


   inline
   SPartyID& SPartyID::operator= ( uint64_t _globalID )
   {
      globalID = _globalID ;
      return *this ;
   };



   inline
   bool SPartyID::operator== ( const SPartyID& rhs ) const
   {
      return s.localID == rhs.s.localID ;
   };


   inline
   SPartyID::operator uint64_t() const
   {
      return globalID ;
   };

#else

   typedef union _SPartyID
   {
      struct
      {
         uint32_t localID ;
         uint32_t extendedID ;
      } s ;
      uint64_t globalID ;
   } SPartyID __attribute__((aligned(8)));

#endif


   typedef uint32_t notificationid_t;
   typedef int32_t dcmd_t;

   /**
    * A pulse representation in socket mode.
    */
   typedef struct sb_pulse
   {
      int32_t code;
      int32_t value;

   } sb_pulse_t;


   /**
    * @brief Describes an implementation version.
    */

   struct SFNDImplementationVersion
   {
      /** @brief The major version number of the implementation. */
      uint16_t majorVersion;

      /** @brief The minor version number of the implementation. */
      uint16_t minorVersion;

      /** @brief The patch level number of the implementation. */
      uint16_t patchlevel;
   };


   /**
    * @brief Describes an interface.
    */
   struct SFNDInterfaceDescription
   {
      /** @brief The interface version. */
      struct SFNDInterfaceVersion version;

      /** @brief The nul terminated interface name. */
      char name[NAME_MAX+1];
   };

   struct SFNDInterfaceDescriptionEx
   {
      /** @brief The version of the implementation to register. */
      struct SFNDImplementationVersion implVersion;

      /** @brief The server channel. */
      int32_t chid;

      /** @brief The server process id. */
      int32_t pid;

      /** @brief The node id. */
      int32_t nd;

      /** explicit padding here since serverID should be aligned on 8 bytes */
      //int32_t padding;

      /** @brief the local id on the slave. */
      SPartyID serverID ;

      /** @brief The null terminated interface name. */
      struct SFNDInterfaceDescription ifDescription ;
   };

   /**
    * @brief Describes channel information.
    */
   struct SFNDChannelInfo
   {
      /** The id of the node the process is running on */
      uint32_t nid;

      /** The ID of the process. */
      pid_t pid;

      /** The channel ID. */
      int32_t chid;
   };

   /**
    * @brief Arguments returned from the service broker back to the client.
    */
   struct SConnectionInfo
   {
      /** The actual interface version. */
      struct SFNDInterfaceVersion ifVersion;

      /** The channel information of the server. */
      struct SFNDChannelInfo channel;

      /** The ID of the server. */
      SPartyID serverID;

      /** Unique ID for the attached client. */
      SPartyID clientID;
   } ;

   /**
    * @brief Connection infomration for a server.
    */
   struct SServerInfo
   {
      /** The channel information of the server. */
      struct SFNDChannelInfo channel;

      /** The ID of the server. */
      SPartyID serverID;
   };

   struct STCPSocketInfo
   {
      uint32_t ipaddress;    ///< in host byte order
      uint16_t port;         ///< in host byte order
   };


   struct STCPConnectionInfo
   {
      /** The actual interface version. */
      struct SFNDInterfaceVersion ifVersion;

      /** The socket information of the server. */
      struct STCPSocketInfo socket;

      /** The ID of the server. */
      SPartyID serverID;

      /** Unique ID for the attached client. */
      SPartyID clientID;
   };


   /**
    * @brief Describes pulse information passed for notifications.
    */
   struct SFNDPulseInfo
   {
      /** The pulse code. */
      int32_t code;
      /** The pulse value. */
      int32_t value;
      /** The id of the channel to send the pulse to. */
      int32_t chid;
      /** The id of process that owns the channel. */
      int32_t pid;
   };




   /**
    * @brief Argument passed with the DCMD_FND_REGISTER_INTERFACE command.
    */
   union SFNDInterfaceRegisterArg
   {
      /**
       * @brief Arguments passed from the client to the service broker.
       */
      struct
      {
         /**
          * @brief The service broker version.
          *
          * Clients have to provide the version of the actually used
          * service broker in this element.
          */
         struct SFNDInterfaceVersion sbVersion;

         /** @brief The description of the interface to register. */
         struct SFNDInterfaceDescription ifDescription;

         /** @brief The version of the implementation to register. */
         struct SFNDImplementationVersion implVersion;

         /** @brief The server channel. */
         int32_t chid;

         /** @brief The server process id. */
         int32_t pid;
      }
         i;

      /**
       * @brief Arguments passed from the service broker to client.
       */
      struct
      {
         /** @brief The unique server ID. */
         SPartyID serverID;
      }
         o;
   };


   /**
    * @brief Argument passed with the DCMD_FND_REGISTER_INTERFACE_GROUPID command.
    */
   union SFNDInterfaceRegisterGroupIDArg
   {
      /**
       * @brief Arguments passed from the client to the service broker.
       */
      struct
      {
         /**
          * @brief The service broker version.
          *
          * Clients have to provide the version of the actually used
          * service broker in this element.
          */
         struct SFNDInterfaceVersion sbVersion;

         /** @brief The description of the interface to register. */
         struct SFNDInterfaceDescription ifDescription;

         /** @brief The version of the implementation to register. */
         struct SFNDImplementationVersion implVersion;

         /** @brief The server channel. */
         int32_t chid;

         /** @brief The server process id. */
         int32_t pid;

         /** @brief The os user group id of this interface. */
         int32_t grpid;
      }
         i;

      /**
       * @brief Arguments passed from the service broker to client.
       */
      struct
      {
         /** @brief The unique server ID. */
         SPartyID serverID;
      }
         o;
   };


   /**
    * @brief Argument passed with the DCMD_FND_REGISTER_INTERFACE_EX command.
    */
   union SFNDInterfaceRegisterExArg
   {
      /**
       * @brief Arguments passed from the client to the service broker.
       */
      struct
      {
         /**
          * @brief The service broker version.
          *
          * Clients have to provide the version of the actually used
          * service broker in this element.
          */
         struct SFNDInterfaceVersion sbVersion;

         /** @brief The version of the implementation to register. */
         struct SFNDImplementationVersion implVersion;

         /** @brief The server channel. */
         int32_t chid;

         /** @brief The server process id. */
         int32_t pid;

         /** @brief number of interfaces. */
         int32_t ifCount;

         /** @brief followed by list of SFNDInterfaceDescription structures */
         /* struct SFNDInterfaceDescription ifDescription[ifCount] ; */
      }
         i;

      /**
       * @brief Arguments passed from the service broker to client.
       */
      struct
      {
         /** @brief list of unique server IDs. */
         SPartyID serverID[1];
      }
         o;
   };

   /**
    * @brief Argument passed with the DCMD_FND_REGISTER_MASTER_INTERFACE_EX command.
    */
   union SFNDInterfaceRegisterMasterExArg
   {
      /**
       * @brief Arguments passed from the client to the service broker.
       */
      struct
      {
         /** @brief The service broker version. */
         struct SFNDInterfaceVersion sbVersion;

         /** @brief number of interfaces. */
         int32_t ifCount;

         /** @brief followed by list of SFNDInterfaceDescriptionEx structures */
         /* struct SFNDInterfaceDescriptionEx ifDescription[ifCount] ; */
      }
         i;

      /**
       * @brief Arguments passed from the service broker to client.
       */
      struct
      {
         /** @brief list of unique server IDs. */
         SPartyID serverID[1];
      }
         o;
   };

   /**
    * @brief Argument passed with the DCMD_FND_UNREGISTER_INTERFACE command.
    */
   union SFNDInterfaceUnregisterArg
   {
      /**
       * @brief Arguments passed to the service broker.
       */
      struct
      {
         /** The service broker version. */
         struct SFNDInterfaceVersion sbVersion;
         /** Reserved for future use. */
         uint32_t reserved ;
         /** The ID of the server to unregister. */
         SPartyID serverID;
      }
         i;
   };


   /**
    *  @brief Argument passed with the DCMD_FND_INTERFACE_ATTACH command.
    */
   union SFNDInterfaceAttachArg
   {
      /**
       * @brief Argument passed to the service broker.
       */
      struct
      {
         /** The service broker version. */
         struct SFNDInterfaceVersion sbVersion;
         /** @brief The interface description. */
         struct SFNDInterfaceDescription ifDescription;

      }
         i;

      /**
       * @brief Arguments returned from the service broker back to the client.
       */
      struct SConnectionInfo o;
   };

   /**
    *  @brief Argument passed with the DCMD_FND_ATTACH_INTERFACE_EXTENDED command.
    */
   union SFNDInterfaceAttachExtendedArg
   {
      /**
       * @brief Argument passed to the service broker.
       */
      struct
      {
         /** The service broker version. */
         struct SFNDInterfaceVersion sbVersion;
         /** Information about the pulse to send when the interface is available. */
         struct SFNDPulseInfo pulse;
         /** @brief The interface description. */
         struct SFNDInterfaceDescription ifDescription;
      }
         i;

      /**
       * @brief Arguments returned from the service broker back to the client.
       */
      struct
      {
         /** The connection information for the server.*/
         struct SConnectionInfo connInfo;
         /** A unique notification ID for the server available or disconnect notifications. */
         notificationid_t notificationID;
      }
         o;
   };

   /**
    *  @brief Argument passed with the DCMD_FND_GET_SERVER_INFORMATION command.
    */
   union SFNDGetServerInformation
   {
      /**
       * @brief Argument passed to the service broker.
       */
      struct
      {
         /** The service broker version. */
         struct SFNDInterfaceVersion sbVersion;
         /** @brief The interface description. */
         struct SFNDInterfaceDescription ifDescription;
      }
         i;

      /**
       * @brief Arguments returned from the service broker back to the client.
       */
      struct SServerInfo o;
   };


   /**
    *  @brief Argument passed with the DCMD_FND_NOTIFY_SERVER_DISCONNECT command.
    */
   union SFNDNotifyServerDisconnectArg
   {
      /**
       * @brief Arguments passed to the service broker.
       */
      struct
      {
         /** The service broker version. */
         struct SFNDInterfaceVersion sbVersion;
         /** Reserved for future use. */
         uint32_t reserved ;
         /** The server ID of the server. */
         SPartyID serverID;
         /** Information about the pulse to send when the server has disconnected. */
         struct SFNDPulseInfo pulse;
      }
         i;

      /**
       * @brief Arguments passed from the service broker to the client.
       */
      struct
      {
         /** A unique notification ID. */
         notificationid_t notificationID;
      }
         o;
   };

   /**
    *  @brief Argument passed with the DCMD_FND_NOTIFY_SERVER_AVAILABLE command.
    */
   union SFNDNotifyServerAvailableArg
   {
      /**
       *  @brief Arguments passed to the service broker.
       */
      struct
      {
         /** @brief The service broker version. */
         struct SFNDInterfaceVersion sbVersion;
         /** Information about the pulse to send when the interface is available. */
         struct SFNDPulseInfo pulse;
         /** The interface description. */
         struct SFNDInterfaceDescription ifDescription;

      }
         i;

      /**
       * @brief Arguments passed from the service broker to the client.
       */
      struct
      {
         /** A unique notification ID. */
         notificationid_t notificationID;
      }
         o;
   };


   /**
    *  @brief Argument passed with the DCMD_FND_NOTIFY_SERVER_AVAILABLE_EX command.
    */
   struct SFNDNotificationDescription
   {
      /* user defined cookie that will be returned in the response */
      uint32_t cookie ;
      /** Information about the pulse to send when the interface is available. */
      struct SFNDPulseInfo pulse;
      /** The interface description. */
      struct SFNDInterfaceDescription ifDescription;
   };

   struct SFNDNotificationID
   {
      /* the cookie specified in the request */
      uint32_t cookie ;
      /* the notification id */
      notificationid_t notificationID ;
   };

   union SFNDNotifyServerAvailableExArg
   {
      /**
       *  @brief Arguments passed to the service broker.
       */
      struct
      {
         /** @brief The service broker version. */
         struct SFNDInterfaceVersion sbVersion;
         /** @brief size of the notificaion list */
         int32_t count ;
         /** @brief list of notifications to set */
         /* SFNDNotificationDescription notifications[count]; */
      }
         i;

      /**
       * @brief Arguments passed from the service broker to the client.
       */
      struct
      {
         /** @brief list of notification IDs. */
         struct SFNDNotificationID notifications[1];
      }
         o;
   };

   /**
    *  @brief Argument passed with the DCMD_FND_DETACH_INTERFACE command.
    */
   union SFNDInterfaceDetachArg
   {
      struct
      {
         /** The service broker version. */
         struct SFNDInterfaceVersion sbVersion;
         /** Reserved for future use. */
         uint32_t reserved ;
         /** The ID of the client to detach. */
         SPartyID clientID;
      }
         i;
   };


   /**
    * Authentication ping from slave to master servicebroker.
    * When connecting a slave to the master the slave sends this ping at the beginning
    * in order to authenticate itself with the id.
    *
    * @todo This packet may probably moved into the 'AUTH' header which then must send its size so
    * older server implementations may interact with newer clients and vice versa.
    */
   union SFNDMasterPingIdArg
   {
      struct
      {
         struct SFNDInterfaceVersion sbVersion;   ///< The service broker version.

         uint32_t id;                             ///< The extended id of the connecting slave.
         uint32_t reserved;                       ///< Reserved for any purpose.
      }
         i;
   };

   struct SFNDMasterPingIdArgExt
   {
      union SFNDMasterPingIdArg arg;

      uint32_t slaveIP;                        ///< The peer IP of the slave servicebroker as seen from the master.
      uint32_t masterIP;                       ///< The masters local interface IP over which the slave servicebroker is reachable.
   };


   /**
    *  @brief Argument passed with the DCMD_FND_NOTIFY_CLIENT_DETACH command.
    */
   union SFNDNotifyClientDetachArg
   {
      /**
       * @brief Arguments passed to the service broker.
       */
      struct
      {
         /** The service broker version. */
         struct SFNDInterfaceVersion sbVersion;
         /** Reserved for future use. */
         uint32_t reserved ;
         /** The ID of the client. */
         SPartyID clientID;
         /** Information about the pulse to send when the client has been detached. */
         struct SFNDPulseInfo pulse;
      }
         i;

      /**
       * @brief Arguments passed from the service broker to the client.
       */
      struct
      {
         /** A unique notfication ID. */
         notificationid_t notificationID;
      }
         o;
   };

   /**
    *  @brief Argument passed with the DCMD_FND_CLEAR_NOTIFICATION command.
    */
   union SFNDClearNotificationArg
   {
      /**
       * @brief Arguments passed to the service broker.
       */
      struct
      {
         /** The service broker version. */
         struct SFNDInterfaceVersion sbVersion;
         /** The ID of the notification to remove. */
         notificationid_t notificationID;
      }
         i;
   };


   /**
    *  @brief Argument passed with the DCMD_FND_GET_INTERFACELIST command.
    */
   union SFNDGetInterfaceListArg
   {
      struct
      {
         /** The service broker version. */
         struct SFNDInterfaceVersion sbVersion ;
         /** buffer element count */
         uint32_t inElementCount;
      }
         i ;

      /**
       * @brief Arguments passed to the service broker.
       */
      struct
      {
         /** the element count within output, could be smaller than inElementCount */
         //uint32_t outElementCount;

         struct SFNDInterfaceDescription interfaceDescriptions[1];
      }
         o ;
   };


   union SFNDNotifyInterfaceListChangeArg
   {
      /**
       * @brief Arguments passed to the service broker.
       */
      struct
      {
         /** The service broker version. */
         struct SFNDInterfaceVersion sbVersion;
         /** Information about the pulse to send when the client has been detached. */
         struct SFNDPulseInfo pulse;
      }
         i;

      /**
       * @brief Arguments passed from the service broker to the client.
       */
      struct
      {
         /** A unique notfication ID. */
         notificationid_t notificationID;
      }
         o;
   };


   /**
    *  @brief Argument passed with the DCMD_FND_MATCH_INTERFACELIST command.
    */
   union SFNDMatchInterfaceListArg
   {
      struct
      {
         /** The service broker version. */
         struct SFNDInterfaceVersion sbVersion ;
         /** buffer element count */
         uint32_t inElementCount;
         /** the regular expression */
         char regExpr[NAME_MAX+1] ;
      }
         i ;

      /**
       * @brief Arguments passed to the service broker.
       */
      struct
      {
         /** the element count within output, could be smaller than inElementCount */
         //uint32_t outElementCount;

         struct SFNDInterfaceDescription interfaceDescriptions[1];
      }
         o ;
   };

   union SFNDNotifyInterfaceListMatchArg
   {
      /**
       * @brief Arguments passed to the service broker.
       */
      struct
      {
         /** The service broker version. */
         struct SFNDInterfaceVersion sbVersion;
         /** Information about the pulse to send when the client has been detached. */
         struct SFNDPulseInfo pulse;
         /** the regular expression */
         char regExpr[NAME_MAX+1] ;
      }
         i;

      /**
       * @brief Arguments passed from the service broker to the client.
       */
      struct
      {
         /** A unique notfication ID. */
         notificationid_t notificationID;
      }
         o;
   };

/** @brief Command to register an interface. */
#define DCMD_FND_REGISTER_INTERFACE              ((dcmd_t) (__DIOTF(_DCMD_MISC,0,union SFNDInterfaceRegisterArg)))

/** @brief Command to unregister an interface. */
#define DCMD_FND_UNREGISTER_INTERFACE            ((dcmd_t) (__DIOTF(_DCMD_MISC,1,union SFNDInterfaceUnregisterArg)))

/** @brief Command to attach to an interface. */
#define DCMD_FND_ATTACH_INTERFACE                ((dcmd_t) (__DIOTF(_DCMD_MISC,2,union SFNDInterfaceAttachArg)))

/** @brief Command to detach an attached interface. */
#define DCMD_FND_DETACH_INTERFACE                ((dcmd_t) (__DIOTF(_DCMD_MISC,3,union SFNDInterfaceDetachArg)))

/** @brief Command to set a 'server disconnect' notification. */
#define DCMD_FND_NOTIFY_SERVER_DISCONNECT        ((dcmd_t) (__DIOTF(_DCMD_MISC,4,union SFNDNotifyServerDisconnectArg)))

/** @brief Command to set a 'server available' notification. */
#define DCMD_FND_NOTIFY_SERVER_AVAILABLE         ((dcmd_t) (__DIOTF(_DCMD_MISC,5,union SFNDNotifyServerAvailableArg)))
#define DCMD_FND_NOTIFY_SERVER_AVAILABLE_EX      ((dcmd_t) (__DIOTF(_DCMD_MISC,5,union SFNDNotifyServerAvailableExArg)))

/** @brief Command to set a 'client detached' notification. */
#define DCMD_FND_NOTIFY_CLIENT_DETACH            ((dcmd_t) (__DIOTF(_DCMD_MISC,6,union SFNDNotifyClientDetachArg)))

/** @brief Command to clear a notification. */
#define DCMD_FND_CLEAR_NOTIFICATION              ((dcmd_t) (__DIOTF(_DCMD_MISC,7,union SFNDClearNotificationArg)))

/** @brief Command receive a serverlist. */
#define DCMD_FND_GET_INTERFACELIST               ((dcmd_t) (__DIOTF(_DCMD_MISC,8,union SFNDGetInterfaceListArg)))

/** @brief Command set a 'serverlist change' notification */
#define DCMD_FND_NOTIFY_INTERFACELIST_CHANGE     ((dcmd_t) (__DIOTF(_DCMD_MISC,9,union SFNDNotifyInterfaceListChangeArg)))

/** @brief Command receive a serverlist. */
#define DCMD_FND_MATCH_INTERFACELIST             ((dcmd_t) (__DIOTF(_DCMD_MISC,10,union SFNDMatchInterfaceListArg)))

/** @brief Command set a 'serverlist change' notification */
#define DCMD_FND_NOTIFY_INTERFACELIST_MATCH      ((dcmd_t) (__DIOTF(_DCMD_MISC,11,union SFNDNotifyInterfaceListMatchArg)))

/** @brief Command to register multiple interfaces at a time */
#define DCMD_FND_REGISTER_INTERFACE_EX           ((dcmd_t) (__DIOTF(_DCMD_MISC,12,union SFNDInterfaceRegisterExArg)))

/** @brief Command to register an interfaces with restricted access rights */
#define DCMD_FND_REGISTER_INTERFACE_GROUPID      ((dcmd_t) (__DIOTF(_DCMD_MISC,13,union SFNDInterfaceRegisterGroupIDArg)))

/** @brief Command to register an interface. */
#define DCMD_FND_REGISTER_MASTER_INTERFACE_EX    ((dcmd_t) (__DIOTF(_DCMD_MISC,30,union SFNDInterfaceRegisterMasterExArg)))

/** @brief Slave SBs ping their master so connections would be kept alive. */
#define DCMD_FND_MASTER_PING                     ((dcmd_t) (__DION(_DCMD_MISC,31)))

/** @brief Slave SBs ping their master in TCP mode with this package. */
#define DCMD_FND_MASTER_PING_ID                  ((dcmd_t) (__DIOTF(_DCMD_MISC,32,union SFNDMasterPingIdArg)))

/** @brief Command to set a 'server disconnect' notification. */
#define DCMD_FND_ATTACH_INTERFACE_EXTENDED                ((dcmd_t) (__DIOTF(_DCMD_MISC,35,union SFNDInterfaceAttachExtendedArg)))

/** @brief Command to attach to an interface. */
#define DCMD_FND_GET_SERVER_INFORMATION                ((dcmd_t) (__DIOTF(_DCMD_MISC,37,union SFNDGetServerInformation)))


   /**
    * @brief Contains symbolc values for the status returned by a devctl().
    */
   typedef enum _SBStatus
   {
      /** Result is OK, operation was successfull. */
      FNDOK,

      /**
       * @brief Interface version mismatch.
       *
       * An interface with a matching name was registered, but the required version
       * does not match.
       */
      FNDVersionMismatch,

      /** @brief There is no registered interface with the passed name. */
      FNDUnknownInterface,

      /** @brief An interface with the same name and version is already registered. */
      FNDInterfaceAlreadyRegistered,

      /** @brief The passed server ID is invalid. */
      FNDInvalidServerID,

      /** @brief The passed client ID is invalid. */
      FNDInvalidClientID,

      /** @brief The passed notification ID is invalid. */
      FNDInvalidNotificationID,

      /** @brief one ore more arguments are not valid. */
      FNDBadArgument,

      /** @brief one ore more arguments are not valid. */
      FNDBadFoundationVersion,

      /** @brief the regular expression is not valid */
      FNDRegularExpression,

      /** @brief access to the interface is not allowed */
      FNDAccessDenied,

      /** @brief Unknown command evaluated on server side */
      FNDUnknownCommand,

      /** @brief internal server error */
      FNDInternalError

   } SBStatus ;

#if defined(__cplusplus)
}
#endif /* __cplusplus defined */

#endif /* DSI_PRIVATE_SERVICEBROKER_H */
