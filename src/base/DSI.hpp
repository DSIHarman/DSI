/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_BASE_DSI_HPP
#define DSI_BASE_DSI_HPP


/// multi packet message scattering flag
#define DSI_MORE_DATA_FLAG 1   

/// DSI message magic
#define DSI_MESSAGE_MAGIC 0x200


namespace DSI 
{
   enum PulseCode
   {
      PULSE_SERVER_AVAILABLE      = 100,
      PULSE_SERVER_DISCONNECT     = 101,
      PULSE_CLIENT_DETACHED       = 102      
   };
   
   
   /**
    * @brief ConnectRequest data for local connections (only exchanged via local transport).
    */
   struct ConnectRequestInfo
   {      
      uint32_t pid;         ///< process id in host-byte-order
      uint32_t channel;     ///< channel id (socket fd) in host-byte-order         
   };

   
   /**
    * @brief ConnectRequest data for TCP/IP connections (only exchanged via TCP/IP).
    *
    * These byteorders should be changed to network-byte-order but then
    * we would drop interoperability of TCP/IP connections with older H/B
    * DSI over TCP/IP implementations.
    */
   struct TCPConnectRequestInfo
   {
      uint32_t ipAddress;   ///< ip address in big endian byte-order
      uint32_t port;        ///< port in big endian byte-order
   };

   
   const char* toString(PulseCode code);
   
   const char* commandToString(uint32_t);
   
   inline
   const char* toString(Command cmd)
   {
      return commandToString(static_cast<uint32_t>(cmd));
   }
   
   /**
    * Returns the local IPv4 address as unsigned integer in host byte order.
    */
   uint32_t getLocalIpAddress();                
   
   /**
    * Returns the local IPv4 address as string in dotted quad notation.
    */
   const char* getLocalIpAddressString();
   
   /**
    * Find an object by internal id. An internally identifiable object must 
    * expose the method getId().
    */
   class FindById
   {
   public:
   
      explicit inline
      FindById(int32_t id)
         : mId(id)
      {
         // NOOP
      }

      template<typename T>
      inline
      bool operator()(T t)
      {
         return t.getId() == mId;
      }

      template<typename T>
      inline
      bool operator()(T* t)
      {
         return t->getId() == mId;
      }
      
   private:
   
      int32_t mId;
   };

}   // namespace DSI


#endif   // DSI_BASE_DSI_HPP
