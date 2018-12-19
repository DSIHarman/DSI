/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;

import org.jdom.Element;

public class Constant extends BaseObject
{
   /* ************************************************************ */

   private String mValue ;
   private DataType mDataType ;
   private ServiceInterface mServiceInterface ;

   /* ************************************************************ */

   public Constant( Element node, ServiceInterface si ) throws Exception
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
      case XML.TYPE:
         mDataType = mServiceInterface.resolveDataType( node.getValue().trim() ) ;
         break;
      default:
         return false ;
      }
      return true ;
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

   public int getIntegerValue()
   {
      if( mValue.matches("'.'") )
      {
         return (int) mValue.charAt(1);
      }
      else if( mValue.startsWith("0x") )
      {
         return Integer.parseInt( mValue.substring(2), 16 );
      }
      else
      {
         return Integer.parseInt( mValue );
      }
   }

   /* ************************************************************ */

   public DataType getDataType()
   {
      return mDataType ;
   }

   /* ************************************************************ */
}
