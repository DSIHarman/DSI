/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_NOTIFICATIONPOOL_HPP
#define DSI_SERVICEBROKER_NOTIFICATIONPOOL_HPP

#include <vector>

#include "dsi/TVariant.hpp"

#include "MasterAdapter.hpp"
#include "InterfaceDescription.hpp"
#include "Log.hpp"
#include "Notification.hpp"


//forward declarations
class NotificationPool;


/**
 * A notification pool plays the role of a proxy for all notifications belonging to the pool, i.e. the following
 * configuration is assumed: n local notifications : 1 local pool = proxy -> 1 remote notification.
 */
class NotificationPoolEntry
{
public:
   /**
    * State machine for a pool: init -> DEFERRED -> CONNECTING -> CONNECTED -> PRECACHING -> MONITOR_DISCONNECT
    *                           init -> DEFERRED -> MONITOR_DISCONNECT
    *                           init -> CONNECTING
    *                           init -> MONITOR_DISCONNECT
    */
   typedef enum
   {
      STATE_UNDEFINED = 0,       //used to detect initialization problems
      STATE_DEFERRED,            //pool containing notifications that are still to be decided if will be connect or
                                 //disconnect
      STATE_CONNECTING,          //pool containing connect notifications
      STATE_CONNECTED,           //server is connected, no request to get server information is sent
      STATE_PRECACHING,          //server is connected, a request to get server information is already sent
                                 //When it is used: when more clients are waiting on server information only the first
                                 //client request is sent to the master the rest of the clients will wait for this
                                 //result.
      STATE_MONITOR_DISCONNECT,  //pool containing disconnect notifications
   } EPoolState;

   NotificationPoolEntry(NotificationPool *aContainer, const InterfaceDescription& ifDescr, bool isDeferredPool = false);
   NotificationPoolEntry(NotificationPool *aContainer, const SPartyID &serverID);

   notificationid_t notificationID ;              // notification id returned from the master

   /**
    * Changes the pool type from DEFFERED -> CONNECTING or from CONNECTING -> PRECACHING
    *
    * @return true if pool type changed.
    */
   bool setState(EPoolState newState);

   /**
    * Changes the pool type from deferred in disconnect
    *
    * @return true if pool type changed.
    */
   bool setState(const SPartyID &serverID);
   static const char* toString(EPoolState poolState);
   const InterfaceDescription* getInterfaceDescription() const;
   const SPartyID* getServerID() const;
   EPoolState getState() const;

   /** Getter method */
   uint32_t getRefCount() const;

   /** Attach a notification to the pool */
   void attach(Notification &aNotification);

   /** Detaches a notification to from the pool, decreases reference counter and cleans-up when the pool has no notification.*/
   void detach(Notification &aNotification);

   /** Getter method */
   uint32_t getPoolID() const;

private:
   uint32_t poolID ;                      // id of this pool
   /**
    * Union like data structure with:
    *    - interface description
    *    - server ID
    */
   typedef DSI::TVariant<DSI_TYPELIST_2(InterfaceDescription, SPartyID)> TDataSwitch;
   TDataSwitch mDataSwitch;
   EPoolState mPoolState;
   /** A reference counter for all notifications in the pool */
   uint32_t mRefCount;

   /** The container where the object is stored.*/
   NotificationPool *mContainer;
};


class NotificationPool
{
public:
   friend class NotificationPoolEntry; //an entry will remove itself from the pool

   NotificationPool();
   ~NotificationPool();

   /*
    * adds a new entry to the list of notifiaction pools and returns
    * its position in the list
    *
    * @param isDeferredPool, the pool will contain notifications that are not yet decided if they are connect or
    * disconnect
    */
   size_t add( const InterfaceDescription& ifDescription, bool isDeferredPool = false );

   size_t add( const SPartyID& partyID );

   /*
    * returns the position of pool entry in the list of -1 if it
    * does not exist.
    * @param extendedSearch if true search also in deferred/preacaching pools
    */
   int32_t find( const InterfaceDescription& descr, bool extendedSearch = false ) const ;

   int32_t find( const SPartyID& partyID ) const ;

   /*
    * returns the position of pool entry in the list of -1 if it
    * does not exist.
    */
   int32_t find( uint32_t poolID ) const ;


   inline
   NotificationPoolEntry& operator[](int idx)
   {
      return mEntries[idx];
   }


   inline
   void clear()
   {
      mEntries.clear();
   }


   size_t size() const;
   unsigned int getTotalCounter() const;

private:

   std::vector<NotificationPoolEntry> mEntries;

   //statistical counter
   unsigned int mTotalCounter;

   /*
    * Removes the notifiaction pool with the given id from the
    * pool list
    */
   void removePool( uint32_t poolID ) ;
} ;


inline
unsigned int NotificationPool::getTotalCounter() const
{
   return mTotalCounter;
}


inline
size_t NotificationPool::size() const
{
   return mEntries.size();
}


inline
bool NotificationPoolEntry::setState(EPoolState newState)
{
   if (STATE_DEFERRED == mPoolState && STATE_CONNECTING == newState)
   {
      mPoolState = STATE_CONNECTING;
   }
   else if (STATE_CONNECTING == mPoolState && STATE_CONNECTED == newState)
   {
      mPoolState = STATE_CONNECTED;
   }
   else if (STATE_CONNECTED == mPoolState && STATE_PRECACHING == newState)
   {
      mPoolState = STATE_PRECACHING;
   }
   else
   {
      Log::error("Can not change pool type from %s to %s", toString(mPoolState), toString(newState));
      return false;
   }
   return true;
}


inline
bool NotificationPoolEntry::setState(const SPartyID &serverID)
{
   if (mPoolState != STATE_DEFERRED && mPoolState != STATE_PRECACHING)
   {
      Log::error("Can not change pool type from %s to DISCONNECT_POOL", toString(mPoolState));
      return false;
   }
   mPoolState = STATE_MONITOR_DISCONNECT;
   mDataSwitch = serverID;
   return true;
}


inline
const InterfaceDescription* NotificationPoolEntry::getInterfaceDescription() const
{
   return mDataSwitch.get<InterfaceDescription>();
}


inline const SPartyID* NotificationPoolEntry::getServerID() const
{
   return mDataSwitch.get<SPartyID>();
}


inline NotificationPoolEntry::EPoolState NotificationPoolEntry::getState() const
{
   return mPoolState;
}


inline uint32_t NotificationPoolEntry::getRefCount() const
{
   return mRefCount;
}


inline void NotificationPoolEntry::attach(Notification &aNotification)
{
   aNotification.poolID = poolID;
   mRefCount++;
}


uint32_t inline NotificationPoolEntry::getPoolID() const
{
   return poolID;
}


#endif // DSI_SERVICEBROKER_NOTIFICATIONPOOL_HPP

