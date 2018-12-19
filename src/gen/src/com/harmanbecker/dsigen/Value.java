/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;

import org.jdom.Element;

public class Value extends BaseObject
{
   private String mDataTypeName = null ;
   private DataType mDataType = null ;
   private String mNotify = null ;
   private String mDefaultValue = null ;
   private boolean mFirst = false ;
   private ServiceInterface mServiceInterface ;

   /* ************************************************************ */

   public Value( Element node, ServiceInterface si, boolean first ) throws Exception
   {
      mFirst = first ;
      mServiceInterface = si ;
      readElements( node );
   }

   /* ************************************************************ */

   public Value( Element node, ServiceInterface si ) throws Exception
   {
      mServiceInterface = si ;
      readElements( node );
   }

   /* ************************************************************ */

   public Value( String name, String description, int id ) throws Exception
   {
      mName = name ;
      mDescription = description ;
      mID = id ;
   }

   /* ************************************************************ */

   protected boolean readElement( Element node )
   {
      switch( XML.getID( node ) )
      {
      case XML.NOTIFY:
         mNotify = node.getValue() ;
         break;
      case XML.DEFAULTVALUE:
         mDefaultValue = node.getValue() ;
         break;
      case XML.ISDEFAULT:
         break;
      case XML.TYPE:
         mDataTypeName = node.getValue();
         break;
      default:
         return false ;
      }
      return true ;
   }

   /* ************************************************************ */

   public void resolve()
   {
      mDataType = mServiceInterface.resolveDataType( mDataTypeName );
   }

   /* ************************************************************ */

   public DataType getDataType()
   {
      return mDataType ;
   }

   /* ************************************************************ */

   public String getUpdateIdName()
   {
      return mServiceInterface.getName() + "_UPD_ID_" + getName();
   }

   /* ************************************************************ */

   public String getVariantTypeDefine( DataType variantType )
   {
      return (variantType.getName() + "_" + getName()).toUpperCase();
   }

   /* ************************************************************ */

   public String getDSIUpdateIdName( boolean addNamespace )
   {
      if( addNamespace )
         return mServiceInterface.getName() + "::" + getDSIUpdateIdName( false );
      else
         return "UPD_ID_" + getName();
   }

   /* ************************************************************ */

   public String getDSIBindingName()
   {
      return "m" + Util.doCapitalLetter( super.getName() ) ;
   }

   /* ************************************************************ */

   public String getNamespaceName()
   {
      return mServiceInterface.getName() + "_" + Util.doCapitalLetter( super.getName() );
   }

   /* ************************************************************ */

   private void checkNotify() throws DSIException
   {
      if( null == mNotify )
      {
         throw new DSIException( getName() + ": notification type not set");
      }
      if(!mNotify.matches("OnChange|Always|Partial"))
      {
         throw new DSIException( getName() + ": bad notification type \"" + mNotify + "\". allowed values: OnChange|Always|Partial");
      }
   }

   /* ************************************************************ */

   public boolean notifyAlways() throws DSIException
   {
      checkNotify();
      return mNotify.equals("Always");
   }

   /* ************************************************************ */

   public boolean notifyOnChange() throws DSIException
   {
      checkNotify();
      return mNotify.equals("OnChange");
   }

   /* ************************************************************ */

   public boolean notifyPartial() throws DSIException
   {
      checkNotify();
      boolean retval = mNotify.equals("Partial") ;

      if(retval && !mDataType.isVector())
      {
         throw new DSIException( getName() + ": Partial notification only allowed for vector attributes");
      }

      return retval ;
   }

   /* ************************************************************ */

   public String getDefaultValue()
   {
      return (null != mDefaultValue && 0 != mDefaultValue.trim().length()) ? mDefaultValue : null ;
   }

   /* ************************************************************ */

   public boolean isFirst()
   {
      return mFirst ;
   }

   /* ************************************************************ */
}
