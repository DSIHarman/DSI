/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;

import org.jdom.Element;

public class EnumID extends BaseObject
{
   /* ************************************************************ */

   private ServiceInterface mServiceInterface ;
   private String mValue = null ;
   private int mIntegerValue = -1 ;
   private Exception mException = null ;

   /* ************************************************************ */

   public EnumID( Element node, ServiceInterface si ) throws Exception
   {
      mServiceInterface = si ;
      readElements( node );
   }

   /* ************************************************************ */

   protected boolean readElement( Element node )
   {
      switch( XML.getID( node ) )
      {
      case XML.VALUE:
         mValue = node.getValue().trim() ;
         break;
      default:
         return false ;
      }
      return true ;
   }

   /* ************************************************************ */

   public int resolveValue( int value )
   {
      if( null != mValue && 0 != mValue.length() )
      {
         try
         {
            mIntegerValue = getIntegerValue( mValue );
         }
         catch( Exception ex )
         {
            mException = ex ;
         }
      }
      else
      {
         mIntegerValue = value ;
      }
      return mIntegerValue ;
   }

   /* ************************************************************ */

   public boolean hasValue()
   {
      return null != mValue ;
   }

   /* ************************************************************ */

   public String getValue()
   {
      return mValue ;
   }

   /* ************************************************************ */

   public int getIntegerValue() throws Exception
   {
      if( null != mException )
      {
         throw mException ;
      }
      return mIntegerValue ;
   }

   /* ************************************************************ */

   private int getIntegerValue( String value )  throws Exception
   {
      if( value.matches("'.'") )
      {
         return (int) value.charAt(1);
      }
      else if( value.startsWith("0x") )
      {
         return (int)Long.parseLong( value.substring(2), 16 );
      }
      else
      {
         try
         {
            return Integer.parseInt( value );
         }
         catch( NumberFormatException ex )
         {
            if( value.contains("|") )
            {
               String val ;
               int ival = 0 ;
               int next = 0 ;
               do
               {
                  int pos = next ;
                  next = value.indexOf('|', next+1) ;
                  if( pos > 0 )
                     pos++;
                  val = next == -1 ? value.substring(pos) : value.substring(pos, next);
                  ival |= getIntegerValue( val.trim() );
               } while( -1 != next );

               return ival ;
            }
            else
            {
               EnumID enumID = mServiceInterface.lookupEnumeration( value );
               if( null == enumID )
               {
                  throw ex ;
               }
               else
               {
                  return enumID.getIntegerValue();
               }
            }
         }
      }
   }

   /* ************************************************************ */
}
