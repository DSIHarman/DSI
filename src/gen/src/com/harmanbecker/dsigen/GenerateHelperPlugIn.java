/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;


public class GenerateHelperPlugIn extends GenerateHelperDSI
{
   /* ************************************************************ */

   public GenerateHelperPlugIn( ServiceInterface si )
   {
      super( si );
   }

   /* ************************************************************ */

   public String[] generateDecode( String name, DataType dataType )
   {
      if( dataType.isSimpleTypedef() )
      {
         dataType = dataType.getBaseTypeR();
      }

      if( dataType.isBuffer() )
      {
         addCode( "// Buffer not Supported" );
      }
      else
      {
         addCode( "if( var.isDefined() )" );
         addCode( "{" );

         if( dataType.isInteger() || dataType.isEnumeration() )
         {
            addCode( "   int ivalue = 0 ;" );
            addCode( "   if( !var.getInteger( ivalue ) )" );
            addCode( "   {" );
            addCode( "      CFlLogger::err(\"ERROR reading data \\\"" + dataType.getDSIBindingName( true ) + " " +  name + "\\\"\");" );
            addCode( "   }" );
            addCode( "   else" );
            addCode( "   {" );
            addCode( "      CFlLogger::msg( 4, \"" + name + " = %d\", ivalue);" );
            addCode( "      " + name + " = (" +  dataType.getDSIBindingName( true ) + ")ivalue ;" );
            addCode( "   }" );
         }
         else if( dataType.isFloat() )
         {
            addCode( "   double dvalue = 0.0 ;" );
            addCode( "   if( !var.getDouble( dvalue ) )" );
            addCode( "   {" );
            addCode( "      CFlLogger::err(\"ERROR reading data \\\"" + dataType.getDSIBindingName( true ) + " " +  name + "\\\"\");" );
            addCode( "   }" );
            addCode( "   else" );
            addCode( "   {" );
            addCode( "      CFlLogger::msg( 4, \"" + name + " = %f\", dvalue);" );
            addCode( "      " + name + " = (" +  dataType.getDSIBindingName( true ) + ")dvalue ;" );
            addCode( "   }" );
         }
         else if( dataType.isString() )
         {
            addCode( "   CFlString str( mContext, var );" );
            addCode( "   if( str.isValid() )" );
            addCode( "   {" );
            addCode( "      CFlLogger::msg( 4, \"" + name + " = %s\", (char*)str);" );
            addCode( "      " + name + " = str ;" );
            addCode( "   }" );
         }
         else if( dataType.isBool() )
         {
            addCode( "   if( !var.getBool( " + name + " ) )" );
            addCode( "   {" );
            addCode( "      CFlLogger::err(\"ERROR reading data \\\"" +  dataType.getType() + " " +  name + "\\\"\");" );
            addCode( "   }" );
            addCode( "   CFlLogger::msg( 4, \"" + name + " = %s\", (" + name + " ? \"true\" : \"false\"));" );
         }
         else
         {
            addCode( "   CFlObject obj( mContext, var );" );
            addCode( "   if( obj.isValid() )" );
            addCode( "   {" );
            addCode( "      decode_" +  dataType.getName() + "( " + name + ", obj );" );
            addCode( "   }" );
         }
         addCode( "}" );
      }

      return prepareCode() ;
   }

   /* ************************************************************ */

   public String[] generateEncode( String name, DataType dataType )
   {
      if( dataType.isSimpleTypedef() )
      {
         dataType = dataType.getBaseTypeR();
      }

      if( dataType.isBuffer() )
      {
         addCode( "// Buffer not Supported" );
         addCode( "var.set( FLQNX_VAR_TYPE_UNDEFINED );" );
         addCode( "(void)" + name + ";" ) ;
      }
      else if( dataType.isInteger() || dataType.isEnumeration() )
      {
         addCode( "var.set( (" + ServiceInterface.getBuildInType("Int32").getType() + ")" + name + " );" );
      }
      else if( dataType.isString() )
      {
     	   addCode( "var.set( " + name + " );" );
      }
      else if( dataType.isBool() )
      {
         addCode( "var.set( 0 != " + name + " );" );
      }
      else if( dataType.isFloat() )
      {
         addCode( "var.set( (double)" + name + " );" );
      }
      else
      {
         addCode( "CFlObject obj( mContext, true );" );
         addCode( "encode_" + dataType.getName() + "( obj, " + name + " );" );
         addCode( "var.set( obj );" );
      }

      return prepareCode() ;
   }

   /* ************************************************************ */

   public boolean hasRequest( Method method )
   {
      Method[] requestMethods = mServiceInterface.getRequestMethods();
      for( Method m : requestMethods )
      {
         if( m.hasResponse() && m.getResponse().equalsIgnoreCase(method.getPlainName()))
         {
            return true ;
         }
      }
      return false ;
   }

   /* ************************************************************ */
}
