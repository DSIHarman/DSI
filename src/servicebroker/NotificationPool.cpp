/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "NotificationPool.hpp"
#include "Notification.hpp"
#include "Servicebroker.hpp"


namespace /*anonymous*/
{
  /** holds the next free pool id */
  uint32_t NextPoolID = 100000 ;
}


NotificationPoolEntry::NotificationPoolEntry(NotificationPool *aContainer, const InterfaceDescription& ifDescr, bool isDeferredPool)
  : notificationID(Notification::INVALID_NOTIFICATION_ID)
  , poolID(NextPoolID++)
  , mDataSwitch(ifDescr)
  , mPoolState((isDeferredPool) ? STATE_DEFERRED : STATE_CONNECTING)
  , mRefCount(0)
  , mContainer(aContainer)
{
  // NOOP
}


NotificationPoolEntry::NotificationPoolEntry(NotificationPool *aContainer, const SPartyID& serverID)
  : notificationID(Notification::INVALID_NOTIFICATION_ID)
  , poolID(NextPoolID++)
  , mDataSwitch(serverID)
  , mPoolState(STATE_MONITOR_DISCONNECT)
  , mRefCount(0)
  , mContainer(aContainer)
{
  // NOOP
}


void NotificationPoolEntry::detach(Notification &aNotification)
{
  const notificationid_t tmp(notificationID);

  aNotification.poolID = 0;
  if (mRefCount > 0)
  {
    mRefCount--;
  }
  if (0 == mRefCount)
  {
    //no reference to the pool so it will be removed
    Log::message(4, "Removing pool %u", poolID);

    if (mContainer)
    {
      mContainer->removePool(poolID);
    }

    if (tmp != Notification::INVALID_NOTIFICATION_ID)
    {
      Servicebroker::getInstance().createClearNotificationJob(tmp);
    }
  }
}


const char* NotificationPoolEntry::toString(EPoolState poolState)
{
  switch (poolState)
  {
  case NotificationPoolEntry::STATE_UNDEFINED:
    return "STATE_UNDEFINED";

  case NotificationPoolEntry::STATE_CONNECTING:
    return "STATE_CONNECTING";

  case NotificationPoolEntry::STATE_MONITOR_DISCONNECT:
    return "STATE_MONITOR_DISCONNECT";

  case NotificationPoolEntry::STATE_CONNECTED:
    return "STATE_CONNECTED";

  case NotificationPoolEntry::STATE_PRECACHING:
    return "STATE_PRECACHING";

  default:   
    return "STATE_???";
  }
}


NotificationPool::NotificationPool()
 : mTotalCounter(0u)
{
  // NOOP
}


NotificationPool::~NotificationPool()
{
  // NOOP
}


size_t NotificationPool::add( const InterfaceDescription& ifDescription, bool isDeferredPool )
{
  NotificationPoolEntry np(this, ifDescription, isDeferredPool) ;   
  mEntries.push_back( np );
  mTotalCounter++;
  return mEntries.size() - 1 ;
}


size_t NotificationPool::add( const SPartyID& partyID )
{
  NotificationPoolEntry np(this, partyID) ;
  mEntries.push_back( np );
  mTotalCounter++;
  return mEntries.size() - 1 ;
}


int32_t NotificationPool::find( const InterfaceDescription& ifDescription, bool extendedSearch ) const
{
  for( int32_t idx = mEntries.size() - 1; idx >= 0; idx-- )
  {
    const NotificationPoolEntry &np = mEntries[idx];
    if (((extendedSearch && np.getState() != NotificationPoolEntry::STATE_MONITOR_DISCONNECT)
        || np.getState() == NotificationPoolEntry::STATE_CONNECTING)
       && *(np.getInterfaceDescription()) == ifDescription)
    {
      return idx ;
    }
  }
  return -1 ;
}


int32_t NotificationPool::find( const SPartyID& partyID ) const
{
  for (int32_t idx = mEntries.size() - 1; idx >= 0; idx--)
  {
    const NotificationPoolEntry &np = mEntries[idx];
    if (NotificationPoolEntry::STATE_MONITOR_DISCONNECT == np.getState()
       && *(np.getServerID()) == partyID)
    {
      return idx ;
    }
  }

  return -1 ;
}


int32_t NotificationPool::find( uint32_t poolID ) const
{
  for( int32_t idx = mEntries.size() - 1; idx >= 0; idx-- )
  {
    if( mEntries[idx].getPoolID() == poolID )
      return idx ;
  }

  return -1 ;
}


void NotificationPool::removePool( uint32_t poolID )
{
  int32_t pos = find( poolID );

  if (-1 != pos)
    mEntries.erase( mEntries.begin() + pos );   
}
