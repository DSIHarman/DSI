/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;

import java.util.Enumeration;
import java.util.List;

import org.jdom.Element;

@SuppressWarnings("unchecked")
public class Method extends DataType
{
   private String mResponse ;

   /* ************************************************************ */

   public Method( Element node, ServiceInterface si ) throws Exception
   {
      super( node, si );
      mName = mName.substring(0, 1).toUpperCase() + mName.substring(1);
   }

   /* ************************************************************ */

   protected boolean readElement( Element node ) throws Exception
   {
      switch( XML.getID( node ) )
      {
      case XML.RESPONSE:
         mResponse = node.getValue() ;
         break;
      case XML.PARAMETERS:
         readParameters( node );
         break;
      default:
         return super.readElement( node ) ;
      }
      return true ;
   }

   /* ************************************************************ */

   private void readParameters( Element node ) throws Exception
   {
      boolean first = true ;
      List<Element> children = node.getChildren();
      for( Element child : children )
      {
         if( XML.PARAMETER == XML.getID( child) )
         {
            Value parameter = new Value( child, mServiceInterface, first );
            parameter.resolve();
            mFields.add( parameter );
            first = false ;
         }
      }
   }

   /* ************************************************************ */
   /**
    * The parameter list of a method always is a structure.
    */
   public boolean isStructure()
   {
      return true ;
   }

   /* ************************************************************ */

   public boolean isRequest()
   {
      return getType().toLowerCase().matches("request|(un)?register");
   }

   /* ************************************************************ */

   public boolean isInformation()
   {
      return getType().toLowerCase().matches("information");
   }

   /* ************************************************************ */

   public boolean isRegister()
   {
      return getType().equalsIgnoreCase("register");
   }

   /* ************************************************************ */

   public boolean isUnregister()
   {
      return getType().equalsIgnoreCase("unregister");
   }

   /* ************************************************************ */

   public String getType()
   {
      return super.getType().toLowerCase();
   }

   /* ************************************************************ */

   public String getParameterStructName()
   {
      return mServiceInterface.getName() + "_" + Util.doCapitalLetter( getType() ) + getPlainName() + "ArgList";
   }

   /* ************************************************************ */

   public String getDSIBindingName( boolean addNamespace )
   {
      if( addNamespace )
         return mServiceInterface.getName() + "::" + Util.doCapitalLetter( getType() ) + getPlainName() + "ArgList";
      else
         return Util.doCapitalLetter( getType() ) + getPlainName() + "ArgList";
   }

   /* ************************************************************ */

   public String getCallbackName()
   {
      return "fn" + Util.doCapitalLetter( mType ) + getPlainName() ;
   }

   /* ************************************************************ */

   public String getMethodName()
   {
      return mType.toLowerCase() + getPlainName() ;
   }

   /* ************************************************************ */

   public String getFunctionName()
   {
      return mServiceInterface.getName() + "_" + Util.doCapitalLetter( mType ) + getPlainName() ;
   }

   /* ************************************************************ */

   public String getUpdateIdName()
   {
      return mServiceInterface.getName() + "_UPD_ID_" + getType() + getPlainName();
   }

   /* ************************************************************ */

   public String getDSIUpdateIdName( boolean addNamespace )
   {
      if( addNamespace )
         return mServiceInterface.getName() + "::" + getDSIUpdateIdName( false );
      else
         return "UPD_ID_" + getType() + getPlainName();
   }

   /* ************************************************************ */

   public String getResponse()
   {
      return mResponse ;
   }

   /* ************************************************************ */

   public boolean hasResponse()
   {
      return null != mResponse ;
   }

   /* ************************************************************ */

   public boolean hasParameters()
   {
      return 0 != mFields.size();
   }

   /* ************************************************************ */

   public Value[] getParameters()
   {
      return getFields();
   }

/* ************************************************************ */

   public boolean hasVectorOrBuffer()
   {
      boolean retval = false ;
      Enumeration<Value> e = mFields.elements();
      while( !retval && e.hasMoreElements() )
      {
         Value field = e.nextElement();
         retval = field.getDataType().isVector() || field.getDataType().isBuffer();
      }
      return retval ;
   }

   /* ************************************************************ */
}
