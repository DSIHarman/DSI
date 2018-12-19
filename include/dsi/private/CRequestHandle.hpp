/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_PRIVATE_CREQUESTHANDLE_HPP
#define DSI_PRIVATE_CREQUESTHANDLE_HPP


#include "dsi/DSI.hpp"
#include "dsi/CChannel.hpp"

namespace DSI
{

   namespace Private
   {
   
      /// FIXME remove template here
      /// FIXME make shared_ptr to reference here, it's ok...
      /// request handle base class on receiver side - could be either client (responses) or server (requests)
      template<typename ChannelT>
      class CRequestHandle
      {
      public:

         inline
         CRequestHandle(const DSI::MessageHeader& hdr, std::tr1::shared_ptr<ChannelT> chnl)            
            : mChnl(chnl)
            , mHdr(hdr)
         {
            // NOOP
         }

         inline
         SPartyID getServerID() const
         {
            return mHdr.serverID;
         }

         inline
         SPartyID getClientID() const
         {
            return mHdr.clientID;
         }

         inline
         std::tr1::shared_ptr<ChannelT> getChannel()
         {
            return mChnl;
         }

         inline
         uint16_t getProtoMinor() const
         {
            return mHdr.protoMinor;
         }
      protected:

         inline
         ~CRequestHandle()
         {
            // NOOP
         }

         std::tr1::shared_ptr<ChannelT> mChnl;
         
      private:

         const DSI::MessageHeader& mHdr;         
      };


      /// ordinary DSI requests base class
      class CDataHandle : public CRequestHandle<CChannel>
      {
      public:

         inline
         CDataHandle(const DSI::MessageHeader& hdr, std::tr1::shared_ptr<CChannel> chnl, const char* payload, size_t len)
            : CRequestHandle<CChannel>(hdr, chnl)
            , mInfo((DSI::EventInfo*)payload)
            , mPayload(payload + sizeof(DSI::EventInfo))
            , mPayloadLength(len - sizeof(DSI::EventInfo))
         {
            // NOOP
         }

         inline
         DSI::RequestType getRequestType() const
         {
            return mInfo->requestType;
         }

         inline
         uint32_t getRequestId() const
         {
            return mInfo->requestID;
         }

         inline
         int32_t getSequenceNumber() const
         {
            return mInfo->sequenceNumber;
         }

         /// payload pointer
         inline
         const char* payload() const
         {
            return mPayload;
         }

         /// payload length
         inline
         size_t size() const
         {
            return mPayloadLength;
         }

      protected:

         const DSI::EventInfo* mInfo;

         const char* mPayload;
         size_t mPayloadLength;
      };


      /// ordinary requests
      class CDataRequestHandle : public CDataHandle
      {
      public:

         inline
         CDataRequestHandle(const DSI::MessageHeader& hdr, std::tr1::shared_ptr<CChannel> chnl, const char* payload, size_t len)
            : CDataHandle(hdr, chnl, payload, len)
         {
            // NOOP
         }

         inline
         DSI::RequestType getRequestType() const
         {
            return mInfo->requestType;
         }
      };


      /// ordinary responses
      class CDataResponseHandle : public CDataHandle
      {
      public:

         inline
         CDataResponseHandle(const DSI::MessageHeader& hdr, std::tr1::shared_ptr<CChannel> chnl, const char* payload, size_t len)
            : CDataHandle(hdr, chnl, payload, len)
         {
            // NOOP
         }

         inline
         DSI::ResultType getResponseType() const   // FIXME names are not straight forward: "result" or "response"?
         {
            return mInfo->responseType;
         }

         inline
         DSI::DataStateType getDataState() const
         {
            return (mInfo->responseType == DSI::RESULT_DATA_OK) ? DSI::DATA_OK : DSI::DATA_INVALID ;
         }
      };
      
   }   // namespace Private
   
}   //namespace DSI


#endif   // DSI_PRIVATE_REQUESTHANDLE_HPP
