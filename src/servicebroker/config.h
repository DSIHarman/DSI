#ifndef DSI_SERVICEBROKER_CONFIG_H
#define DSI_SERVICEBROKER_CONFIG_H


#include <stdint.h>

/// a linux server always provides a HTTP service to have the 'qnxcat' like interface like on QNX
/// The contents of the output is identical with that on QNX and WIN32
#define HAVE_HTTP_SERVICE 1

/// The QNX and Win32 servicebrokers can be implemented in the way that they do message passing for local node and
/// do TCP/IP as slave servicebroker to a Linux master. The Linux servicebroker can also be configured in that way
/// but never to use message passing from a slave servicebroker to the master. This mode helps to do inter-node
/// DSI via TCP/IP without change of the current DSI implementation.

/// slave servicebroker connections will be tracked on protocol timeouts to be closed in-time
#define HAVE_TCP_CLIENT_TIMEOUT_SUPERVISION 1

/**
 * Enabling the switch optimizes the inter-process, inter-platform communication. In this case the clients are managed
 * on the platform where they exist ant not anymore on the master. The servicebrokers must run in tree mode because the
 * caching needs that each servicebroker to have a distinct id.
 */
//#define USE_SERVER_CACHE

#define SB_LOCAL_NODE_ADDRESS 0
#define SB_LOCAL_IP_ADDRESS   0x7f000001  // 127.0.0.1
#define SB_UNKNOWN_USER_ID    0xFFFFFFFF /*-1*/
#define SB_UNKNOWN_GROUP_ID   0xFFFFFFFF /*-1*/
#define SB_UNKNOWN_IP_ADDRESS -1

/// ping timeout for inactive slave -> master connections. If no requests are sent from the slave to the master
/// the master gets pinged by the slave periodically with this timeout in milliseconds.
#define SB_MASTERPING_TIMEOUT 2000

/// timeout in milliseconds for waiting in master adapter between connect tries
#define SB_MASTERADAPTER_RECONNECT_TIMEOUT 200

#define _PULSE_CODE_MINAVAIL 0

/// tcp timeout in milliseconds for sending or receiving stuff to and from master (only tcp master-slave and death-detection connection)
#define SB_MASTERADAPTER_SENDTIMEO 2000
#define SB_MASTERADAPTER_RECVTIMEO 5000

/// Have a look on the phone if you wonder what 3746 stands for. Since all slaves use the same port for their
/// pulse socket only one slave can run on a node. The http server port can be modified by the environment variable
/// SB_HTTP_PORT set to the appropriate (free) port.
#define SB_MASTER_PORT    3746
#define SB_SLAVE_PORT     3747
#define SB_HTTP_PORT      3744


static const int32_t PULSE_MASTER_CONNECTED    = _PULSE_CODE_MINAVAIL + 10 ;
static const int32_t PULSE_MASTER_DISCONNECTED = _PULSE_CODE_MINAVAIL + 11 ;
static const int32_t PULSE_JOB_EXECUTED        = _PULSE_CODE_MINAVAIL + 12 ;


#endif   // DSI_SERVICEBROKER_CONFIG_H
