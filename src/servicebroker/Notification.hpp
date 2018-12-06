/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_NOTIFICATION_HPP
#define DSI_SERVICEBROKER_NOTIFICATION_HPP


#include "dsi/private/servicebroker.h"

#include <sstream>

#include "config.h"
#include "Log.hpp"
#include "IDList.hpp"
#include "RegExp.hpp"
#include "InterfaceDescription.hpp"
#include "ConnectionContext.hpp"


// forward decls
class ClientSpecificData ;
class SocketConnectionContext;

/**
 * @internal
 *
 * @brief Describes a single notification list entry.
 */
class Notification
{
public:

   Notification();
   Notification(const Notification&); // copy constructor (with move semantic) -> changes original object!

   ~Notification();

   static const notificationid_t INVALID_NOTIFICATION_ID = 0xffffffff ;
   
   /*
    * Connects the notification
    */
   bool connect( const SocketConnectionContext& ctx, SFNDPulseInfo &pulse );

   /*
    * Sends the notification
    */
   void send();

   /*
    * sets and compiles a regular expression
    */
   bool setRegExp( const char* regexp );

   /*
    * matches against a previously set regular expression
    */
   bool matchRegExp( const std::string& string );

   /*
    * Does this notification object have a notification?
    */
   bool hasRegExp();

   /*
    * Dumps debugging information to the provided buffer.
    */
   void dumpStats( std::ostringstream& ostream ) const;

   static unsigned int getTotalCounter();
   static unsigned int getCurrentCounter();
   
   ///The type of a notification can change from connect to disconnect
   void setType(const SPartyID &partyID);

   /** The unique notification ID. */
   notificationid_t notificationID;
   /** the notification ID at the master */
   notificationid_t masterNotificationID ;
   /** ID of the notification pool this notification belongs to */
   uint32_t poolID ;
   /** Pointer to the ID list where the notification ID is hosted. */
   ClientSpecificData* host ;
   /** The client. */
   SPartyID partyID ;
   /** The interface description */
   InterfaceDescription ifDescription ;
   /** is it a local notification? */
   bool local ;
   /** user id of the owner process of the notification */
   uid_t uid ;
   /** flag that indicates that the notification is not active any more */
   bool active ;

private:  

   void writeMessage(Log::eSBLogType type, const char* message) const;

   /** regular expression for a interface change match notification */
   RegExp* regExpr ;
   /** The pulse data */
   SFNDPulseInfo pulse ;

   int coid;    ///< The connection id.

   /** The priority for the notification. */
   int prio;
   
   /** node id of the target */
   int nid;

   //statistical counters
   static unsigned int totalCounter;
   static unsigned int currentCounter;

   /**
    * Really does connect the channel/socket. This may be deferred by setting an
    * appropriate compiler flag within config.h. Not that the notification will first be
    * disconnected within the destructor.
    */
   bool doConnect();

   /** no copy operation supported */
   Notification& operator=(const Notification&);
};


inline 
bool Notification::hasRegExp()
{
   return regExpr != nullptr;
}


inline 
unsigned int Notification::getTotalCounter()
{
   return totalCounter;
}


inline 
unsigned int Notification::getCurrentCounter()
{
   return currentCounter;
}

inline 
void Notification::setType(const SPartyID &aPartyID)
{
   partyID = aPartyID;
}

#endif /* DSI_SERVICEBROKER_NOTIFICATION_HPP */
