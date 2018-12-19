/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "dsi/DSI.hpp"
#include "dsi/Streaming.hpp"

#include <arpa/inet.h>
#include <cstdlib>

#include "DSI.hpp"


namespace DSI
{

// --------------------------------------------------------------------------------------


  MessageHeader::MessageHeader()
   : type(0)
   , protoMajor(0)
   , protoMinor(0)    
   , cmd(0)
   , flags(0)
   , packetLength(0)
  {
    serverID.globalID = 0;
    clientID.globalID = 0;
    reserved[0] = 0;
  }


  MessageHeader::MessageHeader(SPartyID serverID, SPartyID clientID, Command cmd, uint16_t protoMinor, uint32_t packetLength)
   : type(DSI_MESSAGE_MAGIC)
   , protoMajor(DSI_PROTOCOL_VERSION_MAJOR)
   , protoMinor(protoMinor)
   , serverID(serverID)
   , clientID(clientID)
   , cmd(static_cast<uint32_t>(cmd))
   , flags(0)
   , packetLength(packetLength)
  {
    reserved[0] = 0;
  }


// --------------------------------------------------------------------------------------


  uint32_t getLocalIpAddress()
  {
    int32_t addr = inet_addr(getLocalIpAddressString());
    return ntohl(addr);
  }


  const char* getLocalIpAddressString()
  {
    static const char *myAddress = nullptr;

    if( !myAddress )
    {
      myAddress = ::getenv("DSI_IP_ADDRESS");

      if( !myAddress )
      {
        myAddress = "127.0.0.1";
      }
    }

    return myAddress;
  }


  const char* toString( RequestType rtype )
  {
    switch( rtype )
    {
      case REQUEST:
        return "REQUEST";
      case REQUEST_NOTIFY:
        return "REQUEST_NOTIFY";
      case REQUEST_STOP_NOTIFY:
        return "REQUEST_STOP_NOTIFY";
      case REQUEST_LOAD_COMPONENT:
        return "REQUEST_LOAD_COMPONENT";
      case REQUEST_STOP_ALL_NOTIFY:
        return "REQUEST_STOP_ALL_NOTIFY";
      case REQUEST_REGISTER_NOTIFY:
        return "REQUEST_REGISTER_NOTIFY";
      case REQUEST_STOP_REGISTER_NOTIFY:
        return "REQUEST_STOP_REGISTER_NOTIFY";
      case REQUEST_STOP_ALL_REGISTER_NOTIFY:
        return "REQUEST_STOP_ALL_REGISTER_NOTIFY";
      default:
        return "UNKNOWN" ;
    }
  }


  const char* toString(ResultType rtype)
  {
    switch(rtype)
    {
      case RESULT_OK:
        return "RESULT_OK";
      case RESULT_INVALID:
        return "RESULT_INVALID";
      case RESULT_DATA_OK:
        return "RESULT_DATA_OK";
      case RESULT_DATA_INVALID:
        return "RESULT_DATA_INVALID";
      case RESULT_REQUEST_ERROR:
        return "RESULT_REQUEST_ERROR";
      case RESULT_REQUEST_BUSY:
        return "RESULT_REQUEST_BUSY";
      default:
        return "UNKNOWN" ;
    }
  }


  const char* commandToString(uint32_t cmd)
  {
    switch(cmd)
    {
      case Invalid:
        return "Invalid";
      case DataRequest:
        return "DataRequest";
      case DataResponse:
        return "DataResponse";
      case ConnectRequest:
        return "ConnectRequest";
      case DisconnectRequest:
        return "DisconnectRequest";
      default:
        return "UNKNOWN";
    }
  }


  const char* toString(PulseCode code)
  {
    switch(code)
    {
      case PULSE_SERVER_AVAILABLE:
        return "PULSE_SERVER_AVAILABLE";
      case PULSE_SERVER_DISCONNECT:
        return "PULSE_SERVER_DISCONNECT";
      case PULSE_CLIENT_DETACHED:
        return "PULSE_CLIENT_DETACHED";
      default:
        return "PULSE_UNKNOWN";
    }
  }


} // namespace DSI
