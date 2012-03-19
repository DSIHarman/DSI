/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;

import java.util.List;

import org.jdom.Element;

@SuppressWarnings( "unchecked" )
public class Util
{
   public static String doCapitalLetter( String str )
   {
      return str.substring(0, 1).toUpperCase() + str.substring(1);
   }

   public static String getBaseName( String filename )
   {
      int index = filename.replace('\\', '/').lastIndexOf('/') ;
      if( -1 != index )
      {
         filename = filename.substring( 1 + index );
      }
      index = filename.indexOf('.') ;
      if( -1 != index )
      {
         filename = filename.substring( 0, index );
      }
      return filename ;
   }

   public static String getPath( String filename )
   {
      int index = filename.replace('\\', '/').lastIndexOf('/') ;
      if( -1 != index )
      {
         filename = filename.substring( 0, index );
      }
      return filename ;
   }

   public static boolean checkFlag( Element node, int xmlId ) throws DSIException
   {
      boolean isSet = false ;
      List<Element> children = node.getChildren();
      for( Element child : children )
      {
         if( xmlId == XML.getID( child ) )
         {
            isSet = Boolean.parseBoolean( child.getValue() ) ;
            break;
         }
      }
      return isSet ;
   }
}
