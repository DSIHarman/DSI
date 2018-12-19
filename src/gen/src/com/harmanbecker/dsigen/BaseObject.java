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

@SuppressWarnings("unchecked")
abstract public class BaseObject implements Comparable
{
   protected String mName ;
   protected int mID = -1 ;
   protected String mDescription ;
   protected boolean mDeprecated = false ;
   protected String mHint ;

   protected BaseObject()
   {
   }

   protected void readElements( Element parent ) throws Exception
   {
      List<Element> children = parent.getChildren();
      for( Element child : children )
      {
         switch( XML.getID( child ) )
         {
         case XML.NAME:
            if( null == mName )
            {
               mName = child.getValue();
            }
            break;
         case XML.ID:
            if( -1 == mID )
            {
               mID = Integer.parseInt( child.getValue() );
            }
            break;
         case XML.DESCRIPTION:
            if( null == mDescription )
            {
               mDescription = child.getValue() ;
            }
            break;
         case XML.DEPRECATED:
            mDeprecated = true ;
            break;
         case XML.HINT:
            mHint = child.getValue() ;
            break;
         default:
            if(!readElement( child ))
            {
               Debug.warning( "Unknown XML tag: " + parent.getName() + " / " + child.getName() );
            }
            break;
         }
      }
   }

   abstract protected boolean readElement( Element node ) throws Exception ;

   public String getName()
   {
      return mName ;
   }

   public String getCapitalName()
   {
      return Util.doCapitalLetter( mName ) ;
   }

   public int getID()
   {
      return mID ;
   }

   public String getDescription( String delim )
   {
      return getDescription().replaceAll("\r\n?", delim) ;
   }

   public String getDescription()
   {
      String descr = null != mDescription ? mDescription : "DESCRIPTION MISSING" ;
      if( mDeprecated )
      {
         descr += "\r\rDEPRECATED: " + mHint;
      }
      return descr ;
   }

   public String toString()
   {
      return mName ;
   }

   public int compareTo(Object obj)
   {
      return mID < ((BaseObject)obj).mID ? 0 : 1 ;
   }

   public boolean equals(Object obj)
   {
      return mID == ((BaseObject)obj).mID ;
   }
}
