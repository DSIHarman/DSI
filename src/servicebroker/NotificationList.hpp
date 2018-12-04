/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_NOTIFICATIONLIST_HPP
#define DSI_SERVICEBROKER_NOTIFICATIONLIST_HPP


#include "Notification.hpp"

#include <list>
#include <sstream>

#include "IDList.hpp"

static const int32_t PULSE_MASTER_SERVER_AVAILABLE  = _PULSE_CODE_MINAVAIL + 20 ;
static const int32_t PULSE_MASTER_SERVER_DISCONNECT = _PULSE_CODE_MINAVAIL + 21 ;
static const int32_t PULSE_MASTER_CLIENT_DETACH     = _PULSE_CODE_MINAVAIL + 22 ;
static const int32_t PULSE_MASTER_SERVERLIST_CHANGE = _PULSE_CODE_MINAVAIL + 23 ;
static const int32_t PULSE_MASTER_ATTACH_EXTENDED   = _PULSE_CODE_MINAVAIL + 24 ;


/**
 * @internal
 *
 * @brief Describes a notification list.
 */
class NotificationList
{
public:
   NotificationList();
   ~NotificationList();

   /**
    * @internal
    *
    * @brief Triggers a single notification.
    *
    * Calling this function triggers a single notification and removes the entry
    * from the list. The entry is also removed from its hosting list.
    *
    * @param id The server/client ID of the notification to trigger.
    */
   /*lint -sem(notificationListTrigger,1p) */
   void trigger( const SPartyID &id );
   void trigger( const InterfaceDescription &ifDescription, gid_t grpid = SB_UNKNOWN_GROUP_ID );
   void trigger( notificationid_t notificationID, bool remove = true );
   void triggerPool( notificationid_t poolID );
   void triggerNotLocal( notificationid_t extendedID );
   void triggerLocal(notificationid_t extendedID);

   /**
    * @internal
    *
    * @brief Triggers multiple notifications.
    *
    * Calling this function triggers all notifications hosted by the passed ID list.
    * The entries are removed from the notification list and the hosting ID list.
    *
    * @param host Pointer to the hosting ID list. This parameter must not be NULL.
    */
   /*lint -sem(notificationListTriggerAll,1p,2p) */
   void trigger( const PartyIDList &host );

   /**
    * @internal
    *
    * @brief Triggers all notifications in the list without removing them afterwards.
    */
   void triggerAll();
   void triggerAll( const InterfaceDescription &ifDescription );

   /**
    * @internal
    *
    * @brief Adds a notification to a notification list.
    *
    * @param host Pointer to the ID list in which the notification ID is contained.
    * @param notificationID Unique identifier for this notification.
    * @param chid The channel ID to send the notification pulse to.
    * @param pid The process id of the originator process - to be read from the connection context.
    * @param code The pulse code for the notification.
    * @param value The pulse value for the notification.
    *
    * @return the new notification id or INVALID_NOTIFICATION_ID on error
    */

   notificationid_t add( const SocketConnectionContext& ctx, SFNDPulseInfo &pulse );

   notificationid_t add( const char* regExpr, const SocketConnectionContext& ctx, SFNDPulseInfo &pulse );

   notificationid_t add( ClientSpecificData *host, const SPartyID &clientID, const SocketConnectionContext& ctx, SFNDPulseInfo &pulse );

   notificationid_t add( ClientSpecificData *host, const InterfaceDescription &ifDescription, const SocketConnectionContext& ctx, SFNDPulseInfo &pulse, bool local, uid_t uid = SB_UNKNOWN_USER_ID );

   /**
    * Add a notification already available from other list.
    */
   void add( Notification& aNotification );

   /**
    * @internal
    *
    * @brief Removes an entry from a notification list.
    *
    * @param list Pointer to the notification list. This parameter must not be NULL.
    * @param notificationID The ID of the notification to remove.
    */
   void remove( notificationid_t notificationID );

   /**
    * @internal
    *
    * @brief Removes all entries hosted by an ID list.
    *
    * @param idList Pointer to the ID list. This argument must not be NULL.
    */
   /*lint -sem(notificationListRemoveAll,1p,2p) */
   void remove( const NotificationIDList &idList );

   /**
    * @internal
    *
    * @brief tries to find a notification with the given notification id
    *        in the list
    *
    * @param notificationID the notification id
    * @return the Notification entry of NULL if the notification was not found.
    */
   Notification* find( notificationid_t notificationID );

   /**
    * Writes statistics about the notification list to a buffer
    *
    * @param ostream stre output stream
    */
   void dumpStats( std::ostringstream& ostream ) const;

   /**
    * @internal
    *
    * @brief sets all masterNotificationID members of all notifications to 0
    */
   int clearMasterNotification();

   /**
    * @internal
    *
    * @brief returns the size of the list
    *
    * @return the size of the list
    */
   size_t size() const;

   /**
    * @return the number of notifications set for the given SPartyID
    */
   size_t count(const SPartyID &id);

   /**
    * Moves all notifications of another list with a given poolID to this list.
    * @return true if notifications were moved
    */
   bool move(NotificationList &source, uint32_t poolID);

   /**
    * Set master notification id for all notifications in the specified pool.
    *
    * @return number of notifications set
    */
   size_t setMasterNotificationID(notificationid_t notificationID, uint32_t poolID);

   /**
    * Change notifications type from deferred to disconnect for all notifications in the specified pool.
    *
    * @return number of notifications changed
    */
   size_t setType(uint32_t poolID, const SPartyID &partyID);

   /**
    * @return pool's size
    */
   size_t poolSize(uint32_t poolID);


   typedef std::list<Notification>::iterator iterator ;
   iterator begin();
   iterator end();
   iterator erase(iterator);
   unsigned int getTotalCounter() const;

private:
   typedef std::list<Notification> CNotificationList ;
   CNotificationList mList ;

   NotificationList( const NotificationList& );
   NotificationList& operator=( const NotificationList& );

   //statistical counter
   unsigned int mTotalCounter;
};


inline 
size_t NotificationList::size() const
{
   return mList.size() ;
}


inline
NotificationList::iterator NotificationList::begin()
{
   return mList.begin();
}


inline
NotificationList::iterator NotificationList::end()
{
   return mList.end();
}


inline
NotificationList::iterator NotificationList::erase(NotificationList::iterator iter)
{
   return mList.erase(iter);
}


inline 
unsigned int NotificationList::getTotalCounter() const
{
   return mTotalCounter;
}

#endif /* DSI_SERVICEBROKER_NOTIFICATIONLIST_HPP */
