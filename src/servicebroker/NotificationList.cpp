/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include <algorithm>

#include "NotificationList.hpp"
#include "IDList.hpp"
#include "Log.hpp"

#define QNX_REPLACE_RESMGR_CALLS
#include "dsi/clientlib.h"

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>


NotificationList::NotificationList()
 : mTotalCounter(0u)
{
  // NOOP
}


NotificationList::~NotificationList()
{
  // NOOP
}


void NotificationList::trigger( const SPartyID &id )
{
  CNotificationList::iterator iter = mList.begin();
  while( iter != mList.end() )
  {
    if( iter->active && iter->partyID.globalID == id.globalID )
    {
      /* send notification pulse */
      iter->send();

      iter = mList.erase( iter );
    }
    else
    {
      ++iter ;
    }
  }
}


void NotificationList::trigger( const InterfaceDescription &ifDescription, gid_t grpid )
{
  CNotificationList::iterator iter = mList.begin();
  while( iter != mList.end() )
  {
    if( iter->ifDescription.name == ifDescription.name )
    {
      if( iter->active
       && iter->ifDescription.majorVersion == ifDescription.majorVersion
       && iter->ifDescription.minorVersion <= ifDescription.minorVersion )
      {
        bool doSend = true ;

        if( (gid_t)SB_UNKNOWN_GROUP_ID != grpid )
        {
          Log::message( 3, " trigger notification grpid:%d, uid:%d", grpid, iter->uid );

          doSend = (0 == iter->uid) ;
          if( !doSend )
          {
            group* gr = getgrgid( grpid );
            if( gr )
            {
              for( char** p = gr->gr_mem; *p; p++ )
              {
                passwd* pw = getpwnam( *p );
                if( pw && static_cast<uid_t>(pw->pw_uid) == iter->uid )
                {
                  doSend = true ;
                  break;
                }
              }
            }
          }
        }

        if( doSend )
        {
          /* send notification pulse */
          iter->send();

          iter = mList.erase( iter );
          continue ;
        }
        else
        {
          Log::warning( 1, " Notification on interface %s retained because of access restrictions", ifDescription.name.c_str() );
          iter->active = false ;
        }
      }
      else
      {
        Log::message( 0, "Interface %s %d.%d available, notification set on version %d.%d", ifDescription.name.c_str()
          , iter->ifDescription.majorVersion, iter->ifDescription.minorVersion
          , ifDescription.majorVersion, ifDescription.minorVersion );
      }
    }
    ++iter ;
  }
}


void NotificationList::trigger( notificationid_t notificationID, bool removeIt )
{
  for( CNotificationList::iterator iter = mList.begin(); iter != mList.end(); ++iter )
  {
    if( iter->active && iter->notificationID == notificationID )
    {
      /* send notification pulse */
      iter->send();

      if( removeIt )
      {
        (void)mList.erase( iter );
      }
      break;
    }
  }
}


void NotificationList::triggerPool( uint32_t poolID )
{
  CNotificationList::iterator iter = mList.begin();
  while( iter != mList.end() )
  {
    if( iter->active && iter->poolID == poolID )
    {
      /* send notification pulse */
      iter->send();
      iter = mList.erase( iter );
    }
    else
    {
      ++iter ;
    }
  }
}


void NotificationList::trigger( const PartyIDList &idList )
{
  CNotificationList::iterator iter = mList.begin();
  while( iter != mList.end() )
  {
    if( iter->active && idList.exists( iter->partyID ))
    {
      /* send notification pulse */
      iter->send();

      iter = mList.erase( iter );
    }
    else
    {
      ++iter ;
    }
  }
}


void NotificationList::triggerAll()
{
  CNotificationList::iterator iter = mList.begin();
  while( iter != mList.end() )
  {
    if( iter->active && !iter->hasRegExp() )
    {
      /* send notification pulse */
      iter->send();
    }
    ++iter ;
  }
}


void NotificationList::triggerAll( const InterfaceDescription &ifDescription )
{
  CNotificationList::iterator iter = mList.begin();
  while( iter != mList.end() )
  {
    if( iter->active && (!iter->hasRegExp() || iter->matchRegExp( ifDescription.name )))
    {
      /* send notification pulse */
      iter->send();
    }
    ++iter ;
  }
}


void NotificationList::triggerNotLocal( uint32_t extendedID )
{
  CNotificationList::iterator iter = mList.begin();
  while( iter != mList.end() )
  {
    if( iter->active && (extendedID != iter->partyID.s.extendedID))
    {
      /* send notification pulse */
      iter->send();

      iter = mList.erase( iter );
    }
    else
    {
      ++iter ;
    }
  }
}


void NotificationList::triggerLocal(uint32_t extendedID)
{
  CNotificationList::iterator iter = mList.begin();
  while( iter != mList.end() )
  {
    if( iter->active && (extendedID == iter->partyID.s.extendedID))
    {
      /* send notification pulse */
      iter->send();

      iter = mList.erase( iter );
    }
    else
    {
      ++iter ;
    }
  }
}


notificationid_t NotificationList::add( const SocketConnectionContext& ctx, SFNDPulseInfo &pulse )
{
  Notification entry ;

  if( entry.connect( ctx, pulse ) )
  {
    mList.push_front( entry );
    mTotalCounter++;

    return entry.notificationID ;
  }
  return Notification::INVALID_NOTIFICATION_ID ;
}


notificationid_t NotificationList::add( const char* regExpr, const SocketConnectionContext& ctx, SFNDPulseInfo &pulse )
{
  Notification entry ;

  if( entry.setRegExp( regExpr ) && entry.connect( ctx, pulse ) )
  {
    mList.push_front( entry );
    mTotalCounter++;

    return entry.notificationID ;
  }

  return Notification::INVALID_NOTIFICATION_ID ;
}


notificationid_t NotificationList::add( ClientSpecificData *host, const SPartyID &clientID, const SocketConnectionContext& ctx, SFNDPulseInfo &pulse )
{
  Notification entry ;

  if( entry.connect( ctx, pulse ) )
  {
    /* initialize ID */
    entry.partyID = clientID ;

    /* assign host */
    entry.host = host ;

    mList.push_front( entry );
    mTotalCounter++;

    return entry.notificationID ;
  }
  return Notification::INVALID_NOTIFICATION_ID ;
}


notificationid_t NotificationList::add( ClientSpecificData *host, const InterfaceDescription &ifDescription, const SocketConnectionContext& ctx, SFNDPulseInfo &pulse, bool local, uid_t uid )
{
  Notification entry ;

  if( entry.connect( ctx, pulse ) )
  {
    entry.ifDescription = ifDescription ;
    entry.host = host ;
    entry.local = local ;
    entry.uid = uid ;
    mList.push_front( entry );
    mTotalCounter++;

    return entry.notificationID ;
  }

  return Notification::INVALID_NOTIFICATION_ID ;
}


void NotificationList::add( Notification& aNotification )
{
  mList.push_front( aNotification );
  mList.front().poolID = aNotification.poolID;
  mTotalCounter++;
}


Notification* NotificationList::find( notificationid_t notificationID )
{
  CNotificationList::iterator iter = mList.begin();
  while( iter != mList.end() )
  {
    if( iter->notificationID == notificationID )
    {
      return &(*iter) ;
    }
    ++iter ;
  }
  return nullptr ;
}


void NotificationList::remove( notificationid_t notificationID )
{
  CNotificationList::iterator iter = mList.begin();
  while( iter != mList.end() )
  {
    if( iter->notificationID == notificationID )
    {
      (void)mList.erase( iter );
      break;
    }

    ++iter ;
  }
}


void NotificationList::remove( const NotificationIDList &idList )
{
  CNotificationList::iterator iter = mList.begin();
  while( iter != mList.end() )
  {     
    if (std::find(idList.begin(), idList.end(), iter->notificationID) != idList.end())
    {
      iter = mList.erase( iter );
    }
    else      
      ++iter ;      
  }
}


void NotificationList::dumpStats( std::ostringstream& ostream ) const
{
  ostream << "     pid  Process                  Notification\n" ;
  for( CNotificationList::const_iterator iter = mList.begin(); iter != mList.end(); ++iter )
  {
    iter->dumpStats( ostream );
  }
}


int NotificationList::clearMasterNotification()
{
  for( CNotificationList::iterator iter = mList.begin(); iter != mList.end(); ++iter )
  {
    iter->masterNotificationID = Notification::INVALID_NOTIFICATION_ID;
    iter->poolID = 0;
  }

  return 0 ;
}


bool NotificationList::move(NotificationList &source, uint32_t poolID)
{
  bool result(false);

  CNotificationList::iterator iter = source.mList.begin();
  while (iter != source.mList.end())
  {
    if (poolID == iter->poolID)
    {
      add(*iter);
      iter = source.mList.erase(iter);
      result = true;
      source.mTotalCounter--;
    }
    else      
      ++iter;      
  }

  return result;
}


size_t NotificationList::setMasterNotificationID(notificationid_t notificationID, uint32_t poolID)
{
  size_t changes(0);

  for (CNotificationList::iterator iter = mList.begin(); iter != mList.end(); ++iter)
  {
    if (poolID == iter->poolID)
    {
      changes++;
      iter->masterNotificationID = notificationID;
    }
  }

  return changes;
}


size_t NotificationList::setType(uint32_t poolID, const SPartyID &partyID)
{
  size_t changes(0);

  for (CNotificationList::iterator iter = mList.begin(); iter != mList.end(); ++iter)
  {
    if (poolID == iter->poolID)
    {
      changes++;
      iter->setType(partyID);
    }
  }

  return changes;
}


size_t NotificationList::poolSize(uint32_t poolID)
{
  size_t lCount(0);

  for (CNotificationList::iterator iter = mList.begin(); iter != mList.end(); ++iter)
  {
    if (poolID == iter->poolID)
    {
      lCount++;
    }
  }

  return lCount;
}


size_t NotificationList::count(const SPartyID &id)
{
  size_t lCount(0);

  for (CNotificationList::iterator iter = mList.begin(); iter != mList.end(); ++iter)
  {
    if (id == iter->partyID)
    {
      lCount++;
    }
  }

  return lCount;
}
