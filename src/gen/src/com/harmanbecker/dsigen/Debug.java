/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;

import java.io.PrintStream;
import java.util.Vector;


/**
 * Debug outputer for the dsi generator
 */
public final class Debug
{
   private Debug()
   {
   }

   private static PrintStream sDebugStream = System.out ;
   private static PrintStream sErrorStream = System.err ;
   private static Vector<String> sStack = new Vector<String>() ;

   /**
    * Flag that indicates if debug output should be printed
    */
   private static boolean debug = false ;

   public static void push( String file )
   {
      sStack.add( file );
   }

   public static void pop()
   {
      if( sStack.size() > 0 )
      {
         sStack.remove(sStack.size()-1);
      }
   }

   /**
    * Set debug output on or off
    * @param value on or off
    */
   public static void set( boolean value )
   {
      debug = value ;
   }

   /**
    * returns the debug output state
    * @return debug on or off
    */
   public static boolean get()
   {
      return debug  ;
   }

   /**
    * Prints a message if debug output is turned on
    * @param string the message
    */
   public static void message( String string )
   {
      if( debug )
      {
         sDebugStream.println( string );
      }
   }

   /**
    * Prints an exception message
    * @param t the exception
    */
   public static void error( Throwable t )
   {
      if( sStack.size() > 0  )
      {
         String currentFile = sStack.lastElement();
         sErrorStream.println( currentFile + ":-1: error: " + t.getMessage() );
      }
      else
      {
         sErrorStream.println( "error: " + t.getMessage() );
      }
      if( debug )
      {
         t.printStackTrace( sErrorStream );
      }
   }

   /**
    * Prints an error message if debug output is turned on
    * @param string the message
    */
   public static void error( String string ) throws DSIException
   {
      error( string, DSIException.ERR_UNSET );
   }

   public static void error( String string, int code ) throws DSIException
   {
      throw new DSIException( string, code );
   }

   /**
    * Prints an error message if debug output is turned on
    * @param string the message
    */
   public static void warning( String string )
   {
      if( sStack.size() > 0  )
      {
         String currentFile = sStack.lastElement();
         sErrorStream.println( currentFile +":-1: warning: " + string );
      }
      else
      {
         sErrorStream.println( "warning: " + string );
      }
   }

   public static PrintStream getDebugStream()
   {
      return sDebugStream ;
   }
}
