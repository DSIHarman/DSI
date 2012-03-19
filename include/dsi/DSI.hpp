/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_DSI_HPP
#define DSI_DSI_HPP

#include <stdint.h>

#include "dsi/clientlib.h"

#include <cstring>


/// maximum packet size of DSI message including eventinfo and header
#define DSI_PACKET_SIZE (4*1024)

/// maximum payload size (still including eventinfo
#define DSI_PAYLOAD_SIZE DSI_PACKET_SIZE - sizeof(DSI::MessageHeader)


/**
 * DSI - Distributed Service Interface. This namespace contains of classes necessary
 * to build DSI clients and servers.
 */
namespace DSI
{
   /**
    * @internal
    * The lowest ID a service interface request may have
    */
   static const uint32_t REQUEST_ID_FIRST = 0x00000000u ;

   /**
    * @internal
    * The highest ID a service interface request may have
    */
   static const uint32_t REQUEST_ID_LAST = 0x7FFFFFFFu ;

   /**
    * @internal
    * The lowest ID a service interface response or information may have
    */
   static const uint32_t RESPONSE_ID_FIRST = 0x80000000u ;

   /**
    * @internal
    * The highest ID a service interface response or information may have
    */
   static const uint32_t RESPONSE_ID_LAST = 0xBFFFFFFFu ;

   /**
    * @internal
    * The lowest ID a service interface attribute may have
    */
   static const uint32_t ATTRIBUTE_ID_FIRST = 0xC0000000u ;

   /**
    * @internal
    * The highest ID a service interface attribute may have
    */
   static const uint32_t ATTRIBUTE_ID_LAST = 0xFFFFFFFFu ;

   static const int32_t INVALID_SEQUENCE_NR = 0 ;
   static const int32_t INVALID_SESSION_ID = -1 ;
   static const int32_t INVALID_NOTIFICATION_ID = -1;

   static const uint32_t INVALID_ID = ATTRIBUTE_ID_LAST ;


   enum Command
   {
      Invalid           =  0,

      DataRequest       =  7,       ///< a data request transfer is initiated
      DataResponse      =  8,       ///< a data response transfer is initiated
      ConnectRequest    =  9,       ///< a connection is requested
      DisconnectRequest = 10,       ///< a disconnection is requested
      ConnectResponse   = 11        ///< response to a connect request
   };


   /**
    * DSI message header as transferred on the wire.
    */
   struct MessageHeader
   {
      int32_t type;            ///< The type of the message (unique for all messages)

      uint16_t protoMajor;     ///< Protocol Major Version
      uint16_t protoMinor;     ///< Protocol Minor Version

      SPartyID serverID;       ///< The ID of the server which sends/receives this message.
      SPartyID clientID;       ///< The ID of the client which sends/receives this message.

      uint32_t cmd;            ///< The command.
      uint32_t flags;          ///< Flasgs (bit 1 indicates if there is at least one more packet comming
      uint32_t packetLength;   ///< The length of this packet (without message header).

      int32_t reserved[1];     ///< reserved (fillup for 8 byte-alignment)

      /**
       * Creates an uninitialized message header.
       */      
      MessageHeader();
      
      /**
       * Creates an initialized message header with given attributes.
       */
      MessageHeader(SPartyID serverID, SPartyID clientID, Command cmd, uint16_t protoMinor = DSI_PROTOCOL_VERSION_MINOR, uint32_t packetLength = 0);
   };


   enum RequestType
   {
      REQUEST                          = 0x0100,  ///< Standard request
      REQUEST_NOTIFY                   = 0x0101,  ///< Register for notifications for a data attribute or for a response method
      REQUEST_STOP_NOTIFY              = 0x0102,  ///< Clear any existing notification for a data attribute or for a response method
      REQUEST_LOAD_COMPONENT           = 0x0103,  ///< Instantiate the component this request is sent to
      REQUEST_STOP_ALL_NOTIFY          = 0x0104,  ///< Clear all existing notification for a data attribute or for a response method
      REQUEST_REGISTER_NOTIFY          = 0x0105,  ///< Register for registzered notifications for a for a response method or information method
      REQUEST_STOP_REGISTER_NOTIFY     = 0x0106,  ///< Clear for registered notifications for a for a response method or information method
      REQUEST_STOP_ALL_REGISTER_NOTIFY = 0x0107   ///< Clear for all registered notifications for a for a response method or information method
   } ;


   enum ResultType
   {
      RESULT_OK            = 0x0200,   ///< The response method of a request has been called, the 'error' method wasn't called
      RESULT_INVALID       = 0x0201,   ///< The 'error' method was called with a result method id as argument
      RESULT_DATA_OK       = 0x0202,   ///< A data attribute has been updated, the new value is valid
      RESULT_DATA_INVALID  = 0x0203,   ///< A data attribute has been marked as invalid, the old value is preserved - only the status changes
      RESULT_REQUEST_ERROR = 0x0204,   ///< The 'error' method was called with a request method id as argument
      RESULT_REQUEST_BUSY  = 0x0205    ///< A call to this request has already been received but the response has not yet been made.
   } ;


   /**
    * brief This enum is used to identify the states of notifiable data elements.
    */
   enum DataStateType
   {
      DATA_NOT_AVAILABLE,     ///< Initial value for constructor
      DATA_INVALID,           ///< The current value of this data attribute (or response parameter) is marked as invalid
      DATA_OK                 ///< The current value of this data attribute (or response parameter) is valid
   } ;


   enum UpdateType
   {
      UPDATE_NONE = -1,

      UPDATE_COMPLETE = 0,
      UPDATE_INSERT,
      UPDATE_REPLACE,
      UPDATE_DELETE
   } ;


   /**
    * Holds all event information.
    */
   struct EventInfo
   {
      /**
       * The version of the service interface.
       * High word contains major version, low word contains minor version.
       */
      uint32_t ifVersion;

      /**
       * the type of request or response
       */
      union
      {
         RequestType requestType;
         ResultType responseType;
      };

      uint32_t requestID;       ///< the request ID passed to the constructor
      int32_t sequenceNumber;   ///< the sequence number

      inline
      EventInfo()
      {
         memset(this, 0, sizeof(*this));
      }
   };


   struct Size
   {
      inline
      Size( uint32_t s = 0 )
       : size(s)
       , padding(0)
      {
         // NOOP
      }

      inline
      operator uint32_t()
      {
         return size;
      }

      uint32_t size;
      int32_t  padding;
   };

   const char* toString( RequestType rtype );
   const char* toString( ResultType rtype );


   inline
   bool isAttributeId( uint32_t id )
   {
      return (id >= ATTRIBUTE_ID_FIRST && id <= ATTRIBUTE_ID_LAST);
   }


   inline
   bool isResponseId( uint32_t id )
   {
      return (id >= RESPONSE_ID_FIRST && id <= RESPONSE_ID_LAST);
   }

}   // namespace DSI


#endif // DSI_DSI_HPP

