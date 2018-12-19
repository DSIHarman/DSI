#ifndef DSI_VERSION_H
#define DSI_VERSION_H

/** 
 * @file version.h 
 * 
 * This file contains protocol versioning information settings for the following protocols:
 *
 * @li inter-application DSI protocol
 * @li application-servicebroker protocol
 * @li inter-servicebroker protocol
 */

/** @brief The DSI major version number. */
#define DSI_VERSION_MAJOR 4

/** @brief The DSI minor version number. */
#define DSI_VERSION_MINOR 0


/** @brief The DSI servicebroker major version number. */
#define DSI_SERVICEBROKER_VERSION_MAJOR 4

/** @brief The DSI servicebroker minor version number. */
#define DSI_SERVICEBROKER_VERSION_MINOR 0


/** @brief The DSI protocol major version number. */
#define DSI_PROTOCOL_VERSION_MAJOR 4

/** @brief The DSI protocol minor version number. */
#define DSI_PROTOCOL_VERSION_MINOR 0


/**
 * @brief Describes an interface version.
 */
struct SFNDInterfaceVersion
{
   /** @brief The major version number of the interface. */
   uint16_t majorVersion;

   /** @brief The minor version number of the interface. */
   uint16_t minorVersion;
};


#endif // DSI_VERSION_H

