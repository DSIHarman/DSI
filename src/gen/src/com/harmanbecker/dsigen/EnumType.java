/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;

import java.util.List;
import java.util.Vector;

import org.jdom.Element;

@SuppressWarnings("unchecked")
public class EnumType extends DataType
{
   protected Vector<EnumID> mEnumIDs = new Vector<EnumID>();

   /* ************************************************************ */

   public EnumType( Element node, ServiceInterface si ) throws Exception
   {
      super( si );
      readElements( node );
   }

   /* ************************************************************ */

   protected boolean readElement( Element node ) throws Exception
   {
      switch( XML.getID( node ) )
      {
      case XML.ENUMIDS:
         readEnumIDs(node);
         break;
      default:
         return super.readElement( node ) ;
      }
      return true ;
   }

   /* ************************************************************ */

   private void readEnumIDs( Element node ) throws Exception
   {
      int nextValue = 0 ;
      List<Element> children = node.getChildren();
      for( Element child : children )
      {
         if( XML.ENUMID == XML.getID( child ) )
         {
            EnumID enumID = new EnumID( child, mServiceInterface );
            nextValue = enumID.resolveValue( nextValue ) + 1 ;
            mEnumIDs.add( enumID );
         }
      }
   }

   /* ************************************************************ */

   public EnumID[] getEnumIDs()
   {
      return mEnumIDs.toArray( new EnumID[mEnumIDs.size()]);
   }

   /* ************************************************************ */

   public String getType()
   {
      return getName();
   }

   /* ************************************************************ */

   public void merge( EnumType e )
   {
      Vector<EnumID> enumIDs = new Vector<EnumID>( e.mEnumIDs ) ;
      enumIDs.addAll( mEnumIDs ) ;
      mEnumIDs = enumIDs ;
   }

   /* ************************************************************ */

   public boolean equals(Object obj)
   {
      return ((EnumType)obj).mName.equals(mName);
   }

   /* ************************************************************ */

   public EnumID lookupEnumID( String name )
   {
      for( EnumID id : mEnumIDs )
      {
         if(id.getName().equals(name))
         {
            return id ;
         }
      }
      return null ;
   }

   /* ************************************************************ */
}
