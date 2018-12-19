/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

#include <cstdio>
#include <cstring>

#include "ConfigFile.hpp"
#include "Log.hpp"


ConfigFile::ConfigFile()
 : mLocalServices()   
 , mGlobalServices()
 , mForwardServices()
 , mTreeMode(false)
{
  // NOOP
}


ConfigFile::~ConfigFile()
{
  // NOOP
}


void ConfigFile::load( const std::string& filename )
{
  const char* fname = "/etc/servicebroker.cfg" ;

  bool useDefault = (filename.size() == 0);

  if (!useDefault)   
    fname = filename.c_str();   

  FILE *f = ::fopen(fname, "r");
  if (!f)
  {
    if (!useDefault)      
      Log::error( "Error loading config file '%s'", fname );      
  }
  else
  {
    char line[1024];
    enum State
    {
      STATE_IGNORE,
      STATE_LOCAL,
      STATE_GLOBAL,
      STATE_FORWARD
    } state = STATE_IGNORE ;

    Log::message( 1, "loading config file %s", fname );

    while(!::feof(f) && !::ferror(f))
    {
      if(::fgets( line, sizeof(line), f ))
      {
        // remove comments
        char* end = strchr( line, '#' );
        if( end )
        {
          *end-- = '\0' ;
        }
        else
        {
          end = line + strlen(line) - 1;
        }

        // right trim
        while( end >= line && *end <= 0x20 )
        {
          *end-- = '\0' ;
        }

        if( line[0] == '[' )
        {
          state = STATE_IGNORE ;

          char* cb = strchr( line, ']' );
          if( cb )
          {
            std::string sectionName( line+1, cb - line - 1 );
            if( sectionName == "LOCAL" )
            {
              state = STATE_LOCAL ;
            }
            else if( sectionName == "GLOBAL" )
            {
              state = STATE_GLOBAL ;
            }
            else if( sectionName == "FORWARD" )
            {
              state = STATE_FORWARD ;
            }
          }
        }
        else if( 0 != line[0] )
        {
          switch( state )
          {
          case STATE_LOCAL:
            Log::message( 3, "   local service: %s", line );
            mLocalServices.insert(line);
            break;

          case STATE_GLOBAL:
            Log::message( 3, "   global service: %s", line );
            mGlobalServices.insert(line);
            break;

          case STATE_FORWARD:
            Log::message( 3, "   forward service: %s", line );
            mForwardServices.insert(line);
            break;

          default:
            break;
          }
        }
      }
    }

    ::fclose( f );
  }
}


bool ConfigFile::forwardService(const std::string& servicename) const
{
  if (mTreeMode)
  {
    // shortcut - mainly for testing purpose
    if (mForwardServices.size() == 1 && *mForwardServices.begin() == "*")
      return true;

    std::string::size_type pos;      
    if (servicename.size() > 4 && (pos = servicename.rfind("_tcp", servicename.size()-4, 4)) == servicename.size()-4)
    {
      std::string the_servicename = servicename.substr(0, pos);
      return mForwardServices.find(the_servicename) != mForwardServices.end();
    }
    else      
      return mForwardServices.find(servicename) != mForwardServices.end();      
  }
  else
    return true;      
}


void ConfigFile::dumpStats( std::ostringstream& /*ostream*/ ) const
{
#if 0
  if(!mLocalServices.empty())
  {
    ostream << "\nLocal Services:\n" ;
    for(CServiceNameSet::const_iterator iter = mLocalServices.begin(); iter != mLocalServices.end(); ++iter)
    {
      ostream << "   " << *iter << "\n" ;
    }
  }
#endif
}

