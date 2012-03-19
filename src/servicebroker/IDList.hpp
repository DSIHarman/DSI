/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_IDLIST_HPP
#define DSI_SERVICEBROKER_IDLIST_HPP

#include "dsi/private/platform.h"

#include <vector>
#include <set>


typedef std::vector<notificationid_t> NotificationIDList ;


inline 
bool operator< (const SPartyID& lhs, const SPartyID&rhs)
{
   return lhs.globalID < rhs.globalID ;
}


class PartyIDList
{
public:

   typedef std::set<SPartyID> tContainerType;
   typedef tContainerType::iterator iterator;
   typedef tContainerType::const_iterator const_iterator;
   
   inline
   PartyIDList() 
    : mList()
   {
      // NOOP
   }
   
   inline
   const_iterator find(const SPartyID& needle) const
   {      
      return mList.find(needle);    
   }
   
   
   inline
   bool exists(const SPartyID& needle) const
   {
      return mList.find(needle) != mList.end();
   }
   
   inline
   size_t size() const
   {
      return mList.size();
   }
   
   inline
   void insert(const SPartyID &element)
   {
      mList.insert(element);
   }
   
   inline
   void remove(const SPartyID &elem)
   {
      mList.erase(elem);
   }
   
   inline
   const_iterator begin() const
   {
      return mList.begin();
   }
   
   inline
   const_iterator end() const
   {
      return mList.end();
   }   
   
   inline
   void erase(const_iterator iter)
   {      
      mList.erase(iter);
   }   
   
   
private:

   tContainerType mList;
};


#endif /* DSI_SERVICEBROKER_IDLIST_HPP */

