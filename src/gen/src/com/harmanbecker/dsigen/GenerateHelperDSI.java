/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

package com.harmanbecker.dsigen;


public class GenerateHelperDSI extends GenerateHelperBase
{
   /* ************************************************************ */

   public GenerateHelperDSI( ServiceInterface si )
   {
      super( si );
   }

   /* ************************************************************ */

   public String[] generateInitCode( DataType dataType )
   {
      Value[] fields = dataType.getFields();
      boolean first = true ;
      for( Value field : fields )
      {
         DataType dt = field.getDataType();
         String defaultValue = field.getDefaultValue();
         if( !dt.isComplex() || null != defaultValue )
         {
            addCode( (first ? ": " : ", ") + field.getName() + "( " + (null != defaultValue ? defaultValue : dt.getInitializer("")) + " )" );
            first = false ;
         }
      }
      return prepareCode() ;
   }

   /* ************************************************************ */

   public String createTypedefDeclaration( DataType dataType )
   {
      StringBuffer sb = new StringBuffer();
      if( dataType.isVariant() )
      {
         sb.append("DSI::TVariant<");
         Value[] fields = dataType.getFields();
         int count = 0;
         for( Value field : fields )
         {
            ++count;
            sb.append( field == fields[0] ? "" : ", " );
            sb.append( "DSI::Private::TTypeList<" );
            sb.append( field.getDataType().getDSIBindingName( mServiceInterface, true ));
         }
         sb.append( ", DSI::Private::SNilType" );

         // all closing brackets of the typelist entries
         for(int i=0; i<count; ++i)
         {
            sb.append("> ");
         }

         // closing bracket of TVariant<...
         sb.append( "> " );
      }
      else if( dataType.isVector() )
      {
         sb.append("std::vector<" + dataType.getBaseType().getDSIBindingName( mServiceInterface, true ) + "> " ) ;
      }
      else if( dataType.isMap() )
      {
         Value fields[] = dataType.getFields();
         sb.append("std::map<" + fields[0].getDataType().getDSIBindingName( mServiceInterface, true ) + ", " + fields[1].getDataType().getDSIBindingName( true ) + " > " ) ;
      }
      else
      {
         sb.append( dataType.getBaseType().getDSIBindingName( mServiceInterface, true ) + " " ) ;
      }
      return sb.toString() ;
   }

   /* ************************************************************ */

   public String createMethodDeclaration( Method method, ServiceInterface scope, String classname, boolean setDefault, String returnType )
   {
      return createMethodDeclaration( method, scope, classname, setDefault, returnType, true, true );
   }

   public String createEmptyMethodDeclaration( Method method, ServiceInterface scope, String classname, boolean setDefault, String returnType )
   {
      return createMethodDeclaration( method, scope, classname, setDefault, returnType, true, false );
   }

   /* ************************************************************ */

   public String createMethodDeclaration( Method method, ServiceInterface scope, String classname, boolean setDefault, String returnType, boolean addRegisterParams )
   {
      return createMethodDeclaration( method, scope, classname, setDefault, returnType, addRegisterParams, true );
   }

   /* ************************************************************ */

   public String createMethodDeclaration( Method method, ServiceInterface scope, String classname, boolean setDefault, String returnType, boolean addRegisterParams, boolean addVariablesNames )
   {
      StringBuffer sb = new StringBuffer( (returnType == null || method.isUnregister()) ? "void " : returnType + " "  );

      if( null != classname )
      {
         sb.append( classname + "::" );
      }

      sb.append( method.getMethodName() );
      sb.append( "(" );

      boolean first = true ;
      if( addRegisterParams )
      {
         if( method.isRegister() )
         {
            sb.append( " const " + mServiceInterface.getName() + "::CUpdateIdVector& ");
            if (addVariablesNames)
            {
               sb.append("updIds");
            }
            else
            {
               sb.append("/*updIds*/");
            }

            first = false ;
         }
         else if( method.isUnregister())
         {
             sb.append( " " + ServiceInterface.getBuildInType("Int32").getType());
             if (addVariablesNames)
             {
                sb.append(" sessionId");
             }
             else
             {
                sb.append(" /*sessionId*/");
             }
             sb.append(", const " + mServiceInterface.getName() + "::CUpdateIdVector& ");
             if (addVariablesNames)
             {
                sb.append("updIds");
             }
             else
             {
                sb.append("/*updIds*/");
             }
             if( setDefault )
             {
                sb.append(" = " + mServiceInterface.getName() + "::CUpdateIdVector()" );
             }
             first = false ;
         }
      }
      else if( method.isUnregister())
      {
         sb.append( " " + ServiceInterface.getBuildInType("Int32").getType());
         if (addVariablesNames)
         {
            sb.append(" sessionID");
         }
         else
         {
            sb.append(" /*sessionID*/");
         }
         first = false ;
      }

      Value[] parameters = method.getParameters();
      for( Value parameter : parameters )
      {
         sb.append( first ? " " : ", " );
         first = false ;
         sb.append( getParameterType( parameter.getDataType(), scope, true, false ) + " ");
         if (addVariablesNames)
         {
            sb.append(parameter.getName());
         }
         else
         {
            sb.append("/*" + parameter.getName() + "*/");
         }
         String defaultValue = parameter.getDefaultValue() ;
         if( setDefault && null != defaultValue )
         {
            sb.append( " = " + defaultValue );
         }
      }

      sb.append( " )" );
      return sb.toString() ;
   }

   /* ************************************************************ */

   private void generateLowerCode2( DataType dataType, String lname, String rname )
   {
      if( dataType.isStructure() )
      {
         Value[] fields = dataType.getFields();
         for( Value field : fields )
         {
            generateLowerCode2( field.getDataType(), lname + "." + field.getName(), rname + "." + field.getName() );
         }
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

   public String getStreamingReadFunction( DataType dataType )
   {
      if( dataType.isSimpleTypedef() )
      {
         return getStreamingReadFunction( dataType.getBaseType() );
      }

	   if( dataType.isInteger() || dataType.isFloat() || dataType.isString() || dataType.isBuffer() || dataType.isBool() )
		   return "DSIRead" ;
	   else
	   {
	      String namespace = "" ;
	      if( dataType.getServiceInterface() != mServiceInterface )
	         namespace = dataType.getServiceInterface() + "::" ;

		   return namespace + "DSIRead" + Util.doCapitalLetter(dataType.getDSIBindingName( false )) ;
	   }
   }

   public String getStreamingWriteFunction( DataType dataType )
   {
      if( dataType.isSimpleTypedef() )
      {
         return getStreamingWriteFunction( dataType.getBaseType() );
      }

      if( dataType.isInteger() || dataType.isFloat() || dataType.isString() || dataType.isBuffer() || dataType.isBool() )
         return "DSIWrite" ;
      else
      {
         String namespace = "" ;
         if( dataType.getServiceInterface() != mServiceInterface )
            namespace = dataType.getServiceInterface() + "::" ;

         return namespace + "DSIWrite" + Util.doCapitalLetter(dataType.getDSIBindingName( false )) ;
      }
   }

   /* ************************************************************ */

   public String getParameterType( DataType dataType, boolean isConst, boolean forceReference )
   {
      return getParameterType( dataType, null, isConst, forceReference );
   }

   public String getParameterType( DataType dataType, ServiceInterface scope, boolean isConst, boolean forceReference )
   {
      String type = "" ;
      if( isConst && dataType.isComplex() )
      {
         type += "const " ;
      }
      if( dataType.isBool() )
      {
         type += "bool" ;
      }
      else
      {
         type += dataType.getDSIBindingName( scope, true ) ;
      }
      if( dataType.isComplex() || forceReference )
      {
         type += " &" ;
      }
      return type ;
   }

   /* ************************************************************ */

   public String getArgumentList( Method method, boolean decl, boolean isConst, boolean forceReference  )
   {
      StringBuffer sb = new StringBuffer();
      Value[] parameters = method.getParameters() ;
      if( method.hasParameters() )
      {
         for( Value parameter : parameters )
         {
            if( parameter != parameters[0] )
               sb.append( ", " ) ;
            if( decl )
               sb.append( getParameterType( parameter.getDataType(), mServiceInterface, isConst, forceReference ) + " " ) ;
            sb.append( parameter.getName() ) ;
         }
      }

      return sb.toString() ;
   }

   /* ************************************************************ */
}

