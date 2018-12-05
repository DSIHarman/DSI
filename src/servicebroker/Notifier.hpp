/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_NOTIFIER_HPP
#define DSI_SERVICEBROKER_NOTIFIER_HPP


class Job;


/**
 * Only used in socket version.
 */
struct InternalNotification
{
   uint32_t code;
   uint64_t value;
};   


/**
 * Internal event sender. Only necessary for servicebroker slaves.
 */
class Notifier
{
public:

   /** 
    * Channel to attach to or valid file descriptor to send data over (implementation dependent).
    */
   Notifier(int fd);   
   ~Notifier();
      
   /**
    * Send event that the given job finished execution.
    */
   void jobFinished(Job& job);   
      
   /**
    * Send event that the master is connected (again).
    */      
   void masterConnected();                     
   
   /**
    * Send event that the master is disconnected (again).
    */
   void masterDisconnected();                     
      
   /**
    * Send a pulse with given code/value pair to the local event dispatcher.
    */ 
   void deliverPulse(int code, int value);
           
private:

   int fd_;
   int prio_;
};


#endif   // DSI_SERVICEBROKER_NOTIFIER_HPP
