/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;

import java.io.File;
import java.io.IOException;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.List;
import java.util.Vector;

import org.jdom.Element;
import org.jdom.input.SAXBuilder;

import com.harmanbecker.generate.GenerateException;


@SuppressWarnings( "unchecked" )
public class ServiceInterface extends BaseObject
{
   /* ************************************************************ */

   private class Version
   {
      public int major = 0 ;
      public int minor = 0 ;

      public boolean versionCheck( Version rhs )
      {
         return major == rhs.major && minor <= rhs.minor ;
      }

      public String toString()
      {
         return major + "." + minor ;
      }
   }
   private Version mVersion = new Version();


   public static  class BaseInterface
   {
      public File file ;
      public String path ;
      public String name ;
   }
   private Vector<BaseInterface> mBaseInterfaces = new Vector<BaseInterface>() ;

   private String mIncludeName ;
   private File mFile ;
   private Vector<DataType> mDataTypes = new Vector<DataType>() ;
   private Vector<Constant> mConstants = new Vector<Constant>() ;
   private Vector<EnumType> mEnumerations = new Vector<EnumType>() ;
   private Vector<Method> mRequestMethods = new Vector<Method>() ;
   private Vector<Method> mResponseMethods = new Vector<Method>() ;
   private Vector<Value> mAttributes = new Vector<Value>() ;

   private static Hashtable<String, ServiceInterface> sIncludes = new Hashtable<String, ServiceInterface>() ;
   private static ServiceInterface sMainInterface = null ;

   /* ************************************************************ */

   // build in data types
   private static final Vector<DataType> sBuildInTypes = new Vector<DataType>() ;
   static
   {
      try
      {
         sBuildInTypes.add( new DataType( "Int64"     , "int64_t"     , "64"        , "Number" ) );
         sBuildInTypes.add( new DataType( "UInt64"    , "int64_t"     , "64"        , "Number" ) );
         sBuildInTypes.add( new DataType( "Int32"     , "int32_t"     , "32"        , "Number" ) );
         sBuildInTypes.add( new DataType( "UInt32"    , "uint32_t"    , "32"        , "Number" ) );
         sBuildInTypes.add( new DataType( "Int16"     , "int16_t"     , "16"        , "Number" ) );
         sBuildInTypes.add( new DataType( "UInt16"    , "uint16_t"    , "16"        , "Number" ) );
         sBuildInTypes.add( new DataType( "Int8"      , "int8_t"      , "8"         , "Number" ) );
         sBuildInTypes.add( new DataType( "UInt8"     , "uint8_t"     , "8"         , "Number" ) );
         sBuildInTypes.add( new DataType( "Boolean"   , "bool"        , "Boolean"   , "Boolean") );
         sBuildInTypes.add( new DataType( "Float"     , "float"       , "Float"     , "Number" ) );
         sBuildInTypes.add( new DataType( "Double"    , "double"      , "Double"    , "Number" ) );
         sBuildInTypes.add( new DataType( "String"    , "char*"       , "String"    , "String" ) );
         sBuildInTypes.add( new DataType( "Buffer"    , "DSIBuffer"   , "Buffer"    , "Object" ) );
      }
      catch( Exception ex )
      {
      }
   }

   /* ************************************************************ */

   public ServiceInterface( String hbsiFilename, boolean allowAbstract ) throws Exception
   {
      hbsiFilename = hbsiFilename.replace('\\', '/');
      Debug.message("   Loading File" );
      Debug.message("      "  + hbsiFilename );

      mFile = SearchPath.find( hbsiFilename );

      Debug.push( hbsiFilename );

      mIncludeName = hbsiFilename.replaceAll("\\.[^\\.]+$", "");
      mName = Util.getBaseName( hbsiFilename ) ;

      if( null == sMainInterface )
      {
         sMainInterface = this ;
      }
      sIncludes.put( mName, this );

      Element root = new SAXBuilder().build( mFile ).getRootElement() ;

      // check for extern flag
      if( mFile.getName().toLowerCase().endsWith("hbsi"))
      {
         if( !Util.checkFlag( root, XML.EXTERN ))
         {
            Debug.error( "DSI Interface must be extern", DSIException.ERR_NO_DSI_INTERFACE );
         }
      }

      if( !allowAbstract && Util.checkFlag( root, XML.ABSTRACT ))
      {
         Debug.error("cannot generate abstract interface or type definition");
      }

      // first we need to read the Enumerations
      List<Element> children = root.getChildren();
      for( Element child : children )
      {
         switch(XML.getID( child ))
         {
         case XML.ENUMS:
            {
               readEnums( child );
            }
            break;
         }
      }
      readElements( root );

      Debug.pop();
   }

   /* ************************************************************ */

   protected boolean readElement( Element node ) throws Exception
   {
      switch( XML.getID( node ) )
      {
      case XML.EXTERN:
         break;
      case XML.ABSTRACT:
         break;
      case XML.VERSION:
         mVersion = readVersion( node );
         break;
      case XML.DATATYPES:
         readDatatypes( node );
         break;
      case XML.ENUMS:
         // enumerations already read
         break;
      case XML.CONSTANTS:
         readConstants( node );
         break;
      case XML.METHODS:
         readMethods( node );
         break;
      case XML.ATTRIBUTES:
         readAttributes( node );
         break;
      case XML.INCLUDES:
         readIncludes( node );
         break;
      case XML.BASEINTERFACE:
         readBaseInterface( node );
         break;
      default:
         return false ;
      }
      return true ;
   }

   /* ************************************************************ */

   public String getFullPath()
   {
      try
      {
         return mFile.getCanonicalPath();
      }
      catch( IOException ex )
      {
         return getName();
      }
   }

   /* ************************************************************ */

   private Version readVersion( Element node )
   {
      Version version = new Version();
      List<Element> children = node.getChildren();
      for( Element child : children )
      {
         switch( XML.getID( child ))
         {
         case XML.MAJOR:
            version.major = Integer.parseInt( child.getValue() );
            break;
         case XML.MINOR:
            version.minor = Integer.parseInt( child.getValue() );
            break;
         default:
            Debug.warning( "Unknown XML tag: " + node.getName() + " / " + child.getName() );
            break;
         }
      }
      return version ;
   }

   /* ************************************************************ */

   private void readDatatypes( Element node ) throws Exception
   {
      List<Element> children = node.getChildren();
      for( Element child : children )
      {
         if( XML.DATATYPE == XML.getID( child ) )
         {
            DataType dataType = new DataType( child, this );
            mDataTypes.add( dataType );
         }
         else
         {
            Debug.warning( "Unknown XML tag: " + node.getName() + " / " + child.getName() );
         }
      }

      for( DataType dataType : mDataTypes )
      {
         dataType.resolve();
      }
   }

   /* ************************************************************ */

   private void readEnums( Element node ) throws Exception
   {
      List<Element> children = node.getChildren();
      for( Element child : children )
      {
         if( XML.ENUM == XML.getID( child ) )
         {
            EnumType e = new EnumType( child, this );
            int index = mEnumerations.indexOf( e );
            if( -1 != index )
            {
               mEnumerations.get( index ).merge( e );
            }
            else
            {
               mDataTypes.add( e );
               mEnumerations.add( e );
            }
         }
         else
         {
            Debug.warning( "Unknown XML tag: " + node.getName() + " / " + child.getName() );
         }
      }
   }

   /* ************************************************************ */

   private void readConstants( Element node ) throws Exception
   {
      List<Element> children = node.getChildren();
      for( Element child : children )
      {
         if( XML.CONSTANT == XML.getID( child ) )
         {
            mConstants.add(new Constant( child, this ));
         }
         else
         {
            Debug.warning( "Unknown XML tag: " + node.getName() + " / " + child.getName() );
         }
      }
   }

   /* ************************************************************ */

   private void readMethods( Element node ) throws Exception
   {
      List<Element> children = node.getChildren();
      for( Element child : children )
      {
         if( XML.METHOD == XML.getID( child) )
         {
            Method method = new Method( child, this );

            if( method.isRequest() )
            {
               // check for duplicate id in hbsi file.
               if(mRequestMethods.contains( method ))
               {
                  throw new GenerateException("Duplicate Request ID " + method.getID() + " in " + mFile.getCanonicalPath(), null);
               }
               mRequestMethods.add( method );
            }
            else
            {
               // check for duplicate id in hbsi file.
               if(mResponseMethods.contains( method ))
               {
                  throw new GenerateException("Duplicate Response ID " + method.getID() + " in " + mFile.getCanonicalPath(), null);
               }
               mResponseMethods.add( method );
            }
         }
         else
         {
            Debug.warning( "Unknown XML tag: " + node.getName() + " / " + child.getName() );
         }
      }
      Collections.sort( mRequestMethods );
      Collections.sort( mResponseMethods );
   }

   /* ************************************************************ */

   private void readAttributes( Element node ) throws Exception
   {
      List<Element> children = node.getChildren();
      for( Element child : children )
      {
         if( XML.ATTRIBUTE == XML.getID( child) )
         {
            Value attribute = new Value( child, this );

            // check for duplicate id in hbsi file.
            if(mAttributes.contains( attribute ))
            {
               throw new GenerateException("Duplicate Attribute ID " + attribute.getID() + " in " + mFile.getCanonicalPath(), null);
            }

            attribute.resolve();
            mAttributes.add( attribute );
         }
         else
         {
            Debug.warning( "Unknown XML tag: " + node.getName() + " / " + child.getName() );
         }
      }

      Collections.sort( mAttributes );
   }

   /* ************************************************************ */

   private void readIncludes( Element node ) throws Exception
   {
      List<Element> children = node.getChildren();
      for( Element child : children )
      {
         if( XML.INCLUDE == XML.getID( child) )
         {
            readInclude( child );
         }
         else
         {
            Debug.warning( "Unknown XML tag: " + node.getName() + " / " + child.getName() );
         }
      }

      Collections.sort( mAttributes );
   }

   /* ************************************************************ */

   private void readInclude( Element node ) throws Exception
   {
      Version version = new Version();
      ServiceInterface si = null ;
      List<Element> children = node.getChildren();
      for( Element child : children )
      {
         switch( XML.getID( child ))
         {
         case XML.NAME:
            si = new ServiceInterface( child.getValue(), true );
            break;
         case XML.MAJOR:
            version.major = Integer.parseInt( child.getValue() );
            break;
         case XML.MINOR:
            version.minor = Integer.parseInt( child.getValue() );
            break;
         default:
            Debug.warning( "Unknown XML tag: " + node.getName() + " / " + child.getName() );
            break;
         }
      }

      if( null != si && !version.versionCheck(si.mVersion))
      {
         throw new GenerateException( si.getName() + ": bad include version " + si.mVersion + ", expected:" + version, null);
      }
   }

   /* ************************************************************ */

   private void readBaseInterface( Element node ) throws Exception
   {
      // keep the original version
      Version expectedBaseVersion = null ;
      Version realBaseVersion = null ;
      List<Element> children = node.getChildren();
      for( Element child : children )
      {
         switch( XML.getID( child ))
         {
         case XML.PATH:
            realBaseVersion = mergeBaseInterface( child.getValue() );
            break;
         case XML.VERSION:
            expectedBaseVersion = readVersion( child );
            break;
         case XML.MINOR:
            break;
         default:
            Debug.warning( "Unknown XML tag: " + node.getName() + " / " + child.getName() );
            break;
         }
      }
      if( null != expectedBaseVersion && null != realBaseVersion && !realBaseVersion.versionCheck(expectedBaseVersion))
      {
         // after mergeBaseInterface mVersion is the version of the base interface
         Debug.error( "bad base interface version " + realBaseVersion + ", expected " + expectedBaseVersion );
      }

   }

   /* ************************************************************ */

   private Version mergeBaseInterface( String path ) throws Exception
   {
      Version version = mVersion ;
      File file = SearchPath.find( path );

      Debug.push( path );

      BaseInterface bi = new BaseInterface();
      bi.file = file ;
      bi.name = Util.getBaseName( path );
      bi.path = Util.getPath( path );
      mBaseInterfaces.add( bi );

      Element root = new SAXBuilder().build( file ).getRootElement() ;

      // check for extern flag
      if( file.getName().toLowerCase().endsWith("hbsi") && !Util.checkFlag( root, XML.EXTERN ))
      {
         Debug.error( "DSI Interface must be extern", DSIException.ERR_NO_DSI_INTERFACE );
      }

      if(!Util.checkFlag( root, XML.ABSTRACT ))
      {
         Debug.error("base interface must be abstract");
      }

      // first we need to read the Enumerations
      List<Element> children = root.getChildren();
      for( Element child : children )
      {
         if( XML.ENUMS == XML.getID( child ) )
         {
            readEnums( child );
         }
      }
      readElements( root );

      Debug.pop();

      // restore original version
      Version baseVersion = mVersion ;
      mVersion = version ;
      return baseVersion ;
   }

   /* ************************************************************ */

   public boolean hasRequestWithResponse()
   {
      for( Method m : mRequestMethods )
      {
         if(m.hasResponse())
         {
            return true;
         }
      }
      return false ;
   }

   /* ************************************************************ */

   public Method[] getRequestMethods()
   {
      return mRequestMethods.toArray( new Method[mRequestMethods.size()] );
   }

   /* ************************************************************ */

   public Method[] getResponseMethods()
   {
      return mResponseMethods.toArray( new Method[mResponseMethods.size()] );
   }

   /* ************************************************************ */

   public Method getResponseMethod(String methodname) throws GenerateException
   {
	  // if not found throw exception
	  for ( Method m : mResponseMethods)
	  {
		  if (m.getPlainName().equalsIgnoreCase(methodname))
		  {
			  return m;
		  }
	  }

      throw new GenerateException("response method " + methodname + " not found", null);
   }

   /* ************************************************************ */

   public Method[] getMethods()
   {
      Vector<Method> methods = new Vector<Method>() ;
      methods.addAll(mRequestMethods);
      methods.addAll(mResponseMethods);
      return methods.toArray( new Method[methods.size()] );
   }

   /* ************************************************************ */

   public Value[] getAttributes()
   {
      return mAttributes.toArray( new Value[mAttributes.size()] );
   }

   /* ************************************************************ */

   public DataType[] getDataTypes()
   {
      return mDataTypes.toArray( new DataType[mDataTypes.size()] );
   }

   public static DataType getBuildInType(String dsiTypeName)
   {
	   for (DataType dt : sBuildInTypes )
	   {
	      if (dt.mName == dsiTypeName)
	      {
	    	  return dt;
	      }
	   }
      return null;
   }


   /* ************************************************************ */

   public DataType[] getDistinctDataTypes()
   {
      Vector<DataType> dataTypes = new Vector<DataType>();

      NEXT_DATATYPE:
      for ( DataType dt : mDataTypes )
      {
         for ( DataType dt2 : dataTypes )
         {
            if(dt2.isEqualDataType(dt))
            {
               continue NEXT_DATATYPE;
            }
         }
         dataTypes.add(dt);
      }
      return dataTypes.toArray( new DataType[dataTypes.size()] );
   }

   /* ************************************************************ */

   public static DataType[] getAllDataTypes()
   {
      Vector<DataType[]> dataTypes = new Vector<DataType[]>();
      DataType[] dt ;
      int count = 0 ;

      Enumeration<ServiceInterface> e = sIncludes.elements() ;
      while( e.hasMoreElements() )
      {
         dt = e.nextElement().getDataTypes() ;
         count += dt.length ;
         dataTypes.add( dt );
      }

      dt = new DataType[count];

      count = 0 ;
      for ( DataType[] t : dataTypes )
      {
         System.arraycopy(t, 0, dt, count, t.length);
         count += t.length ;
      }

      return dt ;
   }

   /* ************************************************************ */

   public EnumType[] getEnumerations()
   {
      return mEnumerations.toArray( new EnumType[mEnumerations.size()] );
   }

   /* ************************************************************ */

   public static String[] getIncludes()
   {
      String[] includes = new String[sIncludes.size()-1];
      int index = 0 ;
      ServiceInterface s ;
      Enumeration<ServiceInterface> e = sIncludes.elements() ;
      while( e.hasMoreElements() )
      {
         s = e.nextElement();
         if( sMainInterface != s )
         {
            includes[index++] = s.getInclude();
         }
      }
      return includes ;
   }

   /* ************************************************************ */

   public static String[] getDSIIncludes()
   {
	      String[] includes = new String[sIncludes.size()-1];
	      int index = 0 ;
	      ServiceInterface s ;
	      Enumeration<ServiceInterface> e = sIncludes.elements() ;
	      while( e.hasMoreElements() )
	      {
	         s = e.nextElement();
	         if( sMainInterface != s )
	         {
	            includes[index++] = s.getInclude().replaceAll("(\\\\|\\/)?([^\\\\\\/]+)$", "$1DSI$2") + ".hpp" ;
	         }
	      }
	      return includes ;
   }

   /* ************************************************************ */

   public boolean hasErrorEnum()
   {
	   for ( EnumType theEnum : mEnumerations )
	   {
		   if (theEnum.getName().equals("Error"))
			   return true;
	   }
	   return false;
   }

   public boolean hasVector()
   {
      for (DataType dt : getDataTypes())
      {
         if (dt.isVector())
            return true;
      }
      return false;
   }

   public boolean hasMap()
   {
      for (DataType dt : getDataTypes())
      {
         if (dt.isMap())
            return true;
      }
      return false;
   }

   public boolean hasVariant()
   {
      for (DataType dt : getDataTypes())
      {
         if (dt.isVariant())
            return true;
      }
      return false;
   }

   /* ************************************************************ */

   public int getMajorVersion()
   {
      return mVersion.major ;
   }

   /* ************************************************************ */

   public int getMinorVersion()
   {
      return mVersion.minor ;
   }

   /* ************************************************************ */

   public DataType resolveDataType( String name )
   {
      // get rid of the byte stream
      if( name.equals("ByteStream"))
      {
         name = "Buffer" ;
      }

      int nsIndex = name.indexOf("::");
      if( -1 != nsIndex )
      {
         // absolute path with namespace
         String namespace = name.substring(0, nsIndex);
         String dtname = name.substring(nsIndex+2);
         ServiceInterface hbdt = sIncludes.get( namespace );
         if( null != hbdt )
         {
            return hbdt.resolveDataType( dtname ) ;
         }
      }
      else
      {
         Enumeration<DataType> e = mDataTypes.elements();
         while( e.hasMoreElements() )
         {
            DataType datatype = e.nextElement();
            if( datatype.getPlainName().equals( name ) )
            {
               return datatype ;
            }
         }

         // now check for build in types
         e = sBuildInTypes.elements();
         while( e.hasMoreElements() )
         {
            DataType datatype = e.nextElement();
            if( datatype.getPlainName().equals( name ) )
            {
               return datatype ;
            }
         }
      }

      throw new Error("Unknown dataType: " + name);
   }

   /* ************************************************************ */

   public String getInclude()
   {
      return mIncludeName ;
   }

   /* ************************************************************ */

   public BaseInterface[] getBaseInterfaces()
   {
      return mBaseInterfaces.toArray(new BaseInterface[mBaseInterfaces.size()]);
   }

   /* ************************************************************ */

   public Constant[] getConstants()
   {
      return mConstants.toArray( new Constant[mConstants.size()] );
   }

   /* ************************************************************ */

   public boolean hasBaseInterfaces()
   {
      return 0 != mBaseInterfaces.size();
   }

   /* ************************************************************ */

   public void setIncludeName( String include )
   {
      mIncludeName = include ;
   }

   /* ************************************************************ */

   static public long lastModified()
   {
      long modified = 0 ;
      for( ServiceInterface si : sIncludes.values() )
      {
         if( si.mFile.lastModified() > modified )
         {
            modified = si.mFile.lastModified() ;
         }
         for( BaseInterface bi : si.mBaseInterfaces )
         {
            if( bi.file.lastModified() > modified )
            {
               modified = bi.file.lastModified() ;
            }
         }
      }

      return modified ;
   }

   /* ************************************************************ */

   public EnumID lookupEnumeration( String name )
   {
      for ( EnumType theEnum : mEnumerations )
      {
         EnumID enumID = theEnum.lookupEnumID(name);
         if( enumID != null )
         {
            return enumID ;
         }
      }
      return null ;
   }

   /* ************************************************************ */

   public boolean hasPartialUpdateAttribute() throws DSIException
   {
      for( Value attribute : mAttributes )
      {
         if( attribute.notifyPartial() )
            return true ;
      }
      return false ;
   }

   /* ************************************************************ */
}

