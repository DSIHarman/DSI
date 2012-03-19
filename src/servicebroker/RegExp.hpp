/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_REGEXP_HPP
#define DSI_SERVICEBROKER_REGEXP_HPP

#include <string>

#include <cstring>

#include "regex.h"

#define MAX_REGEX_MATCHES 10


class RegExp
{
public:
   RegExp();
   ~RegExp();

   RegExp( const RegExp& );
   RegExp& operator = ( const RegExp& );

   /*
    * compiles the regular expression passed in as argument.
    */
   int compile( const std::string& pattern, int cflags );

   /*
    * Matches the regular expression against the given string.
    */
   int execute( const std::string& string, int eflags );

   /**
    * Gives an error message to a given error code.
    */
   size_t error( int errcode, char *errbuf, size_t errbuf_size );

   /*
    * Frees the regular expression
    */
   void free();

   /*
    * Is the regular expression valid?
    */
   bool isValid();

   unsigned int getMatchCount();

   std::string getMatch( int idx );

private:

   regex_t mRegex;

   std::string mPattern ;
   std::string mInput ;
   int32_t mFlags ;
   regmatch_t mMatches[MAX_REGEX_MATCHES];

   int32_t mError;
};


inline 
int RegExp::compile( const std::string& pattern, int cflags )
{
   free();
   int rc = ::regcomp( &mRegex, pattern.c_str(), cflags );
   if( 0 == rc )
   {
      mPattern = pattern ;
      mFlags = cflags ;
   }

   mError = rc;

   return rc;
}


inline 
size_t RegExp::error( int errcode, char *errbuf, size_t errbuf_size )
{
   return ::regerror( errcode, &mRegex, errbuf, errbuf_size );
}


inline 
int RegExp::execute( const std::string& string, int eflags )
{
   mInput = string ;
   return ::regexec( &mRegex, string.c_str(), MAX_REGEX_MATCHES, mMatches, eflags );
}


inline 
void RegExp::free()
{
   if (isValid())
      ::regfree(&mRegex);
}


inline 
unsigned int RegExp::getMatchCount()
{
   unsigned int count = 0 ;
   while( count < MAX_REGEX_MATCHES && mMatches[count].rm_so != -1 )
   {
      count ++ ;
   }
   
   return count ;
}


inline 
std::string RegExp::getMatch( int32_t idx )
{
   return std::string( mInput.c_str() + mMatches[idx].rm_so, mMatches[idx].rm_eo - mMatches[idx].rm_so ) ;
}


inline 
bool RegExp::isValid()
{
   return 0 == mError;
}

#endif   // DSI_SERVICEBROKER_REGEXP_HPP

