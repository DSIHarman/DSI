/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "config.h"
#include "SocketMessageContext.hpp"

#include "JobQueue.hpp"
#include "ServicebrokerServer.hpp"


SocketMessageContext::SocketMessageContext()
 : MessageContext()
 , mCtx(nullptr)
 , mReadBuffer(nullptr)
 , mWriteBuffer(nullptr)
{
   // NOOP
}


SocketMessageContext::SocketMessageContext(SocketConnectionContext& ctxt, tAutoBufferType& readBuffer, tAutoBufferType& writeBuffer)
 : mCtx(&ctxt) 
 , mReadBuffer(&readBuffer)
 , mWriteBuffer(&writeBuffer)
{
   // NOOP
}


SocketMessageContext::SocketMessageContext(const SocketMessageContext& rhs)
 : MessageContext(rhs)
 , mCtx(rhs.mCtx)  
 , mReadBuffer(nullptr)
 , mWriteBuffer(nullptr)
{
   // NOOP
}


SocketMessageContext& SocketMessageContext::operator=(const SocketMessageContext& rhs)
{
   if (&rhs != this)
   {
      MessageContext::operator=(rhs);
      mCtx = rhs.mCtx;
      mReadBuffer = nullptr;
      mWriteBuffer = nullptr;
   }
   
   return *this;   
}   

void SocketMessageContext::prepareResponse(int retval, bool force)
{
   assert(mWriteBuffer);

   if (!force)   
      assert(mState == State_PROCESSING);
   
   mState = State_SENT;

   if (mPutOffset > 0 && retval <= static_cast<int>(FNDOK))    // negative value for GetInterfaceList and MatchInterfaceList
   {      
      mWriteBuffer->response().fr_envelope.fr_size = mPutOffset;
   }   
   else
      mWriteBuffer->response().fr_envelope.fr_size = 0;      
      
   mWriteBuffer->response().fr_returncode = retval;
}


void SocketMessageContext::prepareResponse(int retval, const void* data, size_t len)
{
   assert(mState == State_PROCESSING);
   assert(mWriteBuffer);
   
   mState = State_SENT;

   if (mPutOffset > 0 && retval == static_cast<int>(FNDOK))  
   {  
      mWriteBuffer->response().fr_envelope.fr_size = mPutOffset;      
   }
   else   
      mWriteBuffer->response().fr_envelope.fr_size = len;         
   
   mWriteBuffer->response().fr_returncode = retval;      
   (void)mWriteBuffer->write(data, len, sizeof(sb_response_frame_t));         
}


int SocketMessageContext::readData(void* buf, size_t len)
{   
   assert(buf && len);
   assert(mState == State_INITIAL);
   assert(mReadBuffer);
   
   // this is called by the servicebroker main dispatcher
   mState = State_PROCESSING;
   
   return mReadBuffer->read(buf, len, sizeof(sb_request_frame_t));
}


int SocketMessageContext::readAncillaryData(void* buf, size_t len)
{
   assert(mState == State_PROCESSING);
   assert(mGetOffset >= 0);
   assert(mReadBuffer);
      
   int rc = mReadBuffer->read(buf, len, mGetOffset + sizeof(sb_request_frame_t));
   if (rc > 0)
      mGetOffset += len;
      
   return rc;
}


void SocketMessageContext::writeAncillaryData(const void* buf, size_t len)
{
   assert(mState == State_PROCESSING);
   assert(mPutOffset >= 0); 
   assert(mWriteBuffer);
   
   (void)mWriteBuffer->write(buf, len, mPutOffset + sizeof(sb_response_frame_t));   

   mPutOffset += len;
}


void SocketMessageContext::finalize(Job& job)
{
   assert(mState == State_DEFERRED);

   mCtx->mClient.sendDeferredResponse(job.status, job.ret_val, job.arg, job.rbytes);
}
