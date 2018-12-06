/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_COSTREAM_HPP
#define DSI_COSTREAM_HPP

#include <string>
#include <vector>
#include <map>

#include "dsi/TVariant.hpp"
#include "dsi/CRequestWriter.hpp"

#include "dsi/private/CNonCopyable.hpp"


namespace DSI
{   

   /**
    * A DSI output serializer. This stream also sends the whole data.
    */
   class COStream : public Private::CNonCopyable
   {      
   public:

      /**
       * Constructor.
       */
      inline
      COStream(CRequestWriter& writer)
       : mWriter(writer)
      {
         // NOOP
      }      
            
      /**
       * Generic write method for intrinsic data types. The data will be aligned to sizeof(data).
       */
      template<typename T>
      COStream& write(const T& value);      
      
      /**
       * Write method for bool.
       */
      COStream& write(bool b);

      /**
       * Write method for wide strings (generating UTF-8 sequences on the stream).
       */
      COStream& write(const std::wstring& str);

      /**
       * Write method for single-byte strings as-is, no character set conversion.
       */
      COStream& write(const std::string& buf);      
            
      /**
       * Write method for blob data.
       */
      COStream& write(const void* data, size_t size);
      
   private:

      /**
       * Increase capacity of the underlying buffer object.
       */
      bool setCapacity(size_t capacity);
            
   private:        
      
      CRequestWriter& mWriter;      
   };


// ----------------------------------------------------------------------------------------


   template<typename T>
   inline
   COStream& COStream::write( const T& value )
   {      
      if((2*sizeof(T)) < mWriter.avail() || setCapacity(mWriter.size() + (2*sizeof(T))))
      {         
         // align according to data type alignment requirements
         mWriter.pbump((-mWriter.size()/*=current offset*/) & (sizeof(T)/*=alignment*/ - 1));

         // copy data to the buffer
         *(T*)(mWriter.pptr()) = value;

         // increase buffer size
         mWriter.pbump(sizeof(T));
      }
      
      return *this;
   }


   inline
   COStream& COStream::write(const void* data, size_t size)
   {
      if (size < mWriter.avail() || setCapacity( mWriter.size() + size ))
      {
         memcpy(mWriter.pptr(), data, size);
         mWriter.pbump(size);
      }
      
      return *this;
   }


   inline
   COStream& COStream::write(bool b)
   {
      return write(b ? 1 : 0);
   }


   inline
   COStream& COStream::write(const std::string& buf)
   {
      write(static_cast<uint32_t>(buf.size()) );

      if (0 != buf.size())
         (void)write(buf.data(), buf.size());
         
      return *this;
   }


   inline
   bool COStream::setCapacity(size_t newCapacity)
   {
      (void)mWriter.sbrk(newCapacity - mWriter.size());         
      return true;
   }

}   //namespace DSI


inline
DSI::COStream& operator<<(DSI::COStream& str, const std::string& s)
{
   return str.write(s);   
}


inline
DSI::COStream& operator<<(DSI::COStream& str, const std::wstring& s)
{
   return str.write(s);   
}


#define MAKE_DSI_SERIALIZING_OPERATOR(type)              \
   inline                                                \
   DSI::COStream& operator<<(DSI::COStream& str, type t) \
   {                                                     \
      return str.write(t);                               \
   }

MAKE_DSI_SERIALIZING_OPERATOR(int8_t)
MAKE_DSI_SERIALIZING_OPERATOR(int16_t)
MAKE_DSI_SERIALIZING_OPERATOR(int32_t)
MAKE_DSI_SERIALIZING_OPERATOR(int64_t)

MAKE_DSI_SERIALIZING_OPERATOR(uint8_t)
MAKE_DSI_SERIALIZING_OPERATOR(uint16_t)
MAKE_DSI_SERIALIZING_OPERATOR(uint32_t)
MAKE_DSI_SERIALIZING_OPERATOR(uint64_t)

MAKE_DSI_SERIALIZING_OPERATOR(double)
MAKE_DSI_SERIALIZING_OPERATOR(float)

MAKE_DSI_SERIALIZING_OPERATOR(bool)           
      
      
template<typename T>
inline
DSI::COStream& operator<<(DSI::COStream& str, const T& t)
{
   static_assert(std::tr1::is_enum<T>::value, "");
   
   return str.write((uint32_t&)t);
}


#define DSI_VARIANT_SERIALIZATIONVISITOR(baseclass)            \
      class SerializationVisitor : public baseclass <>         \
      {                                                        \
      public:                                                  \
         explicit inline                                       \
         SerializationVisitor(DSI::COStream& os) : mOs(os) { } \
         template<typename T> inline                           \
         void operator()(const T& t) { mOs << t; }             \
         inline void operator()() { }                          \
      private:                                                 \
         DSI::COStream& mOs;                                   \
      }
      
      
namespace DSI
{

   namespace Private
   {
      DSI_VARIANT_SERIALIZATIONVISITOR(TStaticVisitor);      
      
   }   // namespace Private
   
}   // namespace DSI
      
      
template<typename TypelistT>
inline
DSI::COStream& operator<<(DSI::COStream& os, const DSI::TVariant<TypelistT>& var)
{   
   int32_t typeId = var.getTypeId();
   os << typeId;
   
   DSI::Private::SerializationVisitor vst(os);
   DSI::staticVisit(vst, var);
   
   return os;   
}


template<typename T>
inline
DSI::COStream& operator<<(DSI::COStream& os, const std::vector<T>& v)
{
   int32_t size = v.size();
   os << size;
   
   for(typename std::vector<T>::const_iterator iter = v.begin(); iter != v.end(); ++iter)
   {
      os << *iter;
   }
   
   return os;
}


template<typename KeyT, typename ValueT>
inline
DSI::COStream& operator<<(DSI::COStream& os, const std::map<KeyT, ValueT>& m)
{
   int32_t size = m.size();
   os << size;
   
   for(typename std::map<KeyT, ValueT>::const_iterator iter = m.begin(); iter != m.end(); ++iter)
   {
      os << iter->first;
      os << iter->second;
   }   
   
   return os;
}

#endif // DSI_COSTREAM_HPP


