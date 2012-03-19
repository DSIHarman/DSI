/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "MessageContext.hpp"

#include "JobQueue.hpp"


MessageContext::MessageContext()
 : mState(State_INITIAL) 
 , mGetOffset(-1)
 , mPutOffset(0)
{
   // NOOP
}


MessageContext::MessageContext(const MessageContext& rhs)
 : mState(State_DEFERRED)    // after copying, no read/write functions available any more 
 , mGetOffset(rhs.mGetOffset)
 , mPutOffset(rhs.mPutOffset)
{
   // NOOP
}


MessageContext& MessageContext::operator=(const MessageContext& rhs)
{
   if (&rhs != this)
   {
      mState = State_DEFERRED;
      mGetOffset = rhs.mGetOffset;
      mPutOffset = rhs.mPutOffset;
   }
   
   return *this;
}
