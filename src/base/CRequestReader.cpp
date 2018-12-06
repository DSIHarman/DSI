/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "CRequestReader.hpp"

#include <errno.h>

#include "dsi/CChannel.hpp"
#include "dsi/Streaming.hpp"
#include "dsi/Log.hpp"

#include "CTraceManager.hpp"
#include "DSI.hpp"


TRC_SCOPE_DEF(dsi_base, CRequestReader, receiveAll);


namespace /*anonymous*/
{

class CErrnoSafe
{
public:

   inline
   CErrnoSafe()
    : mError(errno)
   {
      // NOOP
   }

   inline
   ~CErrnoSafe()
   {
      errno = mError;
   }

   inline
   int error() const
   {
      return mError;
   }

private:
   int mError;
};

}   // end namespace


// ------------------------------------------------------------------------------------------------


bool DSI::CRequestReader::receiveAll()
{
   TRC_SCOPE(dsi_base, CRequestReader, receiveAll);

   bool rc = true;
   errno = EINVAL;
      
   if (mCurrent->packetLength > 0)
   {
      if (mBuf.setCapacity(mBuf.size() + mCurrent->packetLength))
      {
         SFNDInterfaceDescription iface;         
         uint32_t requestId = 0;

         if (!mChnl.recvAll(mBuf.pptr(), mCurrent->packetLength))
         {
            CErrnoSafe e;
            DBG_ERROR(( "CRequestReader: receiving payload failed: errno=%d", e.error() ));
            rc = false;
         }
         else
         {            
            if (CTraceManager::resolve(mCurrent->serverID, mCurrent->clientID, iface))            
            {
               requestId = reinterpret_cast<const EventInfo*>(mBuf.gptr())->requestID;
               
               CInputTraceSession session(iface, requestId);
               if (session.isActive()) 
               {         
                  session.write(mCurrent, (const EventInfo*)mBuf.gptr(), mBuf.gptr() + sizeof(EventInfo), mCurrent->packetLength - sizeof(EventInfo));
               }
               else
                  requestId = 0;
            }

            mBuf.pbump(mCurrent->packetLength);
         }

         while((mCurrent->flags & DSI_MORE_DATA_FLAG) && rc)
         {
            if (mChnl.recvAll(&mHdr, sizeof(mHdr)))
            {
               if (mHdr.packetLength > 0)
               {
                  if (!mBuf.setCapacity(mBuf.size() + mHdr.packetLength))
                  {
                     DBG_ERROR(("CRequestReader: out of Memory (capacity :%d)", mBuf.size() + mHdr.packetLength));
                     errno = ENOMEM;
                     rc = false;
                  }
                  else
                  {
                     mCurrent = &mHdr;

                     if (!mChnl.recvAll(mBuf.pptr(), mHdr.packetLength))
                     {
                        CErrnoSafe e;
                        DBG_ERROR(("CRequestReader: receiving payload failed: errno=%d", e.error()));
                        rc = false;
                     }
                     else
                     {             
                        if (requestId != 0)            
                        {
                           CInputTraceSession session(iface, requestId);
                           if (session.isPayloadEnabled()) 
                           {                                       
                              session.write(mCurrent, nullptr, mBuf.pptr(), mHdr.packetLength);
                           }
                        }
                        
                        mBuf.pbump(mHdr.packetLength);
                     }
                  }
               }
            }
            else
            {
               CErrnoSafe e;
               DBG_ERROR(( "CRequestReader: receiving header (more data) failed: errno=%d", e.error()));
               rc = false;
            }
         }
      }
      else
      {
         DBG_ERROR(("CRequestReader: out of Memory (capacity :%d)", mBuf.size() + mHdr.packetLength));
         errno = ENOMEM;
         rc = false;
      }
   }

   return rc;
}
