/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/
#ifndef DSI_SERVICEBROKER_CONFIGFILE_HPP
#define DSI_SERVICEBROKER_CONFIGFILE_HPP


#include <set> 
#include <string>
#include <sstream>


// FIXME could use unordered_set here?
typedef std::set<std::string> CServiceNameSet;


/**
 * The servicebroker config file contains .ini like entries including sections.
 * Sections are marked in braces []. The following sections are available:
 * 
 * <ul> 
 *   <li> [LOCAL]   Contains services which are only visible on the node this servicebroker is running on.
 *   <li> [GLOBAL]  Contains services which should be visible to all. 
 *   <li> [FORWARD] Contains services which should be forwarded to the parent servicebroker (only in tree mode).
 *                  An asterisk line '*' is a special marker which means <i>forward all services</i>.
 * </ul>
 *
 * Note that the [GLOBAL] section is preferred to the [LOCAL] section. So when there is any entry in the global
 * section, the [GLOBAL] section list will be evaluated against a given service interface, the [LOCAL] section will
 * be ignored.
 */
class ConfigFile
{
public:

   ConfigFile();
   ~ConfigFile();

   /**
    * Loads the configuration from a file. If the default configuration file
    * does not exist, nothing happens. Otherwise a trace error is written
    * to SLog
    *
    * @param filename the configuration file
    */
   void load( const std::string& filename );

   /**
    * checks is the given service name is a local service
    *
    * @param servicename the servicename to check
    * @return true if the service is local, otherwise false
    */
   bool isLocalService( const std::string& servicename ) const;
   
   /**
    * @return true if the given servicename is within the forwarding
    *         section of the config file.
    */
   bool forwardService( const std::string& servicename ) const;

   /**
    * Tree mode (as given at the command line via the switch '-t') 
    * has to be set on the config object since this influences the 
    * return values of the @c forwardService method.
    */
   void setTreeMode( bool mode );

   /**
    * @return true if the servicebroker is started in tree mode.
    */
   bool isTreeMode() const;
   
   /**
    * dumps status information about the configuration to a stream
    */
   void dumpStats( std::ostringstream& ostream ) const;

   
private:

   CServiceNameSet mLocalServices ;
   CServiceNameSet mGlobalServices ;
   CServiceNameSet mForwardServices ;
   
   bool mTreeMode ;
};


// ------------------------------------------------------------------------------------


inline
bool ConfigFile::isTreeMode() const
{
   return mTreeMode;
}


inline 
bool ConfigFile::isLocalService( const std::string& servicename ) const
{
   if (!mGlobalServices.empty())
   {
      return mGlobalServices.find(servicename) == mGlobalServices.end();
   }
   else
   {
      return mLocalServices.find(servicename) != mLocalServices.end();
   }
}


inline 
void ConfigFile::setTreeMode( bool mode )
{
   mTreeMode = mode ;
}


#endif //DSI_SERVICEBROKER_CONFIGFILE_HPP

