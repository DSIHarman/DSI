/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_COMMON_LOCKGUARD_HPP
#define DSI_COMMON_LOCKGUARD_HPP


#include "dsi/private/CNonCopyable.hpp"

#include "RecursiveMutex.hpp"


namespace DSI
{
   template<typename LockableT = RecursiveMutex>
   class LockGuard : public Private::CNonCopyable
   {
   public:

      inline
      LockGuard(LockableT& lock)
         : mLock(lock)
      {
         mLock.lock();
      }


      inline
      ~LockGuard()
      {
         mLock.unlock();
      }


   private:

      LockableT& mLock;
   };
}//namespace DSI

#endif   // DSI_COMMON_LOCKGUARD_HPP
