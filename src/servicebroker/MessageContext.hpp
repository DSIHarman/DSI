/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_MESSAGECONTEXT_HPP
#define DSI_SERVICEBROKER_MESSAGECONTEXT_HPP


#include <cstdlib>

//#include "dsi/servicebroker.h"

#include "config.h"


// forward decl
class ConnectionContext;
class Job;


/**
 * The context object is used to read ancillary data from and write a response back to the client.
 * It has an internal state machine since the program flow is defined to first read or write data and
 * then call a prepare function for the response. A copy could be made for deferred responses. In this
 * case, the copy is not functional any more but only stores request specific data enough for just
 * sending a response to the calling client.
 */
class MessageContext
{
public: 

   enum State 
   { 
      State_INITIAL = 0,    ///< initial state of a message before entering processing
      State_PROCESSING,     ///< servicebroker processing has started, event handler can safely be called; 
                            ///< reading and writing the buffers is allowed      
      State_SENT,           ///< any prepareResponse function was called
      State_DEFERRED        ///< The message will not be sent now, deferResponse was called
   };   

   MessageContext();
   
   /** 
    * Construct a copy of this message context. The copied context is in DEFERRED state so you may not
    * call any prepareXXX methods afterwards. This is only good for finalizing any pending jobs.
    */
   MessageContext(const MessageContext& ctx);
      
   /** 
    * resource managers: set return status _RESMGR_NOREPLY
    */
   inline
   void deferResponse()
   {
      mState = State_DEFERRED;
   }
         
   inline
   State responseState() const
   {
      return mState;
   }
   
   /// @internal not to be called from within handlers
   inline
   void setGetOffset(size_t offset)
   {
      mGetOffset = offset;
   }   
   
   /// move semantics
   MessageContext& operator=(const MessageContext& ctx);   
         
protected:
      
   State mState;
   
   /**
    * Current 'buffer' positions.
    * What's this: a request always contains a fixed size request frame, the fixed size may also be 0. 
    * A request or response may always contain ancillary data which are not part of the structured interface
    * description as defined in servicebroker.h. An example is the RegisterEx function for ancillary input data
    * and the GetInterfaceList function for ancillary output data (the given .o frame is not the complete output).
    * This input or output may be read or written with the ancillary functions. Within the servicebroker's dispatch
    * function you must define at which position the ancillary data begins.
    */
   ssize_t mGetOffset;
   ssize_t mPutOffset;
};


#endif  // DSI_SERVICEBROKER_MESSAGECONTEXT_HPP
