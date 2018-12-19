/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;

import java.util.Hashtable;
import java.util.Map;
import org.jdom.Element;

public class XML
{
   public static final int UNKNOWN = -1 ;
   public static final int DSI = 0 ;
   public static final int NAME = 1 ;
   public static final int ID = 2 ;
   public static final int EXTERN = 3 ;
   public static final int VERSION = 4 ;
   public static final int DATATYPES = 5 ;
   public static final int METHODS = 6 ;
   public static final int ATTRIBUTES = 7 ;
   public static final int MAJOR = 8 ;
   public static final int MINOR = 9 ;
   public static final int DATATYPE = 10 ;
   public static final int KIND = 11 ;
   public static final int CONTAINER = 16 ;
   public static final int FIELDS = 17 ;
   public static final int FIELD = 18 ;
   public static final int BASETYPE = 19 ;
   public static final int DEFAULTVALUE = 20 ;
   public static final int TYPE = 21 ;
   public static final int METHOD = 22 ;
   public static final int REQUEST = 23 ;
   public static final int PARAMETER = 24 ;
   public static final int PARAMETERS = 25 ;
   public static final int DESCRIPTION = 26 ;
   public static final int ISDEFAULT = 27 ;
   public static final int ATTRIBUTE = 28 ;
   public static final int NOTIFY = 29 ;
   public static final int ALWAYS = 30 ;
   public static final int ONCHANGE = 31 ;
   public static final int NAMESPACE = 32 ;
   public static final int INCLUDE = 33 ;
   public static final int RESPONSE = 34 ;
   public static final int INCLUDES = 36 ;
   public static final int ENUMS = 37 ;
   public static final int ENUM = 38 ;
   public static final int ENUMIDS = 39 ;
   public static final int ENUMID = 40 ;
   public static final int VALUE = 41 ;
   public static final int DEPRECATED = 42 ;
   public static final int HINT = 43 ;
   public static final int BASEINTERFACE = 44 ;
   public static final int PATH = 45 ;
   public static final int ABSTRACT = 46 ;
   public static final int CONSTANTS = 47 ;
   public static final int CONSTANT = 48 ;

   private static final Map<String, Integer> MappingTable = new Hashtable<String, Integer>();

   static
   {
      MappingTable.put( "DSI", DSI );
      MappingTable.put( "Name", NAME );
      MappingTable.put( "ID", ID );
      MappingTable.put( "Extern", EXTERN );
      MappingTable.put( "Version", VERSION );
      MappingTable.put( "DataTypes", DATATYPES );
      MappingTable.put( "Methods", METHODS );
      MappingTable.put( "Attributes", ATTRIBUTES );
      MappingTable.put( "Major", MAJOR );
      MappingTable.put( "Minor", MINOR );
      MappingTable.put( "DataType", DATATYPE );
      MappingTable.put( "Kind", KIND );
      MappingTable.put( "Container", CONTAINER );
      MappingTable.put( "Fields", FIELDS );
      MappingTable.put( "Field", FIELD );
      MappingTable.put( "BaseType", BASETYPE );
      MappingTable.put( "DefaultValue", DEFAULTVALUE );
      MappingTable.put( "Type", TYPE );
      MappingTable.put( "Method", METHOD );
      MappingTable.put( "Request", REQUEST );
      MappingTable.put( "Parameters", PARAMETERS );
      MappingTable.put( "Parameter", PARAMETER );
      MappingTable.put( "Description", DESCRIPTION );
      MappingTable.put( "IsDefault", ISDEFAULT );
      MappingTable.put( "Attribute", ATTRIBUTE );
      MappingTable.put( "Notify", NOTIFY );
      MappingTable.put( "Always", ALWAYS );
      MappingTable.put( "OnChange", ONCHANGE );
      MappingTable.put( "Namespace", NAMESPACE );
      MappingTable.put( "Include", INCLUDE );
      MappingTable.put( "Response", RESPONSE );
      MappingTable.put( "Includes", INCLUDES );
      MappingTable.put( "Enums", ENUMS );
      MappingTable.put( "Enum", ENUM );
      MappingTable.put( "EnumIDs", ENUMIDS );
      MappingTable.put( "EnumID", ENUMID );
      MappingTable.put( "Value", VALUE );
      MappingTable.put( "Deprecated", DEPRECATED );
      MappingTable.put( "Hint", HINT );
      MappingTable.put( "BaseInterface", BASEINTERFACE );
      MappingTable.put( "Path", PATH );
      MappingTable.put( "Abstract", ABSTRACT );
      MappingTable.put( "Constants", CONSTANTS );
      MappingTable.put( "Constant", CONSTANT );
   }

   public static int getID( String value )
   {
      //System.out.println( value );
      Integer id = MappingTable.get( value );
      return null != id ? id.intValue() : UNKNOWN ;
   }

   public static int getID( Element element )
   {
      return getID( element.getName() );
   }
}
