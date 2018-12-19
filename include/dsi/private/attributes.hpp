/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_PRIVATE_ATTRIBUTES_HPP
#define DSI_PRIVATE_ATTRIBUTES_HPP

#include <stdint.h>
#include <algorithm>
#include <vector>
#include <cassert>
#include <tr1/type_traits>

#include "dsi/CIStream.hpp"
#include "dsi/COStream.hpp"

#include "if.hpp"


namespace DSI
{
   namespace Private
   {
         
      /**
       * @internal
       *
       * Read an partial update request from the stream and store the result in the appropriate function arguments.
       */
      template<typename InputStreamT, typename T>   
      void readPartialAttribute(InputStreamT& istream, std::vector<T>& t, UpdateType* ptype, int16_t* pposition, int16_t* pcount)
      {
         std::vector<T> vec;
         UpdateType type;
         int16_t position;
         int16_t count;

         istream >> type;
         istream >> position;
         istream >> count;         
         istream >> vec;   
         
         switch(type)
         {
         case UPDATE_COMPLETE:
            t.swap(vec);
            break;

         case UPDATE_INSERT:
            t.insert(t.begin()+position, vec.begin(), vec.end());
            break;

         case UPDATE_REPLACE:
            std::copy(vec.begin(), vec.end(), t.begin()+position);         
            break;

         case UPDATE_DELETE:         
            t.erase(t.begin()+position, t.begin()+position+count);
            break;
         
         default:
            // NOOP
            break;
         }

         if(ptype)
            *ptype = type;
            
         if(pposition)
            *pposition = position;
            
         if(pcount)
            *pcount = count;
      }
      
      
      /**
       * @internal
       *
       * Write a partial update request to the stream.
       */
      template<typename OutputStreamT, typename T>   
      void writePartialAttribute(OutputStreamT& ostream, const std::vector<T>& t, UpdateType type, int16_t position, int16_t count)
      {
         if (position < 0)
            position = 0;

         if (count < 0)
            count = t.size() - position;
            
         ostream << type << position << count;   

         int32_t size = (type == DSI::UPDATE_DELETE) ? 0 : count;
         ostream << size;
         if (0 < size)
         {
            typename std::vector<T>::const_pointer pElem = &t[position];
            while(size-- > 0)
            {               
               ostream << *pElem++;    
            }
         }
      }
    
    
      // --------------------------------------------------------------------------------------------
      
    
      template<typename DataT>
      struct AttributeBase
      {                            
         inline
         AttributeBase()
          : mValue()
          , mState(DATA_NOT_AVAILABLE)
         {
            // NOOP
         }
                  
         inline
         bool isValid() const
         {
            return mState == DATA_OK;
         }              
         
         DataT mValue;
         DataStateType mState;
      };
   
      
   
      // ------------------------------------------------------------------------------------------
      
   
      template<typename DataT>
      struct ServerAttributeBase : public AttributeBase<DataT>
      {            
         typedef typename if_<std::tr1::is_arithmetic<DataT>::value, DataT, const DataT&>::type argument_type;         
               
         inline
         void operator=(argument_type data)
         {
            this->mValue = data;
            this->mState = DATA_OK;            
         }
         
         inline
         bool operator==(argument_type data) const
         {
            return this->mValue == data;            
         }
         
         inline
         void invalidate()
         {
            this->mState = DATA_INVALID;
         }     

         inline
         void setState(DataStateType state)
         {
            this->mState = state;
         }
      };
      
      
      // ------------------------------------------------------------------------------------------
      
      
      template<typename DataT>
      struct ServerAttribute : public ServerAttributeBase<DataT>
      {             
         using ServerAttributeBase<DataT>::operator=;   
         using ServerAttributeBase<DataT>::operator==; 
         using ServerAttributeBase<DataT>::invalidate;
         using ServerAttributeBase<DataT>::setState;
      };
      
      
      /// partial update support
      template<typename T>
      struct ServerAttribute<std::vector<T> > : ServerAttributeBase<std::vector<T> >
      {      
         using ServerAttributeBase<std::vector<T> >::operator=;   
         using ServerAttributeBase<std::vector<T> >::operator==; 
         using ServerAttributeBase<std::vector<T> >::invalidate;
         using ServerAttributeBase<std::vector<T> >::setState;
                  
         void set(const std::vector<T>& from, UpdateType type = UPDATE_COMPLETE, int16_t position = -1, int16_t count = -1)
         {
            assert(position && count);
            
            if (position < 0)
               position = 0;
            
            switch(type)
            {
            case UPDATE_COMPLETE:
               position = 0;
               count = from.size();
               this->mValue = from;
               break;

            case UPDATE_INSERT:
               count = from.size();
               this->mValue.insert(this->mValue.begin() + position, from.begin(), from.end());
               break;

            case UPDATE_REPLACE:
               count = from.size();
               std::copy(from.begin(), from.end(), this->mValue.begin()+position);
               break;

            case UPDATE_DELETE:   
               if (count < 0)
                  count = this->mValue.size() - position;
               this->mValue.erase(this->mValue.begin() + position, this->mValue.begin() + position + count);
               break;
               
            default:
               //NOOP
               break;
            }      
            
            this->mState = DATA_OK;            
         }
      };
      
           
      // ------------------------------------------------------------------------------------------
      
      
      template<typename DataT>
      struct ClientAttribute : AttributeBase<DataT>
      {           
         void resetState()
         {
            this->mState = DATA_NOT_AVAILABLE;
         }         
      };
         
    
   }   // namespace Private

}   //namespace DSI


// ------------------------------------------------------------------------------------------
            

template<typename DataT>
inline
DSI::CIStream& operator>>(DSI::CIStream& istream, DSI::Private::ClientAttribute<DataT>& attr)
{   
   istream >> attr.mValue;
   return istream;
}


template<typename DataT>
inline
DSI::COStream& operator<<(DSI::COStream& ostream, const DSI::Private::ServerAttribute<DataT>& attr)
{   
   ostream << attr.mValue;
   return ostream;
}


#endif //DSI_PRIVATE_ATTRIBUTES_HPP
