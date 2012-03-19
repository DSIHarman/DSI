/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_SOCKETMESSAGECONTEXT_HPP
#define DSI_SERVICEBROKER_SOCKETMESSAGECONTEXT_HPP


#include <cstdlib>

#include "MessageContext.hpp"
#include "SocketConnectionContext.hpp"
#include "AutoBuffer.hpp"


// forward decl
class ServicebrokerResourceManager;
class Job;


/**
 * The context object is used to read ancillary data from and write a response back to the client.
 * It has an internal state machine since the program flow is defined to first read or write data and
 * then call a prepare function for the response. A copy could be made for deferred responses. In this
 * case, the copy is not functional any more but only stores request specific data enough for just
 * sending a response to the calling client.
 */
class SocketMessageContext : public MessageContext
{            
   friend class Job;
   
   /// create an invalid default object
   SocketMessageContext();
   
   inline
   operator const void*() const
   {
      return mCtx;
   }
   
public:       
   
   SocketMessageContext(SocketConnectionContext& ctxt, tAutoBufferType& readBuffer, tAutoBufferType& writeBuffer);
   
   /** 
    * Construct a copy of this message context. The copied context is in DEFERRED state so you may not
    * call any prepareXXX methods afterwards. This is only good for finalizing any pending jobs.
    */
   SocketMessageContext(const SocketMessageContext& ctx);
   
   /** 
    * The retval is a FNDxxx value from servicebrokers error codes. No payload here or payload
    * was already written with writeAncillaryData and no statically defined output struct available.
    */
   void prepareResponse(int retval, bool force = false);
   
   /** 
    * The retval is a FNDxxx value from servicebrokers error codes. data and len describe 
    * the payload (the .o part of the argument union).
    */
   void prepareResponse(int retval, const void* data, size_t len);   
      
   /**
    * Internal. Not to be used within handlers. Called by the event dispatcher.
    */
   int readData(void* buf, size_t len);
   
   /**
    * Read data from the input buffer. The read begins after a well-defined input structure.
    * This function can only be used before either @c prepareResponse or @c writeAncillaryData is called.
    */
   int readAncillaryData(void* buf, size_t len);
   
   /**
    * Write data to the output buffer. Write begins after a well-defined output structure, in which
    * case the output structure must be written back via the appropriate call to @c prepareResponse.
    * This function can only be used before @c prepareResponse and after @c readAncillaryData is called.
    */
   void writeAncillaryData(const void* buf, size_t len);

   inline
   const SocketConnectionContext& context() const
   {
      return *mCtx;
   }
   
   /**
    * Finalize the job.
    */
   void finalize(Job& job);
   
   /// move semantics
   SocketMessageContext& operator=(const SocketMessageContext& ctx);   
   
private:   
   
   SocketConnectionContext* mCtx;
   
   tAutoBufferType* mReadBuffer;
   tAutoBufferType* mWriteBuffer;
};


#endif  // DSI_SERVICEBROKER_SOCKETMESSAGECONTEXT_HPP
