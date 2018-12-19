/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

#include "RegExp.hpp"


RegExp::RegExp()
: mFlags(0)
, mError(0)
{
  ::memset(&mRegex, 0, sizeof(regex_t));
  ::memset(mMatches, 0, sizeof(mMatches));
}


RegExp::~RegExp()
{
  free();
}


RegExp::RegExp( const RegExp& rhs )
: mFlags(0)
, mError(0)
{   
  ::memset(mMatches, 0, sizeof(mMatches));

  if( rhs.mPattern.size() )
  {
    (void)compile( rhs.mPattern, rhs.mFlags );
  }
  else
    ::memset(&mRegex, 0, sizeof(regex_t));
}


RegExp& RegExp::operator= ( const RegExp& rhs )
{
  if( this != &rhs )
  {
    free();

    if( rhs.mPattern.size() )      
      (void)compile( rhs.mPattern, rhs.mFlags );      
  }

  return *this ;
}
