/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_CISTREAM_HPP
#define DSI_CISTREAM_HPP


#include <errno.h>
#include <cassert>
#include <cstring>
#include <string>
#include <stdint.h>

#include <map>
#include <vector>

#include "DSI.hpp"
#include "dsi/TVariant.hpp"

#include "dsi/private/CNonCopyable.hpp"


namespace DSI
{

   /**
    * DSI payload deserializer, nothing more.
    */
   class CIStream : public Private::CNonCopyable
   {
   public:

      /**
       * Constructor.
       *
       * @param payload Pointer to payload data.
       * @param len Length of payload data.
       */
      CIStream(const char* payload, size_t len);
      
      /**
       * Generic read method for intrinsic data types.
       */
      template<typename T>
      void read(T& value);

      /**
       * Read method for bool.
       */
      void read(bool& b);

      /**
       * Read method for wide strings reading UTF-8 data from the stream.
       */
      void read(std::wstring& str);

      /**
       * Read method for single-byte strings as-is, aka buffers without any character set conversion.
       */
      void read(std::string& buf);
     
      /**
       * Raw read function. 
       *
       * @return the current pointer within the buffer.
       */
      inline
      const void* gptr() const
      {
         return mData + mOffset;
      }
      
      /**
       * Raw read function. 
       * 
       * @return the length of data that can still be read from the stream.
       */
      inline 
      size_t glen() const
      {
         return mSize - mOffset;
      }
      
      /**
       * Raw read function. Move the current data pointer on the stream.
       */
      inline
      void gbump(size_t len)
      {
         if (len <= glen())
         {
            mOffset += len;
         }
         else
         {
            mOffset = mSize;
            mError = ERANGE;
         }
      }
      
      /**
       * Retrieve the error indicator. 
       * 
       * @return 0 for no error, any other value for error condition on the stream (typically ERANGE).
       */
      inline
      int getError() const
      {
         return mError;
      }      

   private:

      /**
       * Read method for blob data.
       */
      void read( void *buffer, size_t size );

      const char* mData;
      size_t mSize;
      size_t mOffset;

      int mError;
   };


   // ----------------------------------------------------------------------------------
   
   
   template<typename T>
   inline
   void CIStream::read(T& value)
   {   
      // if an error occured we just do nothing
      if( 0 == mError )
      {
         // aligning the read offset.
         mOffset += ((-mOffset) & (sizeof(T)/*=alignment*/ - 1));
         
         // checking if there is enough data in the stream
         if((mOffset+sizeof(T)) <= mSize )
         {
            value = *(T*)(mData+mOffset) ;
            mOffset += sizeof(T) ;
         }
         else
         {
            mError = ERANGE ;
         }
      }
   }


   inline
   void CIStream::read(void * buffer, size_t size)
   {
      if(0 == mError)
      {
         if(size <= (mSize-mOffset))
         {
            memcpy( buffer, mData + mOffset, size );
            mOffset += size ;
         }
         else
         {
            mError = ERANGE ;
         }
      }
   }


   inline
   void CIStream::read(bool& b)
   {
      int32_t bval = 0 ;
      read( bval ) ;
      b = (0 != bval) ;
   }

} //namespace DSI


inline
DSI::CIStream& operator>>(DSI::CIStream& str, std::string& s)
{
   str.read(s);
   return str;
}


inline
DSI::CIStream& operator>>(DSI::CIStream& str, std::wstring& s)
{
   str.read(s);
   return str;
}


#define MAKE_DSI_DESERIALIZING_OPERATOR(type)               \
   inline                                                   \
   DSI::CIStream& operator>>(DSI::CIStream& str, type& t)   \
   {                                                        \
      str.read(t);                                          \
      return str;                                           \
   }

MAKE_DSI_DESERIALIZING_OPERATOR(int8_t)
MAKE_DSI_DESERIALIZING_OPERATOR(int16_t)
MAKE_DSI_DESERIALIZING_OPERATOR(int32_t)
MAKE_DSI_DESERIALIZING_OPERATOR(int64_t)

MAKE_DSI_DESERIALIZING_OPERATOR(uint8_t)
MAKE_DSI_DESERIALIZING_OPERATOR(uint16_t)
MAKE_DSI_DESERIALIZING_OPERATOR(uint32_t)
MAKE_DSI_DESERIALIZING_OPERATOR(uint64_t)

MAKE_DSI_DESERIALIZING_OPERATOR(double)
MAKE_DSI_DESERIALIZING_OPERATOR(float)

MAKE_DSI_DESERIALIZING_OPERATOR(bool)


template<typename T>      
inline                                       
DSI::CIStream& operator>>(DSI::CIStream& str, T& t) 
{     
   DSI_STATIC_ASSERT(std::tr1::is_enum<T>::value);
   
   str.read((uint32_t&)t);         
   return str;   
} 


#define DSI_VARIANT_DESERIALIZATIONVISITOR(baseclass)         \
   class DeserializationVisitor : public baseclass <>         \
   {                                                          \
   public:                                                    \
      explicit inline                                         \
      DeserializationVisitor(DSI::CIStream& is) : mIs(is) { } \
      template<typename T> inline                             \
      void operator()(T& t) { mIs >> t; }                     \
      inline void operator()() { assert(false); }             \
   private:                                                   \
      DSI::CIStream& mIs;                                     \
   }


namespace DSI
{
   namespace Private
   {
      DSI_VARIANT_DESERIALIZATIONVISITOR(TStaticVisitor);      
      
      template<typename T>
      struct VariantDeserializationHelperHelper
      {
         template<typename VariantT>
         static
         void eval(VariantT& v)
         {
            v.template reset<T>();
         }
      };
      
      template<>
      struct VariantDeserializationHelperHelper<Private::SNilType>
      {
         template<typename VariantT>
         static
         void eval(VariantT& v)
         {
            v.reset();
         }
      };

      struct VariantDeserializationHelper
      {
         template<typename VariantT>
         static inline
         void reset(VariantT& v, int idx)
         {
            typedef void(*CallfuncType)(VariantT&);
            static const CallfuncType funcs[] = {         
               &VariantDeserializationHelperHelper<Private::SNilType>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<0, typename VariantT::tTypelistType>::type>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<1, typename VariantT::tTypelistType>::type>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<2, typename VariantT::tTypelistType>::type>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<3, typename VariantT::tTypelistType>::type>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<4, typename VariantT::tTypelistType>::type>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<5, typename VariantT::tTypelistType>::type>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<6, typename VariantT::tTypelistType>::type>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<7, typename VariantT::tTypelistType>::type>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<7, typename VariantT::tTypelistType>::type>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<8, typename VariantT::tTypelistType>::type>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<10, typename VariantT::tTypelistType>::type>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<11, typename VariantT::tTypelistType>::type>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<12, typename VariantT::tTypelistType>::type>::eval,
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<13, typename VariantT::tTypelistType>::type>::eval,            
               &VariantDeserializationHelperHelper<typename Private::TypeListAlgos::RelaxedTypeAt<14, typename VariantT::tTypelistType>::type>::eval         
            };
                     
            if (idx > 0 && idx <= Private::TypeListAlgos::Size<typename VariantT::tTypelistType>::value)
            {            
               funcs[idx](v);
            }
            else
            {            
               VariantDeserializationHelperHelper<Private::SNilType>::template eval(v);            
            }
         }   
      };

   }   // namespace Private
   
}   // namespace DSI


template<typename TypelistT>
inline
DSI::CIStream& operator>>(DSI::CIStream& is, DSI::TVariant<TypelistT>& var)
{
   int32_t typeId = 0;
   is >> typeId;     

   DSI::Private::VariantDeserializationHelper::reset(var, typeId);
   
   if (!var.isEmpty())   
   {            
      DSI::Private::DeserializationVisitor vst(is);
      DSI::staticVisit(vst, var);
   }
   
   return is;
}


template<typename T>
inline
DSI::CIStream& operator>>(DSI::CIStream& is, std::vector<T>& v)
{
   int32_t newSize = 0;   
   is >> newSize;   
   v.resize(newSize);   
   
   for(typename std::vector<T>::iterator iter = v.begin(); iter != v.end(); ++iter)
   {   
      is >> *iter;
   }   
   
   return is;
}


// FIXME possibly need optimization - no temporaries
template<typename KeyT, typename ValueT>
inline
DSI::CIStream& operator>>(DSI::CIStream& is, std::map<KeyT, ValueT>& m)
{
   int32_t newSize = 0;   
   is >> newSize;   
   
   KeyT key;
   ValueT value;
      
   while(newSize-- > 0)
   {      
      is >> key;
      is >> value;
      
      m[key] = value;
   }   
   
   return is;
}


#endif // DSI_CISTREAM_HPP
