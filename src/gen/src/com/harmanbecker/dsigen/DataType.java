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
import java.util.Vector;

import org.jdom.Element;

@SuppressWarnings("unchecked")
public class DataType extends BaseObject
{
   public static final int TYPE_UNKNOWN   = 0 ;
   public static final int TYPE_INTEGER   = 1 ;
   public static final int TYPE_FLOAT     = 2 ;
   public static final int TYPE_STRING    = 3 ;
   public static final int TYPE_BUFFER    = 4 ;
   public static final int TYPE_VECTOR    = 5 ;
   public static final int TYPE_VARIANT   = 6 ;
   public static final int TYPE_MAP       = 7 ;
   public static final int TYPE_BOOL      = 8 ;

   /* ************************************************************ */

   private String             mAS2Type ;
   private String             mContainer ;
   private String             mBaseTypeName ;
   private DataType           mBaseType ;
   private String             mKind ;
   protected Vector<Value>    mFields = new Vector<Value>();
   protected String           mType ;
   private String             mFuncName ;
   protected ServiceInterface mServiceInterface ;

   /* ************************************************************ */

   public DataType( Element node, ServiceInterface si ) throws Exception
   {
      mServiceInterface = si ;
      readElements( node );
      mFuncName = "UNKNOWN" ;
      if( null == mType )
      {
         mType = mName ;
      }
   }

   /* ************************************************************ */

   public DataType( ServiceInterface si ) throws Exception
   {
      mServiceInterface = si ;
      mKind = mFuncName = "UNKNOWN" ;
   }

   /* ************************************************************ */

   public DataType( String name, String type, String funcName, String as2Type ) throws Exception
   {
      mName = name ;
      mType = type ;
      mFuncName = funcName ;
      mKind = "BuildIn" ;
      mAS2Type = as2Type ;
   }

   /* ************************************************************ */

   protected boolean readElement( Element node ) throws Exception
   {
      switch( XML.getID( node ) )
      {
      case XML.INCLUDE:
         Debug.error( "Including external header files is not allowed in DSI.", DSIException.ERR_NO_DSI_INTERFACE );
      case XML.NAMESPACE:
         break;
      case XML.KIND:
         mKind = node.getValue() ;
         break;
      case XML.TYPE:
         mType = node.getValue() ;
         if( mType.equals("ByteStream") )
         {
            // we make a byte stream to a buffer
            mType = "Buffer" ;
         }
         break;
      case XML.FIELDS:
         readFields( node );
         break;
      case XML.CONTAINER:
         mContainer = node.getValue() ;
         break;
      case XML.BASETYPE:
         mBaseTypeName = node.getValue() ;
         break;
      default:
         return false ;
      }
      return true ;
   }

   /* ************************************************************ */

   private void readFields( Element node ) throws Exception
   {
      List<Element> children = node.getChildren();
      for( Element child : children )
      {
         if( XML.FIELD == XML.getID( child ) )
         {
            Value field = new Value( child, mServiceInterface );
            mFields.add( field );
         }
      }
   }

   /* ************************************************************ */

   public void resolve()
   {
      if( null != mBaseTypeName )
      {
         mBaseType = mServiceInterface.resolveDataType( mBaseTypeName ) ;
      }
      for( Value field : mFields )
      {
         field.resolve() ;
      }
   }

   /* ************************************************************ */

   public boolean isFloat()
   {
      return TYPE_FLOAT == getTypeId() ;
   }

   /* ************************************************************ */

   public boolean isString()
   {
      return TYPE_STRING == getTypeId() ;
   }

   /* ************************************************************ */

   public boolean isInteger()
   {
      return TYPE_INTEGER == getTypeId() ;
   }

   /* ************************************************************ */

   public boolean isBuffer()
   {
      return TYPE_BUFFER == getTypeId() ;
   }

   /* ************************************************************ */

   public boolean isBool()
   {
      return TYPE_BOOL == getTypeId() ;
   }

   /* ************************************************************ */

   public boolean isVector()
   {
      return TYPE_VECTOR == getTypeId() ;
   }

   /* ************************************************************ */

   public boolean isVariant()
   {
      return TYPE_VARIANT == getTypeId() ;
   }

   /* ************************************************************ */

   public boolean isMap()
   {
      return TYPE_MAP == getTypeId() ;
   }

   /* ************************************************************ */

   public boolean isSimpleTypedef()
   {
      return isTypedef() && (null == mContainer || 0 == mContainer.length());
   }

   /* ************************************************************ */

   public boolean isTypedef()
   {
      return null != mKind && mKind.equalsIgnoreCase("Typedef");
   }

   /* ************************************************************ */

   public boolean isStructure()
   {
      return null != mKind && mKind.equalsIgnoreCase("Structure");
   }

   /* ************************************************************ */

   /**
    * @return for a Vector getBaseType() returns the base type
    */
   public DataType getBaseType()
   {
      return mBaseType ;
   }

   public DataType getBaseTypeR()
   {
      return null != mBaseType ? mBaseType.getBaseTypeR() : this ;
   }

   /* ************************************************************ */

   private class Counter
   {
	   public int value = 0 ;
   }
   public boolean useMemcpy()
   {
	   /* The counter counts the number of pod data types. */
	   Counter c = new Counter() ;
	   /* only use memcpy if there are more than 4 pod data types */
	   return useMemcpy( c ) && c.value > 4 ;
   }

   /* ************************************************************ */

   private boolean useMemcpy( Counter podCounter )
   {
	   if( isStructure() )
	   {
		   boolean retval = true ;
		   Enumeration<Value> e = mFields.elements();
	       while( retval && e.hasMoreElements() )
	       {
	          retval = e.nextElement().getDataType().useMemcpy( podCounter );
	       }
	       return retval ;
	   }
	   else
	   {
		   podCounter.value++;
		   return isBool() || (isInteger() && !mName.matches("int64_t|uint64_t")) || isFloat() ;
	   }
   }

   /* ************************************************************ */

   /**
    * @ return true if the DataType is a complex type
    */
   public boolean isComplex()
   {
      if( isSimpleTypedef() )
      {
         return getBaseType().isComplex();
      }

      return isStructure() || isVariant() || isString() || isBuffer() || isVector() || isMap() ;
   }

   /* ************************************************************ */

   /*
    *  checks if the data type has data that was allocated on the heap
    */
   public boolean hasComplexData()
   {
      if( isSimpleTypedef() )
      {
         return getBaseType().hasComplexData();
      }

      boolean retval = false ;
      if( isStructure() || isVariant() )
      {
         Enumeration<Value> e = mFields.elements();
         while( !retval && e.hasMoreElements() )
         {
            Value field = e.nextElement();
            retval = field.getDataType().hasComplexData();
         }
      }
      else
      {
         retval = isComplex();
      }
      return retval ;
   }

   /* ************************************************************ */

   public Value[] getFields()
   {
      return mFields.toArray( new Value[mFields.size()]);
   }

   /* ************************************************************ */

   public String getType()
   {
      return mType;
   }

   /* ************************************************************ */

   public String getName()
   {
      return (isStructure() || isTypedef()) ? mServiceInterface.getName() + "_" + super.getName() : super.getName() ;
   }

   /* ************************************************************ */

   public String getDestructor( boolean addNamespace )
   {
      if( isSimpleTypedef() )
      {
         return getBaseType().getDestructor( addNamespace );
      }
      return "~" + getDSIBindingName( addNamespace ) + "()";
   }

   /* ************************************************************ */

   public String getDSIBindingName( ServiceInterface scope, boolean useVectorTypedef )
   {
      if( !isVector() || useVectorTypedef )
      {
         return getDSIBindingName( scope != mServiceInterface );
      }

      return "std::vector<" + getBaseType().getDSIBindingName( scope != getBaseType().mServiceInterface ) + "> " ;
   }

   public String getDSIBindingName( ServiceInterface scope )
   {
      return getDSIBindingName( scope != mServiceInterface );
   }

   public String getDSIBindingName()
   {
      return getDSIBindingName( true );
   }

   public String getDSIBindingName( boolean addNamespace )
   {
      String name = "";
      if( isString() )
      {
         name = "std::wstring" ;
      }
      else if( isBuffer() )
      {
         name = "std::string" ;
      }
      else if( isBool() )
      {
         name = "bool" ;
      }
      else
      {
         if( null != mServiceInterface && addNamespace )
         {
            name = mServiceInterface.getName() + "::" ;
         }
         name += getType();
      }
      return name ;
   }

   /* ************************************************************ */

   public String getPlainName()
   {
      return super.getName() ;
   }

   /* ************************************************************ */

   public String getFuncName()
   {
      return mFuncName ;
   }

   /* ************************************************************ */

   public boolean isEnumeration()
   {
      return this instanceof EnumType ;
   }

   /* ************************************************************ */

   public EnumType getEnumeration()
   {
      return (EnumType) this ;
   }


   /* ************************************************************ */

   public String getAS2DataType()
   {
      if( mAS2Type != null )
      {
         return mAS2Type ;
      }
      else if( isVector() )
      {
    	  return null != mServiceInterface ? "com.harmanbecker." + mServiceInterface.getName() + "." + mType : mType ;
      }
      else if( isEnumeration() )
      {
         return "Number" ; //getEnumeration().getName();
      }
      else if( isTypedef() )
      {
         return "Object" ;
      }
      else
      {
         return null != mServiceInterface ? "com.harmanbecker." + mServiceInterface.getName() + "." + mType : mType ;
      }
   }

   /* ************************************************************ */

   public String getAS3DataType()
   {
      if(mName.matches("Int64|Int32|Int16|Int8"))
      {
          return "int";
      }
      else if(mName.matches("UInt64|UInt32|UInt16|UInt8"))
      {
          return "uint";
      }
      else if(mName.matches("Float|Double"))
      {
          return "Number";
      }
      else if(mName.matches("Boolean"))
      {
          return "Boolean";
       }

      if( mAS2Type != null )
      {
         return mAS2Type ;
      }
      else if( isVector() )
      {
         return "Array" ;
      }
      else if( isEnumeration() )
      {
         return "uint" ; //getEnumeration().getName();
      }
      else if( isSimpleTypedef() )
      {
         return getBaseType().getAS3DataType() ;
      }
      else if( isTypedef() )
      {
         return "Object" ;
      }
      else
      {
         return null != mServiceInterface ? "com.harmanbecker." + mServiceInterface.getName() + "." + mType : mType ;
      }
   }

   /* ************************************************************ */

   public boolean isAS2Number()
   {
      return (null != mAS2Type && mAS2Type.equals("Number")) || isEnumeration() ;
   }

   /* ************************************************************ */

   public boolean isAS2Boolean()
   {
      return null != mAS2Type && mAS2Type.equals("Boolean");
   }

   /* ************************************************************ */

   public boolean isAS2String()
   {
      return null != mAS2Type && mAS2Type.equals("String");
   }

   /* ************************************************************ */

   public String getInitializer( String prefix )
   {
      String init = "";
      if( isBool() )
      {
         init = prefix + "false" ;
      }
      else if( isEnumeration() )
      {
         init = prefix + "(" + getDSIBindingName( true ) + ") 0" ;
      }
      else if( isFloat() )
      {
         init = prefix + "0.0" ;
      }
      else if( !isComplex() )
      {
         init = prefix + "0" ;
      }
      return init ;
   }

   /* ************************************************************ */

   public boolean isMapKey()
   {
      for( DataType dt : mServiceInterface.getDataTypes())
      {
         if( dt.isMap() && (this == dt.getFields()[0].getDataType()))
         {
            return true ;
         }
      }
      return false ;
   }

   /* ************************************************************ */

   ServiceInterface getServiceInterface()
   {
      return mServiceInterface ;
   }

   /* ************************************************************ */

   public boolean isPartialUpdate() throws DSIException
   {
      if(!isVector())
      {
         return false ;
      }

      for( Value attribute : mServiceInterface.getAttributes())
      {
         if(attribute.getDataType() == this && attribute.notifyPartial())
         {
            return true ;
         }
      }
      return false ;
   }

   /* ************************************************************ */

   public int getTypeId()
   {
      if(isSimpleTypedef())
         return mBaseType.getTypeId();

      if(mName.matches("Int64|Int32|Int16|Int8|UInt64|UInt32|UInt16|UInt8"))
         return TYPE_INTEGER;
      if(mName.matches("Float|Double"))
         return TYPE_FLOAT;
      if(mName.equals("String"))
         return TYPE_STRING;
      if(mName.equals("Buffer"))
         return TYPE_BUFFER;
      if(mName.equals("Boolean"))
         return TYPE_BOOL;

      if( null != mContainer )
      {
         if(mContainer.equals("Vector"))
            return TYPE_VECTOR;
         if(mContainer.equals("Variant"))
            return TYPE_VARIANT;
         if(mContainer.equals("Map"))
            return TYPE_MAP;
      }

      return TYPE_UNKNOWN ;
   }

   /* ************************************************************ */

   public boolean isEqualDataType( DataType dt )
   {
      if(isSimpleTypedef())
      {
         return mBaseType.isEqualDataType(dt);
      }
      else if(dt.isSimpleTypedef())
      {
         return isEqualDataType(dt.mBaseType);
      }
      else if(getTypeId() != dt.getTypeId())
      {
         return false ;
      }
      else if(isVector())
      {
         return mBaseType.isEqualDataType(dt.mBaseType);
      }
      else if(isVariant() || isMap())
      {
         if(mFields.size() != dt.mFields.size())
         {
            return false ;
         }
         for( int idx=0; idx<mFields.size(); idx++ )
         {
            if(!mFields.get(idx).getDataType().isEqualDataType(dt.mFields.get(idx).getDataType()))
            {
               return false ;
            }
         }
         return true ;
      }
      else
      {
         return mName.equals(dt.mName);
      }
   }

   /* ************************************************************ */
}
