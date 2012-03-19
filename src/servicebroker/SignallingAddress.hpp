/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_SIGNALLINGADDRESS_HPP
#define DSI_SERVICEBROKER_SIGNALLINGADDRESS_HPP


/**
 * Store information about how notification pulses should be delivered.
 * Just for removing the pid/chid stuff from Servicebroker.cpp.
 */
union SignallingAddress
{
   /** 
    * The underlying subsystem supports message passing.
    */
   struct
   {
      int32_t pid;
      int32_t chid;
   } mp;
   
   /** 
    * The underlying subsystem supports TCP or UDP sockets.
    */
   struct
   {
      int32_t ip;
      int32_t port;
   } ip; 

   /** 
    * The is an independent coding in a major/minor switch.
    */
   struct   
   {
      int32_t major;  
      int32_t minor;  
   } internal;
};


#endif   // DSI_SERVICEBROKER_SIGNALLINGADDRESS_HPP
