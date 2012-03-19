/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_DESCRIPTION_HPP
#define DSI_SERVICEBROKER_DESCRIPTION_HPP

#include <string>
#include <cstring>

#include "dsi/private/servicebroker.h"


struct InterfaceDescription
{
   std::string name ;      // interface name
   
   int32_t majorVersion ;  // interface major version
   int32_t minorVersion ;  // interface minor version

   InterfaceDescription();
   InterfaceDescription( const SFNDInterfaceDescription& ifDescription );

   // converting SFNDInterfaceDescription -> InterfaceDescription
   InterfaceDescription& operator = ( const SFNDInterfaceDescription& ifDescription );

   // converting InterfaceDescription -> SFNDInterfaceDescription
   operator SFNDInterfaceDescription() const;

   bool operator == ( const InterfaceDescription& ifDescription ) const ;
   bool operator == ( const SFNDInterfaceDescription& ifDescription ) const ;
   bool operator > ( const InterfaceDescription& ifDescription ) const ;
};


inline 
InterfaceDescription::InterfaceDescription() 
 : majorVersion(0) 
 , minorVersion(0)
{
   // NOOP
}


inline 
InterfaceDescription::InterfaceDescription( const SFNDInterfaceDescription& ifDescription )
 : name(ifDescription.name)
 , majorVersion(ifDescription.version.majorVersion)
 , minorVersion(ifDescription.version.minorVersion) 
{
   // NOOP
}


inline 
InterfaceDescription::operator SFNDInterfaceDescription() const
{
   SFNDInterfaceDescription ifDescription ;
   strncpy( ifDescription.name, name.c_str(), sizeof(ifDescription.name) ) ;
   ifDescription.version.majorVersion = majorVersion ;
   ifDescription.version.minorVersion = minorVersion ;
   return ifDescription ;
}


inline 
bool InterfaceDescription::operator== ( const SFNDInterfaceDescription& ifDescription ) const
{
   return name == ifDescription.name
       && majorVersion == ifDescription.version.majorVersion
       && minorVersion == ifDescription.version.minorVersion ;
}


inline 
bool InterfaceDescription::operator== ( const InterfaceDescription& ifDescription ) const
{
   return name == ifDescription.name
       && majorVersion == ifDescription.majorVersion
       && minorVersion == ifDescription.minorVersion ;
}


#endif // DSI_SERVICEBROKER_DESCRIPTION_HPP

