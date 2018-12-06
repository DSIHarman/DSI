/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#include "InterfaceDescription.hpp"


InterfaceDescription& InterfaceDescription::operator= ( const SFNDInterfaceDescription& ifDescription )
{
  name = ifDescription.name ;
  majorVersion = ifDescription.version.majorVersion ;
  minorVersion = ifDescription.version.minorVersion ;

  return *this ;
}


bool InterfaceDescription::operator> ( const InterfaceDescription& ifDescription ) const
{
  if (name > ifDescription.name)
  {
    return true;
  }
  else if (name == ifDescription.name)
  {
    if (majorVersion > ifDescription.majorVersion)
    {
      return true;
    }
    else if (majorVersion == ifDescription.majorVersion)
    {
      return minorVersion > ifDescription.minorVersion;
    }
  }
  return false;
}
