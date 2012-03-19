/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "dsi/TVariant.hpp"
#include "dsi/COStream.hpp"
#include "dsi/CIStream.hpp"
#include "dsi/DSI.hpp"
#include "dsi/CRequestWriter.hpp"

#include "CDummyChannel.hpp"


class TVariantTest : public CppUnit::TestFixture
{
public:
   CPPUNIT_TEST_SUITE(TVariantTest);
      CPPUNIT_TEST(testSizeOf);
      CPPUNIT_TEST(testSimpleSerialization);      
      CPPUNIT_TEST(testInvalidSerialization);      
      CPPUNIT_TEST(testComplexSerialization);      
   CPPUNIT_TEST_SUITE_END();

public:
   void testSizeOf();   
   void testSimpleSerialization();   
   void testInvalidSerialization();
   void testComplexSerialization();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TVariantTest);


// --------------------------------------------------------------------------------


void TVariantTest::testSizeOf()
{
   DSI::TVariant<DSI_TYPELIST_2(int32_t, double)> v;
   
   CPPUNIT_ASSERT(sizeof(double) == 8);
   CPPUNIT_ASSERT(std::tr1::alignment_of<double>::value == 8);
   
   CPPUNIT_ASSERT(v.size == sizeof(double));
   CPPUNIT_ASSERT(v.alignment == std::tr1::alignment_of<double>::value);
            
   CPPUNIT_ASSERT(sizeof(v) == sizeof(double) + sizeof(int)/*the idx*/ + sizeof(int)/*alignment stuff between idx and data*/);
}


void TVariantTest::testSimpleSerialization()
{
   const double d = 3.1415;            
   
   for(int i=0; i<3; ++i)
   {      
      DSI::TVariant<DSI_TYPELIST_3(int32_t, double, std::string)> v;
      
      switch(i)
      {        
      case 0:
         v = 42;
         break;
         
      case 1:         
         v = d;   // avoid implicit casts to int
         break;
         
      case 2:
         v = std::string("Hallo Welt");
         break;
      
      default:
         CPPUNIT_FAIL("invalid loop count");
         break;
      }      
      
      DSI::CRequestWriter writer;
      DSI::COStream os(writer);                
      os << v;
      
      DSI::TVariant<DSI_TYPELIST_3(int32_t, double, std::string)> v1;            
      DSI::CIStream is(writer.gptr(), writer.size());
      is >> v1;
      
      CPPUNIT_ASSERT(v == v1);
   }   
}


void TVariantTest::testInvalidSerialization()
{         
   // different variants - if target variant is too small it must fail!
   DSI::TVariant<DSI_TYPELIST_3(int32_t, double, std::string)> v;
   v = std::string("Hallo Welt");
   
   DSI::CRequestWriter writer;
   DSI::COStream os(writer);                
   os << v;
      
   DSI::TVariant<DSI_TYPELIST_2(int32_t, double)> v1;            
   DSI::CIStream is(writer.gptr(), writer.size());
   is >> v1;
   
   CPPUNIT_ASSERT(v1.isEmpty());
}


void TVariantTest::testComplexSerialization()
{   
   typedef std::map<uint32_t, std::string> tMapType;   
   typedef std::vector<tMapType> tVectorType;
   typedef DSI_TYPELIST_3(int32_t, tVectorType, std::string) tTypelistType;
   
   DSI::TVariant<tTypelistType> v;
   
   tMapType m;
   m[42] = "Hallo Welt";
   m[7] = "The code guru";
   
   tVectorType vec;   
   vec.push_back(m);
   
   v = vec;
   
   DSI::CRequestWriter writer;
   DSI::COStream os(writer);                
   os << v;
      
   DSI::TVariant<tTypelistType> v1;            
   DSI::CIStream is(writer.gptr(), writer.size());
   is >> v1;
   
   CPPUNIT_ASSERT(v == v1);
}
