/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;

import java.util.Vector;

public class GenerateHelperBase
{
   private Vector<String> mCode = new Vector<String>() ;
   private Vector<String> mIndentStack = new Vector<String>() ;
   protected int mVariableIndex = 0 ;
   protected ServiceInterface mServiceInterface ;

   GenerateHelperBase( ServiceInterface si )
   {
      mServiceInterface = si ;
   }

   /* ************************************************************ */

   protected String createUniqueVariable()
   {
      return "_var" + mVariableIndex++ ;
   }

   /* ************************************************************ */

   protected String createUniqueVariable( String name )
   {
      return "_" + name + mVariableIndex++ ;
   }

   /* ************************************************************ */

   protected void pushIndent( int indent )
   {
      StringBuffer buf ;
      if( 0 < mIndentStack.size() )
      {
         buf = new StringBuffer( mIndentStack.lastElement() );
      }
      else
      {
         buf = new StringBuffer();
      }

      while( 0 < indent-- )
      {
         buf.append("   ");
      }

      mIndentStack.add( buf.toString() );
   }

   /* ************************************************************ */

   protected void popIndent()
   {
      if( 0 <  mIndentStack.size() )
      {
         mIndentStack.remove( mIndentStack.size() - 1 );
      }
   }

   /* ************************************************************ */

   protected String[] prepareCode()
   {
      String[] retCode = mCode.toArray( new String[mCode.size()] ) ;
      mCode.clear();
      mIndentStack.clear();
      mVariableIndex = 0 ;
      return retCode ;
   }

   /* ************************************************************ */

   protected void addCode( String line )
   {
      if( null != line )
      {
         String indent = "" ;
         if( 0 < mIndentStack.size() )
         {
            indent = mIndentStack.lastElement();
         }
         //mCode.add( "/* " + indent.length() + " */" + indent + line );
         mCode.add( indent + line );
      }
   }

   /* ************************************************************ */



   /* ************************************************************ */

   protected static String makeIndent( int indent )
   {
      StringBuffer buf = new StringBuffer();
      while( 0 < indent-- )
      {
         buf.append("   ");
      }
      return buf.toString();
   }

   /* ************************************************************ */
}
