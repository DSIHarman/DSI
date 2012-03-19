/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Enumeration;
import java.util.Vector;

@SuppressWarnings("serial")
public class SearchPath
{
   private static Vector<String> sSearchPaths = new Vector<String>()
   {
      // avoid duplicates
      public boolean add( String path )
      {
         return super.contains(path) ? true : super.add( path ) ;
      }
   };

   private SearchPath()
   {
   }

   public static void add( String path )
   {
      String[] pathList = path.split(";");
      for( String s : pathList )
      {
         sSearchPaths.add( s );
      }
   }

   public static File find( String filename ) throws FileNotFoundException
   {
      File file = new File( filename );
      if( file.isAbsolute() )
      {
         if( file.exists() )
         {
            return file ;
         }
      }
      else
      {
         Enumeration<String> e = sSearchPaths.elements();
         while( e.hasMoreElements() )
         {
            String path = e.nextElement();
            file = new File(path + "/" + filename);
            if( file.exists() )
            {
               return file ;
            }
         }
      }
      throw new FileNotFoundException( "File not Found: " + filename ) ;
   }

   public static void dump()
   {
      Debug.message("search path:");
      Enumeration<String> e = sSearchPaths.elements();
      while( e.hasMoreElements() )
      {
         Debug.message("   " + e.nextElement());
      }
   }
}
