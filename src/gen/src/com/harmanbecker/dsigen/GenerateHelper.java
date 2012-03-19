/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;


public class GenerateHelper extends GenerateHelperBase
{
   /* ************************************************************ */

   public GenerateHelper( ServiceInterface si )
   {
      super( si );
   }

   /* ************************************************************ */

   private String generateCleanupCode2( DataType dataType, String name )
   {
      if( dataType.isSimpleTypedef() )
         dataType = dataType.getBaseTypeR() ;

      if( dataType.isStructure() || dataType.isVector() || dataType.isVariant() || dataType.isMap() )
      {
         return dataType.getName() + "_Free( &" + name + " );" ;
      }
      else if( dataType.isComplex() )
      {
         return "DSI" + dataType.getFuncName() + "Free( " + name + " );" ;
      }
      return null ;
   }

   /* ************************************************************ */

   public String[] generateCleanupCode( DataType dataType, String name )
   {
      return generateCleanupCode( dataType, name, true );
   }

   /* ************************************************************ */

   public String[] generateCleanupCode( DataType dataType, String name, boolean recursive )
   {
      if( dataType.isSimpleTypedef() )
         dataType = dataType.getBaseTypeR() ;

      if( recursive && dataType.isStructure() )
      {
         Value[] fields = dataType.getFields();
         for( Value field : fields )
         {
            DataType fieldType = field.getDataType();
            if( fieldType.hasComplexData() )
            {
               addCode( generateCleanupCode2( fieldType, name + field.getName()));
            }
         }
      }
      else if( recursive && dataType.isVector() )
      {
         if( dataType.getBaseType().hasComplexData() )
         {
            addCode( "{");
            addCode( "   int idx = DSIVectorSize( " + name + " ) ;" );
            addCode( "   while( --idx >= 0 )");
            addCode( "   {");
            DataType baseType = dataType.getBaseType();
            pushIndent( 2 );
            if( baseType.hasComplexData() )
            {
               addCode( generateCleanupCode2( baseType, name + "->data[idx]" ));
            }
            popIndent();
            addCode( "   }" );
            addCode( "}" );
         }
         addCode( "DSIVectorFree( " + name + " );" );
      }
      else
      {
         addCode( generateCleanupCode2( dataType, name ));
      }

      return prepareCode() ;
   }

   /* ************************************************************ */

   public String getStreamingReadFunction( DataType dataType )
   {
      if( dataType.isSimpleTypedef() )
         return getStreamingReadFunction( dataType.getBaseType() );
      else if( dataType instanceof Method )
         return "DSIRead" + ((Method)dataType).getParameterStructName() ;
      else if( dataType.isInteger() || dataType.isFloat() || dataType.isString() || dataType.isBuffer() || dataType.isBool() )
		   return "DSIRead" + dataType.getFuncName() ;
	   else if( dataType.isEnumeration() )
		   return "DSIRead32" ;
	   else
		   return "DSIRead" + dataType.getName() ;
   }

   /* ************************************************************ */

   public String getStreamingWriteFunction( DataType dataType )
   {
      if( dataType.isSimpleTypedef() )
         return getStreamingWriteFunction( dataType.getBaseType() );
      else if( dataType instanceof Method )
         return "DSIWrite" + ((Method)dataType).getParameterStructName() ;
      else if( dataType.isInteger() || dataType.isFloat() || dataType.isString() || dataType.isBuffer() || dataType.isBool() )
         return "DSIWrite" + dataType.getFuncName() ;
      else if( dataType.isEnumeration() )
         return "DSIWrite32" ;
      else
         return "DSIWrite" + dataType.getName() ;
   }

   /* ************************************************************ */

   public String getDecleration( DataType dataType, String name )
   {
      if( dataType.isSimpleTypedef() )
         dataType = dataType.getBaseTypeR() ;

      String decl = getType( dataType ) + " " + name ;
      if( !dataType.isStructure() && !dataType.isVariant() )
         decl += " = 0" ;
      return decl ;
   }

   /* ************************************************************ */

   public String getInitialisation( DataType dataType, String name )
   {
      if( dataType.isSimpleTypedef() )
         dataType = dataType.getBaseTypeR() ;

      if( dataType.isStructure() || dataType.isVariant() )
    	  return "memset( &" + name + ", 0, sizeof(" + name + ") ) " ;
      return null ;
   }

   /* ************************************************************ */

   public String[] generateCopyCode( DataType dataType, String lname, String rname, boolean reference )
   {
      if( dataType.isSimpleTypedef() )
         dataType = dataType.getBaseTypeR() ;

      if( (dataType.isStructure() && dataType.hasComplexData()) || dataType.isVector() || dataType.isVariant() )
      {
         addCode( getType( dataType ) + "_Copy( &" + lname + ", " + (reference || dataType.isVector() ? "&" : "") + rname + " );");
      }
      else if( dataType.isString() )
      {
         addCode( lname + " = strdup(" + rname + ");");
      }
      else
      {
         addCode( lname + " = " + (!reference && dataType.isStructure() ? "*" : "") + rname + " ;");
      }
      return prepareCode() ;
   }

   /* ************************************************************ */

   private void generateLowerCode2( DataType dataType, String lname, String rname )
   {
      if( dataType.isSimpleTypedef() )
         dataType = dataType.getBaseTypeR() ;

      if( dataType.isStructure() )
      {
         Value[] fields = dataType.getFields();
         for( Value field : fields )
         {
            generateLowerCode2( field.getDataType(), lname + "." + field.getName(), rname + "." + field.getName() );
         }
      }
      else if( dataType.isString() )
      {
         addCode( " 0 > strcmp(" + lname + ", " + rname + ")" );
      }
      else
      {
         addCode( lname + " < " + rname );
      }
   }

   /* ************************************************************ */

   public String[] generateLowerCode( DataType dataType, String lname, String rname )
   {
      generateLowerCode2( dataType, lname, rname );
      return prepareCode() ;
   }

   /* ************************************************************ */

   private void generateEqualCode2( DataType dataType, String lname, String rname )
   {
      if( dataType.isSimpleTypedef() )
         dataType = dataType.getBaseTypeR() ;

      if( dataType.isStructure() )
      {
         Value[] fields = dataType.getFields();
         for( Value field : fields )
         {
            generateEqualCode2( field.getDataType(), lname + "." + field.getName(), rname + "." + field.getName() );
         }
      }
      else if( dataType.isString() )
      {
         addCode( " 0 == strcmp(" + lname + ", " + rname + ")" );
      }
      else
      {
         addCode( lname + " == " + rname );
      }
   }

   /* ************************************************************ */

   public String[] generateEqualCode( DataType dataType, String lname, String rname )
   {
      generateEqualCode2( dataType, lname, rname );
      return prepareCode() ;
   }

   /* ************************************************************ */

   public String getParameterCallStringCpp( Value value )
   {
      DataType dataType = value.getDataType() ;

      if( dataType.isSimpleTypedef() )
         dataType = dataType.getBaseTypeR() ;

      return (dataType.isStructure() || dataType.isVariant() ? " *" : "") + value.getName() ;
   }

   /* ************************************************************ */

   public String getParameterCallString( Value value )
   {
      DataType dataType = value.getDataType() ;

      if( dataType.isSimpleTypedef() )
         dataType = dataType.getBaseTypeR() ;

      return (dataType.isStructure() || dataType.isVariant() ? " &" : "") + value.getName() ;
   }

   /* ************************************************************ */

   public String getParameterCallString( Method method )
   {
      StringBuffer sb = new StringBuffer();
      for( Value parameter : method.getParameters() )
      {
         sb.append( ", " + getParameterCallString(parameter)) ;
      }
      return sb.toString();
   }

   /* ************************************************************ */

   public String getParameterCallStringEx( Method method )
   {
      StringBuffer sb = new StringBuffer();
      for( Value parameter : method.getParameters() )
      {
         DataType dt = parameter.getDataType() ;
         if( dt.isVector() )
         {
            if( dt.getBaseType().isString())
            {
               sb.append(", (const char* const*)DSIVectorData( " + parameter.getName() + " ), DSIVectorSize( " + parameter.getName() + " )" );
            }
            else
            {
               sb.append(", DSIVectorData( " + parameter.getName() + " ), DSIVectorSize( " + parameter.getName() + " )" );
            }
         }
         else if( dt.isBuffer() )
         {
            sb.append(", DSIBufferGet( " + parameter.getName() + " ), DSIBufferSize( " + parameter.getName() + " )" ) ;
         }
         else
         {
            sb.append( ", " + getParameterCallString(parameter)) ;
         }
      }
      return sb.toString();
   }

   /* ************************************************************ */

   public String getParameterDeclStringCpp( Value value )
   {
      DataType dataType = value.getDataType() ;

      if( dataType.isSimpleTypedef() )
         dataType = dataType.getBaseTypeR() ;

      return (dataType.isComplex() ? "const " : " ") + getType( dataType ) + (dataType.isStructure() || dataType.isVariant() ? "& " : " ") + value.getName() ;
   }

   /* ************************************************************ */

   public String getParameterDeclString( Value value )
   {
      DataType dataType = value.getDataType() ;

      if( dataType.isSimpleTypedef() )
         dataType = dataType.getBaseTypeR() ;

      return (dataType.isComplex() ? "const " : " ") + getType( dataType ) + (dataType.isStructure() || dataType.isVariant() ? "* " : " ") + value.getName() ;
   }

   /* ************************************************************ */

   public String getParameterDeclString( Method method )
   {
      StringBuffer sb = new StringBuffer();
      for( Value parameter : method.getParameters() )
      {
         sb.append( ", " + getParameterDeclString( parameter ));
      }
      return sb.toString();
   }

   /* ************************************************************ */

   public String getParameterDeclStringEx( Method method )
   {
      StringBuffer sb = new StringBuffer();
      for( Value parameter : method.getParameters() )
      {
         DataType dt = parameter.getDataType();
         if( dt.isVector() )
         {
            if( dt.getBaseType().isString())
            {
               sb.append( ", const char* const* " + parameter.getName()) ;
            }
            else
            {
               sb.append( ", const " + getType( dt.getBaseType() ) + "* " + parameter.getName()) ;
            }
            sb.append( ", int " + parameter.getName() + "Size" );
         }
         else if( dt.isBuffer() )
         {
            sb.append( ", const void* " + parameter.getName() + ", int " + parameter.getName() + "Size" );
         }
         else
         {
            sb.append(", " + getParameterDeclString( parameter ));
         }
      }
      return sb.toString() ;
   }

   /* ************************************************************ */

   public String getType( DataType dataType )
   {
      return getType( dataType, false );
   }

   /* ************************************************************ */

   public String getType( DataType dataType, boolean constString )
   {
      if( dataType instanceof Method )
      {
         return dataType.getType() ;
      }
      else if( constString && dataType.isString() )
      {
         return "const char*" ;
      }
      else
      {
         return ((dataType.isStructure() || dataType.isTypedef()) ? dataType.getServiceInterface().getName() + "_" : "" ) + dataType.getType() ;
      }
   }

   /* ************************************************************ */
}
