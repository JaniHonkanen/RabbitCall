#include "pch.h"

CsOutputGenerator::NamespaceWriter::NamespaceWriter(const string &namespaceName) {
	namespaceParts = cppParseUtil->cppNamespaceParser.parseNamespaceParts(namespaceName);
}

void CsOutputGenerator::NamespaceWriter::begin(StringBuilder &output) {
	for (const string &ns : namespaceParts) {
		output.appendIndent() << "namespace " << ns << " {\n";
		output.changeIndent(+1);
	}
}

void CsOutputGenerator::NamespaceWriter::end(StringBuilder &output) {
	for (const string &ns : namespaceParts) {
		output.changeIndent(-1);
		output.appendLine("}");
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CsOutputGenerator::outputFunction(CppFuncVar *func, CppClass *enclosingClassIfNotGlobal, int functionIndex, StringBuilder &output, vector<Callback> &callbacksOut) {
	string entryPoint = partition->getFunctionEntryPoint(func, enclosingClassIfNotGlobal);
	string externFunctionName = sb() << OUTPUT_FUNCTION_NAME_PREFIX << functionIndex;

	bool isNonStaticMember = !func->isStatic && enclosingClassIfNotGlobal;
	bool isExceptionCheckEnabled = !func->isNoexcept;

	output.appendLine("");

	// Find names for the return value ptr and exception ptr that don't clash with parameter names.
	string returnValuePtrName = OUTPUT_RETURN_VALUE_PTR;
	string exceptionPtrName = OUTPUT_EXCEPTION_PTR;
	while (true) {
		bool modified = false;
		for (auto &param : func->functionParameters) {
			if (returnValuePtrName == param->declarationName) {
				returnValuePtrName += '_'; // Prevent clash with function parameter names.
				modified = true;
			}
			if (exceptionPtrName == param->declarationName) {
				exceptionPtrName += '_'; // Prevent clash with function parameter names.
				modified = true;
			}
		}
		if (!modified) break;
	}

	// Create C# delegates for callback functions that are passed as a parameter to the C++ function.
	vector<CppFuncVar *> callbackParameters;
	vector<int64_t> callbackIndicesByParameterIndex;
	vector<string> delegateTypeNamesByParameterIndex; // Contains empty strings for those parameters that are not callback functions.
	vector<string> delegateDeclarationLines;
	for (const shared_ptr<CppFuncVar> &param : func->functionParameters) {
		string delegateTypeName;
		int64_t callbackIndex = -1;
		if (param->isLambdaFunction) {
			callbackParameters.push_back(param.get());

			CppFuncVar returnType = param->getFunctionReturnType();

			// Use C# built-in Action delegate for the callback if it does not have return values or parameters (P/Invoke does not allow generic Action<>/Func<> types).
			if (returnType.isVoid() && param->functionParameters.empty()) {
				delegateTypeName = "Action";
			}
			else {
				delegateTypeName = sb() << "Callback_" << func->declarationName << "_" << param->declarationName;

				StringBuilder delegateParameterList;
				StringJoiner joiner(&delegateParameterList, ", ");
				for (auto &cbParam : param->functionParameters) {
					joiner.append(formatDeclaration(*cbParam, Language::CS, TypePresentation::PUBLIC));
				}
				joiner.finish();
				delegateDeclarationLines.push_back(sb() << "public delegate " << formatDeclaration(returnType, delegateTypeName, Language::CS, TypePresentation::PUBLIC) << "(" << delegateParameterList << ");");
			}

			callbackIndex = callbacksOut.size();
			callbacksOut.emplace_back();
			Callback &callback = callbacksOut.back();
			callback.callback = param.get();
			callback.publicDelegateTypeName = delegateTypeName;
		}
		delegateTypeNamesByParameterIndex.push_back(delegateTypeName);
		callbackIndicesByParameterIndex.push_back(callbackIndex);
	}

	outputComment(func->comment.get(), output);

	{
		output.appendIndent();

		// Output the signature of the wrapper function that is called by application C# code.
		{
			output << "public " << (isNonStaticMember ? "" : "static ") << formatDeclaration(func->getFunctionReturnTypeAndName(), Language::CS, TypePresentation::PUBLIC) << "(";
			StringJoiner joiner(&output, ", ");
			for (int i = 0; i < (int)func->functionParameters.size(); i++) {
				CppFuncVar *param = func->functionParameters.at(i).get();
				string delegateName = delegateTypeNamesByParameterIndex.at(i);
				if (!delegateName.empty()) { // This parameter is a callback function
					joiner.append(sb() << delegateName << " " << param->declarationName);
				}
				else {
					joiner.append(formatDeclaration(*param, Language::CS, TypePresentation::PUBLIC));
				}
			}
			joiner.finish();
			output << ")        /****/ {"; // Leave some space between the function declaration and body to make it easier to read.
		}

		output << OUTPUT_INTERNAL_UTIL_CLASS "." OUTPUT_CHECK_INIT "();";
		
		if (!func->isVoid()) {
			output << formatDeclaration(func->getFunctionReturnTypeAndName(), returnValuePtrName, Language::CS, TypePresentation::TRANSFER_RETURN_VALUE) << ";";
		}

		if (isExceptionCheckEnabled) {
			output << OUTPUT_PTR_AND_SIZE << " " << exceptionPtrName << ";";
		}

		// Output function parameter values for the P/Invoke call (see explanations above).
		{
			output << externFunctionName << "(";
			StringJoiner joiner(&output, ",");

			if (isNonStaticMember) {
				joiner.append(OUTPUT_THIS_PTR);
			}

			for (int64_t i = 0; i < (int64_t)func->functionParameters.size(); i++) {
				CppFuncVar *param = func->functionParameters.at(i).get();
				int64_t callbackIndex = callbackIndicesByParameterIndex.at(i);
				if (callbackIndex >= 0) {
					// This is a callback parameter => send the pointer to the static delegate that receives the callback from C++.
					joiner.append(sb() << OUTPUT_TRANSFER_DELEGATE_PTR << callbackIndex);
				}
				else {
					joiner.append(param->declarationName);
				}
			}
			
			for (CppFuncVar *param : callbackParameters) {
				// Create a GC handle for each callback method so that the callback object is not GC'd if there are only C++ references but no C# references to it.
				joiner.append(sb() << "(void *)GCHandle.ToIntPtr(GCHandle.Alloc(" << param->declarationName << "))");
			}
			
			if (!func->isVoid()) {
				joiner.append(sb() << "&" << returnValuePtrName);
			}

			if (isExceptionCheckEnabled) {
				joiner.append(sb() << "&" << exceptionPtrName);
			}

			joiner.finish();
			output << ");";
		}

		if (isExceptionCheckEnabled) {
			output << OUTPUT_INTERNAL_UTIL_CLASS "." OUTPUT_CHECK_EXCEPTION "(" << exceptionPtrName << ");";
		}

		// Get the return value from the temporary variable.
		if (!func->isVoid()) {
			string returnValue = returnValuePtrName;
			if (func->pointerDepth == 0) {
				returnValue = func->type->transformReturnValueInCs(returnValue);
			}

			output << "return " << returnValue << ";";
		}

		output << "}";

		// Output an attribute that defines the C++ entry point of the function.
		output << " [SuppressUnmanagedCodeSecurity, DllImport(" OUTPUT_INTERNAL_UTIL_CLASS "." OUTPUT_CPP_LIBRARY_FILE ", EntryPoint = \"" << entryPoint << "\")]";

		// Output the "extern" method declaration that represents the C++ function.
		{
			output << " static extern void " << externFunctionName << "(";
			StringJoiner joiner(&output, ",");

			// Send the "this" pointer as a parameter if the function is a member function.
			if (isNonStaticMember) {
				joiner.append("void *" OUTPUT_THIS_PTR);
			}

			for (int i = 0; i < (int)func->functionParameters.size(); i++) {
				string paramName = string(OUTPUT_PARAM_NAME_PREFIX) + to_string(i);
				
				StringBuilder pb;
				CppFuncVar *param = func->functionParameters.at(i).get();
				if (param->isLambdaFunction) {
					// Send callback delegates as pointers.
					pb << "IntPtr " << paramName;
				}
				else {
					pb << formatDeclaration(*param, paramName, Language::CS, TypePresentation::TRANSFER_PARAMETER);
				}
				joiner.append(pb);
			}

			// For each callback function, send a GC handle that will be released when the C++ no longer references the callback.
			for (int i = 0; i < (int)callbackParameters.size(); i++) {
				joiner.append(sb() << "void *" OUTPUT_CALLBACK_ID_PARAM_NAME_PREFIX << i);
			}

			// Retrieve the C++ function's return value using an out-parameter pointer, because using return values in P/Invoke is more complicated for complex types and can be slower.
			if (!func->isVoid()) {
				joiner.append(formatDeclaration(getFunctionReturnValuePtrType(func), Language::CS, TypePresentation::TRANSFER_RETURN_VALUE));
			}

			if (isExceptionCheckEnabled) {
				// Retrieve C++ exceptions by using an out-parameter, because P/Invoke does not support propagating/converting exceptions properly to C#.
				joiner.append(sb() << OUTPUT_PTR_AND_SIZE << " *" << exceptionPtrName);
			}

			joiner.finish();
			output << ");";
		}

		output << '\n';
	}

	// Output public delegates that the application uses.
	for (const auto &line : delegateDeclarationLines) {
		output.appendLine(line);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CsOutputGenerator::outputClass(CppClass *clazz, StringBuilder &output) {
	output.appendLine("");

	NamespaceWriter namespaceWriter(cppParseUtil->cppNamespaceParser.getNamespaceNameFromQualifiedName(clazz->typeNames.cppType));
	namespaceWriter.begin(output);

	string unqualifiedClassName = cppParseUtil->csNamespaceParser.getUnqualifiedNameFromQualifiedName(clazz->typeMapping->typeNames.csType);

	if (clazz->classType == CppClassDeclarationType::CLASS || clazz->classType == CppClassDeclarationType::STRUCT) {
		outputComment(clazz->comment.get(), output);

		if (clazz->typeMapping->isPassByValue) {
			output.appendIndent() << "[StructLayout(LayoutKind.Explicit, Size = " << clazz->getLayoutOrThrow()->size << ")]\n";
		}

		output.appendIndent() << "public unsafe partial struct " << unqualifiedClassName << " {\n";
		output.changeIndent(+1);

		if (!clazz->typeMapping->isPassByValue) {
			// Wrap the C++ pointer in a C# struct that can be handled without allocating C# objects (which would increase GC overhead).
			output.appendLine("void *" OUTPUT_THIS_PTR ";");
			output.appendIndent() << "public " << unqualifiedClassName << "(void *ptr) { " OUTPUT_THIS_PTR " = ptr; }\n";
			output.appendLine("public bool isNull() { return " OUTPUT_THIS_PTR " == null; }");
			output.appendLine("public void * getPtr() { return " OUTPUT_THIS_PTR "; }");

			bool firstSuperclass = true;
			partition->forEachSuperclassDepthFirst(clazz, true, CppAccessModifier::UNKNOWN, [&](CppClass *superclass, CppAccessModifier accessModifier) {
				if (firstSuperclass) {
					output.appendLine("");
					firstSuperclass = false;
				}
				string superclassCsName = superclass->typeMapping->typeNames.csType;
				output.appendIndent() << "public static implicit operator " << superclassCsName << "(" << clazz->typeMapping->typeNames.csType << " v) => new " << superclassCsName << "(v.getPtr());\n";
			});
		}

		if (clazz->typeMapping->isPassByValue) {
			for (auto &field : clazz->getLayoutOrThrow()->fields) {
				int64_t arraySizeToUse = field.arraySize;
				bool isArray = arraySizeToUse > 0;
				string csTypeToUse = field.typeNames.csType;
				bool useByteArrayInsteadOfUnsupportedStructArray = false;

				outputComment(field.comment.get(), output);

				string fieldOffsetString = sb() << field.offset;
				output.appendIndent();
				output << "[FieldOffset(" << fieldOffsetString << ")] ";
				for (int64_t i = 0; i < max((int64_t)0, 3 - (int64_t)fieldOffsetString.size()); i++) output << " "; // Add spaces to align the variables at the same column.
				output << "public ";
				if (isArray) {
					output << "fixed ";
					if (!contains(config->allowedCsFixedSizeArrayTypes, csTypeToUse)) {
						useByteArrayInsteadOfUnsupportedStructArray = true;
						csTypeToUse = "byte";
						arraySizeToUse *= field.pointerDepth > 0 ? config->getPointerSizeBytes() : field.elementSize;
					}
				}
				output << csTypeToUse;
				output << " ";
				if (!useByteArrayInsteadOfUnsupportedStructArray) {
					for (int64_t i = 0; i < field.pointerDepth; i++) output << "*";
				}
				output << field.name;
				if (isArray) {
					output << "[" << arraySizeToUse << "]";
				}
				output << ";\n";
			}
		}

		vector<Callback> callbacks;
		int functionIndex = 0;
		for (auto &func : partition->getAccessibleMemberFunctions(clazz)) {
			outputFunction(func, clazz, functionIndex, output, callbacks);
			functionIndex++;
		}
		outputCallbackInternals(unqualifiedClassName, callbacks, output);

		output.changeIndent(-1);
		output.appendLine("}");
	}
	else if (clazz->classType == CppClassDeclarationType::ENUM) {
		outputComment(clazz->comment.get(), output);
		output.appendIndent() << "public enum " << unqualifiedClassName << " {\n";
		output.changeIndent(+1);
		for (const CppEnumField &field : clazz->enumFields) {
			outputComment(field.comment.get(), output);
			output.appendIndent();
			output << field.name;
			if (!field.value.empty()) output << " = " << field.value;
			output << ",\n";
		}
		output.changeIndent(-1);
		output.appendLine("}");
	}

	namespaceWriter.end(output);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CsOutputGenerator::outputCallbackInternals(const string &className, vector<Callback> &callbacks, StringBuilder &output) {
	int64_t numCallbacks = (int64_t)callbacks.size();

	if (numCallbacks > 0) {
		output.appendLine("");
	}

	for (int64_t callbackIndex = 0; callbackIndex < numCallbacks; callbackIndex++) {
		Callback &callback = callbacks.at(callbackIndex);

		string transferDelegateTypeName = sb() << OUTPUT_TRANSFER_DELEGATE << callbackIndex;
		CppFuncVar returnType = callback.callback->getFunctionReturnType();

		output.appendIndent() << "static " << transferDelegateTypeName << " " OUTPUT_TRANSFER_DELEGATE_INSTANCE << callbackIndex << ";\n";
		output.appendIndent() << "static " << "IntPtr " OUTPUT_TRANSFER_DELEGATE_PTR << callbackIndex << ";\n";

		// If the return type needs special marshalling, output the necessary attribute.
		string returnTypeMarshalAttribute = returnType.type->csMarshalAttributeIfUsed;
		if (!returnTypeMarshalAttribute.empty()) {
			output.appendIndent() << "[return: " << returnTypeMarshalAttribute << "]\n";
		}

		StringBuilder delegateParameterList;
		StringJoiner joiner(&delegateParameterList, ", ");
		for (auto &cbParam : callback.callback->functionParameters) {
			joiner.append(formatDeclaration(*cbParam, Language::CS, TypePresentation::TRANSFER_CALLBACK_PARAMETER));
		}
		joiner.append("IntPtr " OUTPUT_CALLBACK_DELEGATE_INSTANCE_PARAM);
		joiner.finish();

		output.appendIndent() << "delegate " << formatDeclaration(returnType, transferDelegateTypeName, Language::CS, TypePresentation::TRANSFER_CALLBACK_RETURN_VALUE) << "(" << delegateParameterList << ");\n";

		// Output a static method that receives the call from C++ and forwards it to the application delegate.
		// P/Invoke would support passing a delegate directly to the native function, but IL2CPP supports only static delegates this way
		// => use the forwarding method to add non-static delegate support to IL2CPP.
		output.appendIndent() << "#if ENABLE_IL2CPP\n";
		output.appendIndent() << "[AOT.MonoPInvokeCallback(typeof(" << transferDelegateTypeName << "))]\n";
		output.appendIndent() << "#endif\n";
		output.appendIndent() << "static " << formatDeclaration(returnType, sb() << OUTPUT_TRANSFER_METHOD << callbackIndex, Language::CS, TypePresentation::TRANSFER_CALLBACK_RETURN_VALUE) << "(" << delegateParameterList << ") {";

		if (!returnType.isVoid()) {
			output << "return ";
		}
		{
			// Call the application delegate.
			output << "((" << callback.publicDelegateTypeName << ")GCHandle.FromIntPtr(" OUTPUT_CALLBACK_DELEGATE_INSTANCE_PARAM ").Target)(";
			StringJoiner joiner(&output, ",");
			for (auto &param : callback.callback->functionParameters) {
				joiner.append(param->declarationName);
			}
			joiner.finish();
			output << ");";
		}

		output << "}\n";
	}

	if (numCallbacks > 0) {
		// Output a static constructor that stores the transfer delegates and their function pointer to variables,
		// so that the delegates won't be garbage-collected and so that the pointers are quickly available.
		output.appendLine("");
		output.appendIndent() << "static " << className << "() {\n";
		output.changeIndent(+1);

		for (int64_t callbackIndex = 0; callbackIndex < numCallbacks; callbackIndex++) {
			output.appendIndent() << OUTPUT_TRANSFER_DELEGATE_INSTANCE << callbackIndex << " = " OUTPUT_TRANSFER_METHOD << callbackIndex << ";\n";
			output.appendIndent() << OUTPUT_TRANSFER_DELEGATE_PTR << callbackIndex << " = Marshal.GetFunctionPointerForDelegate(" OUTPUT_TRANSFER_DELEGATE_INSTANCE << callbackIndex << ");\n";
		}

		output.changeIndent(-1);
		output.appendIndent() << "}\n";
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CsOutputGenerator::generateOutput(StringBuilder &output) {
	StopWatch stopWatch = app->createStopWatchForPerformanceMeasurement();

	string apiClassName = config->getApiClassName();
	string initMethodName = "init";

	output.appendIndent() << "// " << getAutogeneratedFileComment() << '\n';
	output.appendLine("");
	output.appendLine("// ReSharper disable All");
	output.appendLine("");
	output.appendLine("using System;");
	output.appendLine("using System.Runtime.InteropServices;");
	output.appendLine("using System.Security;");
	output.appendLine("using System.Text;");

	output.appendLine("");
	NamespaceWriter namespaceWriter(config->csNamespace);
	namespaceWriter.begin(output);

	if (isMainPartition()) {
		output.appendIndent() << "public static unsafe class " << apiClassName << " {\n";
		output.changeIndent(+1);
		output.appendIndent() << "public const string cppLibraryFile = " OUTPUT_INTERNAL_UTIL_CLASS "." OUTPUT_CPP_LIBRARY_FILE ";\n";
		output.appendLine("");
		output.appendIndent() << "public static void " << initMethodName << "() {\n";
		output.changeIndent(+1);
		output.appendIndent() << OUTPUT_INTERNAL_UTIL_CLASS "." << initMethodName << "();\n";
		output.changeIndent(-1);
		output.appendLine("}");
		output.changeIndent(-1);
		output.appendLine("}");

		output.appendLine("");
		output.appendLine("public unsafe struct " OUTPUT_PTR_AND_SIZE " {");
		output.changeIndent(+1);
		output.appendLine("public void *ptr;");
		output.appendLine("public long size;");
		output.changeIndent(-1);
		output.appendLine("}");

		output.appendLine("");
		output.appendLine("public static unsafe class " OUTPUT_INTERNAL_UTIL_CLASS " {");
		output.changeIndent(+1);
		{
			output.appendIndent() << "public const string " OUTPUT_CPP_LIBRARY_FILE " = \"" << getCppLibraryFile() << "\";\n";
			output.appendLine("static bool isInitialized = false;");
			output.appendLine("static Encoding utf8Encoding = new UTF8Encoding();");
			output.appendLine("static Encoding utf16Encoding = new UnicodeEncoding();");
			output.appendLine("");
			output.appendLine("// Invoked when the c++ code no longer needs a particular c# callback. Keep a reference to this function here so that it is not GC'd.");
			output.appendLine("static ReleaseCallbackCallback releaseCallbackCallback;");
			output.appendLine("");
			output.appendLine("delegate void ReleaseCallbackCallback(IntPtr ptr);");
			output.appendIndent() << "[SuppressUnmanagedCodeSecurity, DllImport(" OUTPUT_CPP_LIBRARY_FILE ", EntryPoint = \"rabbitcall_init\")]\n";
			output.appendLine("static extern void rabbitcall_init(ReleaseCallbackCallback releaseCallbackCallback, " OUTPUT_PTR_AND_SIZE " *exceptionPtr);");
			output.appendLine("");
			output.appendIndent() << "[SuppressUnmanagedCodeSecurity, DllImport(" OUTPUT_CPP_LIBRARY_FILE ", EntryPoint = \"rabbitcall_allocateMemory\")]\n";
			output.appendLine("public static extern void * rabbitcall_allocateMemory(long size);");
			output.appendLine("");
			output.appendIndent() << "[SuppressUnmanagedCodeSecurity, DllImport(" OUTPUT_CPP_LIBRARY_FILE ", EntryPoint = \"rabbitcall_deallocateMemory\")]\n";
			output.appendLine("public static extern void rabbitcall_deallocateMemory(void *ptr);");
			output.appendLine("");
			output.appendIndent() << "[SuppressUnmanagedCodeSecurity, DllImport(" OUTPUT_CPP_LIBRARY_FILE ", EntryPoint = \"rabbitcall_getTypeSizeByName\")]\n";
			output.appendLine("public static extern long rabbitcall_getTypeSizeByName([MarshalAs(UnmanagedType.LPStr)] string name);");
			output.appendLine("");
			output.appendLine("static void checkTypeSize(string cppName, string csName, long csSize, long? configuredSize) {");
			output.changeIndent(+1);
			output.appendLine("long cppSize = rabbitcall_getTypeSizeByName(cppName);");
			output.appendLine("if (cppSize != csSize) throw new Exception($\"Type has different size in C++ ({cppName}: {cppSize}) than in C# ({csName}: {csSize})\");");
			output.appendLine("if (configuredSize != null && cppSize != configuredSize.Value) throw new Exception($\"Type has different size in C++ ({cppName}: {cppSize}) than in configuration ({configuredSize.Value})\");");
			output.changeIndent(-1);
			output.appendLine("}");
			output.appendLine("");
			output.appendIndent() << "public static void " << initMethodName << "() {\n";
			output.changeIndent(+1);
			output.appendLine("if (isInitialized) throw new Exception(\"Already initialized\");");
			output.appendLine("isInitialized = true;");
			output.appendLine("");
			output.appendLine(OUTPUT_PTR_AND_SIZE " " OUTPUT_EXCEPTION_PTR ";");
			output.appendLine("releaseCallbackCallback = " OUTPUT_INTERNAL_UTIL_CLASS ".releaseCallback;");
			output.appendLine("rabbitcall_init(releaseCallbackCallback, &" OUTPUT_EXCEPTION_PTR ");");
			output.appendLine("if (" OUTPUT_EXCEPTION_PTR ".ptr != null) throw new Exception(" OUTPUT_INTERNAL_UTIL_CLASS ".readStringUtf8AndFree(" OUTPUT_EXCEPTION_PTR "));");
			output.appendLine("");
			partition->getTypeMap()->forEachTypeMapping([&](TypeMapping *typeMapping) {
				if (typeMapping->hasDefinedSize()) {
					output.appendIndent() << "checkTypeSize(\"" << typeMapping->typeNames.cppType << "\", \"" << typeMapping->typeNames.csType << "\", sizeof(" << typeMapping->typeNames.csType << "), " << typeMapping->size << ");\n";
				}
			});
			output.appendLine("");
			int64_t configuredPointerSize = config->getPointerSizeBytes();
			output.appendIndent() << "if (sizeof(void *) != " << configuredPointerSize << ") throw new Exception($\"Different configured pointer size (" << configuredPointerSize << " bytes) than actual size ({sizeof(void *)} bytes)\");\n";
			output.changeIndent(-1);
			output.appendLine("}");
			output.appendLine("");
			output.appendLine("public static void " OUTPUT_CHECK_INIT "() {");
			output.changeIndent(+1);
			output.appendIndent() << "if (!isInitialized) throw new Exception(\"RabbitCall not initialized, please call " << apiClassName << "." << initMethodName << "() at startup (for all partitions if you have multiple ones).\");\n";
			output.changeIndent(-1);
			output.appendLine("}");
			output.appendLine("");
			output.appendLine("public static string readStringUtf8AndFree(" OUTPUT_PTR_AND_SIZE " ptr) {");
			output.changeIndent(+1);
			output.appendLine("if (ptr.ptr == null) return null;");
			output.appendLine("string s = utf8Encoding.GetString((byte *)ptr.ptr, checked((int)(ptr.size - sizeof(byte))));");
			output.appendLine("rabbitcall_deallocateMemory(ptr.ptr); // Free the buffer that was allocated in c++");
			output.appendLine("return s;");
			output.changeIndent(-1);
			output.appendLine("}");
			output.appendLine("");
			output.appendLine("public static string readStringUtf16AndFree(" OUTPUT_PTR_AND_SIZE " ptr) {");
			output.changeIndent(+1);
			output.appendLine("if (ptr.ptr == null) return null;");
			output.appendLine("string s = utf16Encoding.GetString((byte *)ptr.ptr, checked((int)(ptr.size - sizeof(char))));");
			output.appendLine("rabbitcall_deallocateMemory(ptr.ptr); // Free the buffer that was allocated in c++");
			output.appendLine("return s;");
			output.changeIndent(-1);
			output.appendLine("}");
			output.appendLine("");
			output.appendLine("#if ENABLE_IL2CPP");
			output.appendLine(sb() << "[AOT.MonoPInvokeCallback(typeof(ReleaseCallbackCallback))]");
			output.appendLine("#endif");
			output.appendLine("public static void releaseCallback(IntPtr callback) {");
			output.changeIndent(+1);
			output.appendLine("GCHandle.FromIntPtr(callback).Free();");
			output.changeIndent(-1);
			output.appendLine("}");
			output.appendLine("");
			output.appendLine("public static void " OUTPUT_CHECK_EXCEPTION "(" OUTPUT_PTR_AND_SIZE " " OUTPUT_EXCEPTION_PTR ") {");
			output.changeIndent(+1);
			output.appendLine("if (" OUTPUT_EXCEPTION_PTR ".ptr != null) throw new Exception(readStringUtf8AndFree(" OUTPUT_EXCEPTION_PTR "));");
			output.changeIndent(-1);
			output.appendLine("}");
		}
		output.changeIndent(-1);
		output.appendLine("}");
	}

	map<string, vector<CppFuncVar *>> globalFunctionsByNamespace;
	partition->forEachGlobalFunction([&](CppFuncVar *func) {
		string ns = func->getNamespaceName();
		vector<CppFuncVar *> &list = globalFunctionsByNamespace[ns];
		list.push_back(func);
	});

	for (auto &entry : globalFunctionsByNamespace) {
		output.appendLine("");
		NamespaceWriter funcNamespaceWriter(entry.first);
		funcNamespaceWriter.begin(output);

		string globalClassName = config->csGlobalFunctionContainerClass;
		output.appendIndent() << "public static unsafe partial class " << globalClassName << " {\n";
		output.changeIndent(+1);

		vector<Callback> callbacks;
		int functionIndex = 0;
		for (auto &func : entry.second) {
			outputFunction(func, nullptr, functionIndex, output, callbacks);
			functionIndex++;
		}
		outputCallbackInternals(globalClassName, callbacks, output);

		output.changeIndent(-1);
		output.appendLine("}");
		funcNamespaceWriter.end(output);
	}

	partition->forEachClass([&](CppClass *clazz) {
		outputClass(clazz, output);
	});

	namespaceWriter.end(output);

	stopWatch.mark("output / generate .cs");
}
