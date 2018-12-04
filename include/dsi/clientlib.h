#ifndef DSI_CLIENTLIB_H
#define DSI_CLIENTLIB_H

#include "dsi/private/servicebroker.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @file clientlib.h
 *
 * Servicebroker clientlib.
 *
 * The servicebroker clientlib provides a convenient interface to the
 * servicebroker service. Normally, a DSI user does not have ot bother with these
 * functions since they are all called from within the DSI library adequately.
 * If you ever plan to talk to the servicebroker directly you are encouraged to call
 * these functions in favour of implementing the socket protocol on your own since
 * this function interface is ment to be portable.
 */


/**
 * Opens the servicebroker and returns the handle to it.
 *
 * @param filename the absolute path to the servicebroker (usually '/srv/servicebroker')
 * @return the handle to the servicebroker or -1 if an error occurred.
 *         see errno for details.
 */
   int SBOpen( const char* filename );

/**
 * Returns a Unix socket acceptor to be used for accepting new notification clients.
 * You must call listen on the returned handle.
 */
   int SBOpenNotificationHandle();

/**
 * Closes the servicebroker handle.
 *
 * @param handle the servicebroker handle
 */
   void SBClose( int handle );

/**
 * Registers a new interface at the servicebroker
 *
 * @param handle the servicebroker handle
 * @param ifName the interface name
 * @param majorVersion the major version of the interface
 * @param minorVersion the minor version of the interface
 * @param chid the channel id through which a client can connect to the interface
 * @param serverID a pointer to a SPartyID variable where to store the server
 *        handle in
 * @return EOK on success
 */
   int SBRegisterInterface( int handle, const char* ifName,
                            int majorVersion, int minorVersion, int chid,
                            SPartyID *serverId );

/**
 * Experimental.
 *
 * @param ifName The normal interface name.
 * @param ipaddress in host byte order. If set to 0, localhost is assumed.
 * @param port in host byte order.
 */
   int SBRegisterInterfaceTCP( int handle, const char* ifName,
                               int majorVersion, int minorVersion, uint32_t ipaddress, uint16_t port,
                               SPartyID *serverId );


/**
 * Registers a new interface at the servicebroker that will only be accessible for members of
 * a certain user group.
 *
 * @param handle the servicebroker handle
 * @param ifName the interface name
 * @param majorVersion the major version of the interface
 * @param minorVersion the minor version of the interface
 * @param groupName name of the user group that is allowed to attach to the interface
 *        (see /etc/group for user groups)
 * @param chid the channel id through which a client can connect to the interface
 * @param serverId A pointer to a SPartyID variable where to store the server
 *        handle in
 * @return EOK on success
 */
   int SBRegisterGroupInterface( int handle, const char* ifName,
                                 int majorVersion, int minorVersion, int chid,
                                 const char* groupName, SPartyID *serverId );


/**
 * Registers multiple interface at the servicebroker
 *
 * @param handle the servicebroker handle
 * @param ifDescr array of SFNDInterfaceDescription structures
 * @param descrCount number of elements in ifDescr
 * @param chid the channel id through which a client can connect to the interface
 * @param serverId A pointer to an array of SPartyID that must be at least descrCount
 *        in size. Each entry in this array receives the server id of the corresponding
 *        entry in the ifDescr array. If an error occurred while registering an interface
 *        the globalId of the server id is set to -1
 * @return EOK on success
 */
   int SBRegisterInterfaceEx( int handle, const struct SFNDInterfaceDescription* ifDescr,
                              int descrCount, int chid,
                              SPartyID *serverId );

/**
 * Registers multiple interface at the servicebroker
 *
 * @param handle the servicebroker handle
 * @param ifDescr Array of SFNDInterfaceDescription structures (interface names stay as-is though the servicebroker will
 *        add a _tcp at the end of the names).
 * @param descrCount number of elements in ifDescr
 * @param ipaddress In host byte order.
 * @param port In host byte order.
 * @param serverId A pointer to an array of SPartyID that must be at least descrCount
 *        in size. Each entry in this array receives the server id of the corresponding
 *        entry in the ifDescr array. If an error occurred while registering an interface
 *        the globalId of the server id is set to -1
 * @return EOK on success
 */
   int SBRegisterInterfaceExTCP( int handle, const struct SFNDInterfaceDescription* ifDescr,
                                 int descrCount, uint32_t ipaddress, uint16_t port,
                                 SPartyID *serverId );


/**
 * Remove an existing interface from the servicebroker registry
 *
 * @param handle the servicebroker handle
 * @param serverId the id of the server
 * @return EOK on success
 */
   int SBUnregisterInterface( int handle, SPartyID serverId );

/**
 * Attach to an existing interface
 *
 * @param handle the servicebroker handle
 * @param ifName the interface name
 * @param majorVersion the major version of the interface
 * @param minorVersion the minor version of the interface
 * @param connInfo pointer to a SConnectionInfo structure that receives
 *        the information needed to connect to the interface
 * @return EOK on success
 */
   int SBAttachInterface( int handle, const char* ifName,
                          int majorVersion, int minorVersion,
                          struct SConnectionInfo *connInfo );

/**
 * Attach to an existing interface. The function reacts distinctly depending on the availability of the interface:
 * <ul>
 *  <li> if interface available: returns connection information back and sets server disconnect notification
 *  <li> if interface not available: sets a server available notification
 * </ul>
 *
 * @param handle the servicebroker handle
 * @param ifName the interface name
 * @param majorVersion the major version of the interface
 * @param minorVersion the minor version of the interface
 * @param connInfo pointer to a SConnectionInfo structure that receives
 *        the information needed to connect to the interface
 * @param chid the channel id through which a client can connect to the interface
 * @param code the pulsecode of the notification
 * @param value the value of the notification
 * @return EOK on success
 */
   int SBAttachInterfaceExtended( int handle, const char* ifName,
                                  int majorVersion, int minorVersion,
                                  struct SConnectionInfo *connInfo,
                                  int chid, int code, int value,
                                  notificationid_t *notificationID );

/**
 * Starts a service broker query for the server providing the service interface.
 *
 * @param handle the servicebroker handle
 * @param ifName the interface name
 * @param majorVersion the major version of the interface
 * @param minorVersion the minor version of the interface
 * @param serverInfo pointer to a SServerInfo structure that receives
 *        the information needed to connect to the interface
 * @return EOK on success
 */
   int SBGetServerInformation( int handle, const char* ifName,
                               int majorVersion, int minorVersion,
                               struct SServerInfo *serverInfo);

/**
 * Experimental.
 *
 * @param ifName The normal interface name.
 */
   int SBAttachInterfaceTCP( int handle, const char* ifName,
                             int majorVersion, int minorVersion,
                             struct STCPConnectionInfo *connInfo );




/**
 * Detaches a connected interface
 *
 * @param handle the servicebroker handle
 * @param clientId the client id of interface (returned by SBAttachInterface)
 * @return EOK on success
 */
   int SBDetachInterface( int handle, SPartyID clientId );

/**
 * Set a notification that will be sent as soon as the specified server
 * is available.
 *
 * @param handle the servicebroker handle
 * @param ifName the interface name
 * @param majorVersion the major version of the interface
 * @param minorVersion the minor version of the interface
 * @param chid the channel id of the channel that will receive the
 *        notification pulse. The pid is the pid of the calling
 *        process.
 * @param code the pulsecode of the notification
 * @param value the value of the notification
 * @param notificationID a pointer to an integer that receives the
 *        notification id. This id can be used to clear the notification.
 * @return EOK on success
 */
   int SBSetServerAvailableNotification( int handle, const char* ifName,
                                         int majorVersion, int minorVersion,
                                         int chid, int code, int value,
                                         notificationid_t *notificationID );

/**
 * Set a notification that will be sent when the specified server
 * disconnects.
 *
 * @param handle the servicebroker handle
 * @param serverID the id of the server
 * @param chid the channel id of the channel that will receive the
 *        notification pulse. The pid is the pid of the calling
 *        process.
 * @param code the pulsecode of the notification
 * @param value the value of the notification
 * @param notificationID a pointer to an integer that receives the
 *        notification id. This id can be used to clear the notification.
 * @return EOK on success
 */
   int SBSetServerDisconnectNotification( int handle, SPartyID serverID,
                                          int chid, int code, int value,
                                          notificationid_t *notificationID );

/**
 * Set a notification that will be sent when the specified client
 * disconnects.
 *
 * @param handle the servicebroker handle
 * @param clientID the id of the client
 * @param chid the channel id of the channel that will receive the
 *        notification pulse. The pid is the pid of the calling
 *        process.
 * @param code the pulsecode of the notification
 * @param value the value of the notification
 * @param notificationID a pointer to an integer that receives the
 *        notification id. This id can be used to clear the notification.
 * @return EOK on success
 */
   int SBSetClientDetachNotification( int handle, SPartyID clientID,
                                      int chid, int code, int value,
                                      notificationid_t *notificationID );


/**
 * Clear a notification.
 *
 * @param handle the servicebroker handle
 * @param notificationID the id of the notification to clear
 * @return EOK on success
 */
   int SBClearNotification( int handle, notificationid_t notificationID );


/**
 * Returns the list of registered interfaces from the servicebroker.
 *
 * @param handle the servicebroker handle
 * @param ifs pointer to an array of SFNDInterfaceDescription structs
 * @param inCount number of elements in the array ifs points to
 * @param outCount pointer to a variable that will receive the number of interfaces
 * @return EOK on success or an error if the handle is invalid or the buffer is
 *         not big enough.
 */
   int SBGetInterfaceList( int handle, struct SFNDInterfaceDescription *ifs,
                           int inCount, int *outCount );


/**
 * Set a notification that will be sent when a new service is available
 * or if a service was removed. This notification is not a one shot
 * notification. It will remain armed until it is cleared.
 *
 * @param handle the servicebroker handle
 * @param chid the channel id of the channel that will receive the
 *        notification pulse. The pid is the pid of the calling
 *        process.
 * @param code the pulsecode of the notification
 * @param value the value of the notification
 * @param notificationID a pointer to an integer that receives the
 *        notification id. This id can be used to clear the notification.
 * @return EOK on success
 */
   int SBSetInterfaceListChangeNotification( int handle,
                                             int chid, int code, int value,
                                             notificationid_t *notificationID );

/**
 * Returns a list of registered interfaces from the servicebroker that
 * matches a regulat expression.
 *
 * @param handle the servicebroker handle
 * @param regExpr the regular expression (man regcomp).
 * @param ifs pointer to an array of SFNDInterfaceDescription structs
 * @param inCount number of elements in the array ifs points to
 * @param outCount pointer to a variable that will receive the number of interfaces
 *        that match the regular expression. If the regular expression could not
 *        be compiled this argument returns a negative value. See the servicebroker
 *        log for details in this case.
 * @return EOK on success or an error if the handle is invalid or the buffer is
 *         not big enough.
 */
   int SBMatchInterfaceList( int handle, const char* regExpr,
                             struct SFNDInterfaceDescription *ifs,
                             int inCount, int *outCount );

/**
 * Set a notification that will be sent a newly registered interface matches the
 * provides regular expression. This notification is not a one shot notification.
 * It will remain armed until it is cleared.
 *
 * @param handle the servicebroker handle
 * @param regExpr the regular expression (man regcomp).
 * @param chid the channel id of the channel that will receive the
 *        notification pulse. The pid is the pid of the calling
 *        process.
 * @param code the pulsecode of the notification
 * @param value the value of the notification
 * @param notificationID a pointer to an integer that receives the
 *        notification id. This id can be used to clear the notification.
 * @return EOK on success
 */
   int SBSetInterfaceMatchChangeNotification( int handle, const char* regExpr,
                                              int chid, int code, int value,
                                              notificationid_t *notificationID );

/**
 * Get the dsi version of the clientlib
 * @param sbVersion a pointer to the DSI version of the servicebroker clientlib
 * @return EOK on success
 */
   void SBGetDSIVersion(struct SFNDInterfaceVersion *sbVersion );
#ifdef __cplusplus
}
#endif

#endif   // DSI_CLIENTLIB_H



