#include "pch.h"

void CppOutputGenerator::outputCallbackWrapper(CppFuncVar *callbackParam, CppFuncVar *enclosingFunction, CppClass *enclosingClassIfNotGlobal, StringBuilder &output, string *wrapperClassNameOut) {
	string wrapperClassName = cppProject->reserveCppGlobalIdentifier(sb() << OUTPUT_CALLBACK "_" << (enclosingClassIfNotGlobal ? enclosingClassIfNotGlobal->typeNames.cppType : "global") << "_" << enclosingFunction->declarationName << "_" << callbackParam->declarationName);
	*wrapperClassNameOut = wrapperClassName;

	CppFuncVar returnType = callbackParam->getFunctionReturnType();

	output << OUTPUT_CALLBACK_WRAPPER "(" << wrapperClassName << ", ";
	output << "typedef " << formatDeclaration(*callbackParam, "FunctionPtrType", Language::CPP, TypePresentation::TRANSFER_PARAMETER) << ", ";
	output << formatDeclaration(returnType, "operator()", Language::CPP, TypePresentation::PUBLIC) << "(";

	{
		StringJoiner joiner(&output, ",");
		for (auto &param : callbackParam->functionParameters) {
			joiner.append(formatDeclaration(*param, Language::CPP, TypePresentation::PUBLIC));
		}
		joiner.finish();
	}

	output << ") { ";
	const char *returnTempVariableIfUsed = nullptr;
	if (!returnType.isVoid()) {
		returnTempVariableIfUsed = OUTPUT_RETURN_VALUE_TEMP;
		output << formatDeclaration(returnType, OUTPUT_RETURN_VALUE_TEMP, Language::CPP, TypePresentation::TRANSFER_CALLBACK_RETURN_VALUE) << " = ";
	}
	output << "cb->callbackHandler(";
	{
		StringJoiner joiner(&output, ",");
		for (auto &param : callbackParam->functionParameters) {
			string p = param->declarationName;
			if (param->pointerDepth == 0) {
				if (param->type->isString) {
					p.append(".c_str()"); // Strings from C++ to C# callbacks are passed as a pointer.
				}
			}
			joiner.append(p);
		}
		joiner.append("cb->appCallback");
		joiner.finish();
	}
	output << ");";
	if (returnType.type->isString) {
		// Convert the temporary "const char *" string to an std::string object (or similar depending on string type) before deallocating the temporary string.
		returnTempVariableIfUsed = OUTPUT_RETURN_VALUE_TEMP2;
		output << " " << formatDeclaration(returnType, OUTPUT_RETURN_VALUE_TEMP2, Language::CPP, TypePresentation::PUBLIC) << "(" << OUTPUT_RETURN_VALUE_TEMP << ");";

		output << " " << OUTPUT_DEALLOCATE_TASKMEM << "((void *)" << OUTPUT_RETURN_VALUE_TEMP << ");";
	}
	if (returnTempVariableIfUsed) {
		output << " return " << returnTempVariableIfUsed << ";";
	}
	output << " }";
	output << ")";
	output << '\n';
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CppOutputGenerator::outputFunction(CppFuncVar *func, CppClass *enclosingClassIfNotGlobal, StringBuilder &output) {
	bool isNonStaticMember = !func->isStatic && enclosingClassIfNotGlobal;
	bool isExceptionCheckEnabled = !func->isNoexcept;

	vector<string> wrapperClassNamesByParameterIndex;
	for (int i = 0; i < (int)func->functionParameters.size(); i++) {
		CppFuncVar *param = func->functionParameters.at(i).get();

		if (param->isLambdaFunction) {
			// For each callback function, generate a functor class that keeps a reference count to the C# object and translates calls from C++ to C#.
			string wrapperClassName;
			outputCallbackWrapper(param, func, enclosingClassIfNotGlobal, output, &wrapperClassName);
			wrapperClassNamesByParameterIndex.push_back(wrapperClassName);
		}
	}

	output.appendIndent();
	output << (isExceptionCheckEnabled ? OUTPUT_FUNC_MACRO_EXC : OUTPUT_FUNC_MACRO_NOEXC) << "(";

	// Output function declaration.
	{
		output << partition->getFunctionEntryPoint(func, enclosingClassIfNotGlobal) << "(";

		StringJoiner joiner(&output, ",");

		if (isNonStaticMember) {
			// Put the "this" pointer as the first parameter, because it is the first parameter also in C++ functions, and the wrapper
			// function in C++ can pass the parameters to the final C++ function in the same registers in which they were passed from C#.
			joiner.append(sb() << enclosingClassIfNotGlobal->typeNames.cppType << " *" << OUTPUT_THIS_PTR);
		}

		for (int i = 0; i < (int)func->functionParameters.size(); i++) {
			string paramName = string(OUTPUT_PARAM_NAME_PREFIX) + to_string(i);
			CppFuncVar *param = func->functionParameters.at(i).get();
			joiner.append(formatDeclaration(*param, paramName, Language::CPP, TypePresentation::TRANSFER_PARAMETER));
		}

		for (int i = 0; i < (int)wrapperClassNamesByParameterIndex.size(); i++) {
			joiner.append(sb() << "void *" OUTPUT_CALLBACK_ID_PARAM_NAME_PREFIX << i);
		}

		if (!func->isVoid()) {
			joiner.append(formatDeclaration(getFunctionReturnValuePtrType(func), Language::CPP, TypePresentation::TRANSFER_RETURN_VALUE));
		}

		if (isExceptionCheckEnabled) {
			joiner.append(OUTPUT_PTR_AND_SIZE " *" OUTPUT_EXCEPTION_PTR);
		}

		joiner.finish();
		output << "), ";
	}

	{
		// Write the return value to the out-parameter pointer.
		string callSuffix;
		if (!func->isVoid()) {
			output << "*" OUTPUT_RETURN_VALUE_PTR " = ";

			TypeMapping *returnType = func->type;
			if (func->pointerDepth == 0) {
				if (returnType->isString) {
					output << "_rc_createString(";
					callSuffix = ")";
				}
			}
		}

		// Output function name.
		if (isNonStaticMember) {
			output << OUTPUT_THIS_PTR "->";
			//b << "((" << enclosingClassIfNotGlobal->typeNames.cppType << " *)" << OUTPUT_THIS_PTR << ")->";
		}
		else {
			if (enclosingClassIfNotGlobal) {
				output << enclosingClassIfNotGlobal->typeNames.cppType << "::";
			}
			else {
				output << func->namespacePrefixIfGlobal;
			}
		}

		// Output parameters.
		output << func->declarationName;
		output << "(";
		StringJoiner joiner(&output, ",");
		{
			int callbackIndex = 0;
			for (int i = 0; i < (int)func->functionParameters.size(); i++) {
				string paramName = string(OUTPUT_PARAM_NAME_PREFIX) + to_string(i);

				CppFuncVar *param = func->functionParameters.at(i).get();
				TypeMapping *paramType = param->type;
				if (param->isLambdaFunction) {
					// Wrap the callback function pointer in a functor that can be passed e.g. as a lambda function to the final C++ function. Create a C++ object that maintains a reference count and releases
					// the C# GC handle when there are no more C++ std::function objects or others referencing the callback, because the callback could be stored on the C++ side and used after this function has returned.
					// It might be possible to optimize this by deferring the creation of the reference count object until there are more than one references on the C++ side.
					string functionPtrType = formatDeclaration(*param, "", Language::CPP, TypePresentation::TRANSFER_PARAMETER);
					joiner.append(sb() << wrapperClassNamesByParameterIndex.at(callbackIndex) << "(new " OUTPUT_CALLBACK_HOLDER "<" << functionPtrType << ">(" << paramName << ", " OUTPUT_CALLBACK_ID_PARAM_NAME_PREFIX << to_string(callbackIndex) << "))");
					callbackIndex++;
				}
				else {
					string v = paramName;
					if (paramType->isString) {
						v = sb() << paramType->typeNames.cppType << "((const " << paramType->charType << " *)" << v << ")";
					}
					joiner.append(v);
				}
			}
		}
		joiner.finish();
		output << ")" << callSuffix << ";";
	}

	output << ")\n";
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CppOutputGenerator::outputClass(CppClass *clazz, StringBuilder &output) {
	if (clazz->classType != CppClassDeclarationType::ENUM) {
		bool first = true;
		for (auto &func : partition->getAccessibleMemberFunctions(clazz)) {
			if (first) { // Output empty line before non-empty classes only.
				output.appendLine("");
				first = false;
			}
			outputFunction(func, clazz, output);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CppOutputGenerator::outputEnum(CppClass *clazz, StringBuilder &output) {
	string namespaceName;
	string unqualifiedName;
	cppParseUtil->cppNamespaceParser.separateNamespaceAndName(namespaceName, unqualifiedName, clazz->typeNames.cppType);

	string reflVariableName = sb() << "RabbitCallInternalNamespace::" << partition->getEnumReflectionVariableName(clazz, false);

	output.appendLine("");

	if (!namespaceName.empty()) {
		output.appendIndent() << "namespace " << namespaceName << " {\n";
		output.changeIndent(+1);
	}

	output.appendIndent() << unqualifiedName << " parse_" << unqualifiedName << "(const std::string &s) { return (" << unqualifiedName << ")" << reflVariableName << ".parse(s); }\n";
	output.appendIndent() << "std::string toString_" << unqualifiedName << "(" << unqualifiedName << " v) { return " << reflVariableName << ".toString((int64_t)v); }\n";
	output.appendIndent() << "std::ostream & operator<<(std::ostream &os, const " << unqualifiedName << " &v) { return os << " << reflVariableName << ".toString((int64_t)v); }\n";

	if (!namespaceName.empty()) {
		output.changeIndent(-1);
		output.appendLine("}");
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CppOutputGenerator::generateOutput(StringBuilder &output) {
	StopWatch stopWatch = app->createStopWatchForPerformanceMeasurement();

	generatedHeaderFile = getMainPartition()->getOutputFileByTypeIfExists("h");
	if (!generatedHeaderFile) EXC(".h output file not specified");

	typeMap = cppProject->getTypeMap();

	output.appendIndent() << "// " << getAutogeneratedFileComment() << '\n';
	output.appendLine("");
	output.appendLine(config->generatedCppFilePrologue);

	if (config->includeSourceHeadersInGeneratedCpp) {
		for (const string &file : partition->getSourceHeaderFilesUsed()) {
			output.appendIndent() << "#include \"" << getRelativePathForIncludeDirective(file) << "\"\n";
		}
	}
	
	if (generatedHeaderFile) {
		output.appendLine("");
		output.appendIndent() << "#include \"" << getRelativePathForIncludeDirective(generatedHeaderFile->file.toString()) << "\"\n";
	}

	if (isMainPartition()) {
		output.appendLine("");
		output.appendLine("RabbitCallType::RabbitCallType(const std::string &name, size_t size): name(name), size(size) {}");
		output.appendLine("size_t RabbitCallType::getSize() { return size; }");
		output.appendLine("const std::string & RabbitCallType::getName() { return name; }");
	}

	output.appendLine("");
	output.appendLine("namespace RabbitCallInternalNamespace {");
	output.changeIndent(+1);
	{
		if (isMainPartition()) {
			output.appendLine("");
			output.appendLine("void * " OUTPUT_ALLOCATE_MEMORY "(int64_t size) {");
			output.changeIndent(+1);
			output.appendLine("void *p = malloc(size);");
			output.appendLine("if (!p) throw std::bad_alloc();");
			output.appendLine("return p;");
			output.changeIndent(-1);
			output.appendLine("}");

			output.appendLine("");
			output.appendLine("void " OUTPUT_DEALLOCATE_MEMORY "(void *ptr) noexcept {");
			output.changeIndent(+1);
			output.appendLine("free(ptr);");
			output.changeIndent(-1);
			output.appendLine("}");

			output.appendLine("");
			output.appendLine("void * " OUTPUT_ALLOCATE_TASKMEM "(int64_t size) {");
			output.changeIndent(+1);
			output.appendLine("#if defined (_MSC_VER)");
			output.appendLine("void *p = CoTaskMemAlloc(size);");
			output.appendLine("#else");
			output.appendLine("void *p = malloc(size);");
			output.appendLine("#endif");
			output.appendLine("if (!p) throw std::bad_alloc();");
			output.appendLine("return p;");
			output.changeIndent(-1);
			output.appendLine("}");

			output.appendLine("");
			output.appendLine("void " OUTPUT_DEALLOCATE_TASKMEM "(void *ptr) noexcept {");
			output.changeIndent(+1);
			output.appendLine("#if defined (_MSC_VER)");
			output.appendLine("CoTaskMemFree(ptr);");
			output.appendLine("#else");
			output.appendLine("free(ptr);");
			output.appendLine("#endif");
			output.changeIndent(-1);
			output.appendLine("}");

			output.appendLine("");
			output.appendLine("void RabbitCallEnum::setMapping(int64_t id, const std::string &name) {");
			output.changeIndent(+1);
			output.appendLine("namesById[id] = name;");
			output.appendLine("idsByName[name] = id;");
			output.changeIndent(-1);
			output.appendLine("}");

			output.appendLine("");
			output.appendLine("int64_t RabbitCallEnum::parse(const std::string &name) {");
			output.changeIndent(+1);
			output.appendLine("auto i = idsByName.find(name);");
			output.appendIndent() << "return i == idsByName.end() ? 0 : i->second;\n";
			output.changeIndent(-1);
			output.appendLine("}");

			output.appendLine("");
			output.appendIndent() << "std::string RabbitCallEnum::toString(int64_t id) {\n";
			output.changeIndent(+1);
			output.appendLine("auto i = namesById.find(id);");
			output.appendLine("return i == namesById.end() ? \"\" : i->second;");
			output.changeIndent(-1);
			output.appendLine("}");

			output.appendLine("RabbitCallInternal rabbitCallInternal;");
			output.appendLine("");
			output.appendLine("RabbitCallType * RabbitCallInternal::getTypeByName(const std::string &name) {");
			output.changeIndent(+1);
			output.appendLine("auto i = typesByName.find(name);");
			output.appendLine("return i == typesByName.end() ? NULL : i->second;");
			output.changeIndent(-1);
			output.appendLine("}");
		}

		output.appendLine("");
		partition->forEachEnum([&](CppClass* clazz) {
			output.appendIndent() << "RabbitCallEnum " << partition->getEnumReflectionVariableName(clazz, false) << ";\n";
		});
	}
	output.changeIndent(-1);
	output.appendLine("}");

	output.appendLine("");
	output.appendLine("using namespace RabbitCallInternalNamespace;");

	if (isMainPartition()) {
		output.appendLine("");
		output.appendLine("extern \"C\" RC_EXPORT void rabbitcall_init(void(*releaseCallbackCallback)(void *), " OUTPUT_PTR_AND_SIZE " *" OUTPUT_EXCEPTION_PTR ") noexcept {");
		output.changeIndent(+1);
		output.appendLine("try {");
		output.changeIndent(+1);
		output.appendLine("if (rabbitCallInternal.initialized) throw std::logic_error(\"RabbitCall already initialized\");");
		output.appendLine("rabbitCallInternal.initialized = true;");
		output.appendLine("rabbitCallInternal.releaseCallbackCallback = releaseCallbackCallback;");
		typeMap->forEachTypeMapping([&](TypeMapping *m) {
			if (m->isString) {
				// Check that the string character type has correct size.
				string charType = m->charType;
				TypeMapping *charTypeMapping = typeMap->getTypeMappingByPossiblyPartiallyQualifiedCppTypeOrThrow(charType, m->sourceLocation);
				string stringCharSizeExpr = sb() << "sizeof(" << m->typeNames.cppType << "().c_str()[0])";
				output.appendLine(sb() << "if (" << stringCharSizeExpr << " != " << charTypeMapping->size << ") throw std::logic_error((std::string(\"The character type ('" << charType << "') configured for " << m->typeNames.cppType << " has incorrect size: " << charTypeMapping->size << ", expected: \") + std::to_string(" << stringCharSizeExpr << ") + \" (wrong character type in configuration file?)\").c_str());");
			}
		});
		cppProject->forEachPartition([&](CppPartition *p) {
			output.appendLine(sb() << "RabbitCallInternalNamespace::initPartition_" << p->getName() << "();");
		});
		output.changeIndent(-1);
		output.appendLine("}");
		output.appendLine("catch (std::exception &" OUTPUT_EXCEPTION_VAR ") {");
		output.changeIndent(+1);
		output.appendLine("*" OUTPUT_EXCEPTION_PTR " = _rc_createString(std::string(" OUTPUT_EXCEPTION_VAR ".what()));");
		output.changeIndent(-1);
		output.appendLine("}");
		output.changeIndent(-1);
		output.appendLine("}");

		output.appendLine("");
		output.appendLine("extern \"C\" RC_EXPORT void * rabbitcall_allocateMemory(int64_t size) {");
		output.changeIndent(+1);
		output.appendLine("return " OUTPUT_ALLOCATE_MEMORY "(size);");
		output.changeIndent(-1);
		output.appendLine("}");

		output.appendLine("");
		output.appendLine("extern \"C\" RC_EXPORT void rabbitcall_deallocateMemory(void *ptr) noexcept {");
		output.changeIndent(+1);
		output.appendLine(OUTPUT_DEALLOCATE_MEMORY "(ptr);");
		output.changeIndent(-1);
		output.appendLine("}");

		output.appendLine("");
		output.appendLine("extern \"C\" RC_EXPORT int64_t rabbitcall_getTypeSizeByName(const char *name) noexcept {");
		output.changeIndent(+1);
		output.appendLine("RabbitCallType *type = rabbitCallInternal.getTypeByName(name);");
		output.appendLine("return type == NULL ? -1 : (int64_t)type->getSize();");
		output.changeIndent(-1);
		output.appendLine("}");
	}

	output.appendLine("");
	output.appendLine(sb() << "void RabbitCallInternalNamespace::initPartition_" << partition->getName() << "() {");
	output.changeIndent(+1);
	typeMap->forEachTypeMapping([&](TypeMapping *typeMapping) {
		if ((typeMapping->partitionName.empty() && isMainPartition()) || typeMapping->partitionName == partition->getName()) {
			string typeName = typeMapping->typeNames.cppType;
			string sizeExpr = "0";
			if (typeMapping->hasDefinedSize()) {
				sizeExpr = sb() << "sizeof(" << typeName << ")";
			}
			output.appendIndent() << "rabbitCallInternal.typesByName[\"" << typeName << "\"] = new RabbitCallType(\"" << typeName << "\", " << sizeExpr << ");\n";
		}
	});
	partition->forEachEnum([&](CppClass *clazz) {
		string enumVariable = partition->getEnumReflectionVariableName(clazz, false);
		// Set the enum mappings in reverse order so that if multiple fields have the same value, the value will be mapped to the first one.
		for (int64_t i = (int64_t)clazz->enumFields.size() - 1; i >= 0; i--) {
			CppEnumField &field = clazz->enumFields.at(i);
			string name = field.name;
			output.appendIndent() << "RabbitCallInternalNamespace::" << enumVariable << ".setMapping((int64_t)" << clazz->typeNames.cppType << "::" << name << ", \"" << name << "\");\n";
		}
	});
	output.changeIndent(-1);
	output.appendLine("}");

	partition->forEachEnum([&](CppClass *clazz) {
		outputEnum(clazz, output);
	});

	output.appendLine("");
	partition->forEachGlobalFunction([&](CppFuncVar *func) {
		outputFunction(func, nullptr, output);
	});

	partition->forEachClass([&](CppClass *clazz) {
		outputClass(clazz, output);
	});

	stopWatch.mark("output / generate .cpp");
}

