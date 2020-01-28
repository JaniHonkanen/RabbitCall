#include "pch.h"

CppPartition::CppPartition(const Config::Partition &partitionConfig, CppProject *cppProject)
	: cppProject(cppProject) {

	partitionName = partitionConfig.name;
	cppSourceDirIfExists = partitionConfig.cppSourceDir;
	cppSourceFileSet = partitionConfig.cppSourceFileSet;
	
	config = cppProject->getConfig();
	errorList = cppProject->getErrorList();
	typeMap = cppProject->getTypeMap();
	
	funcVarParser = make_shared<CppFuncVarParser>(config, typeMap);

	for (const auto &entry : config->outputDirsByType) {
		string fileType = entry.first;
		const Config::OutputDir &outputDir = entry.second;

		string filename = sb() << "rabbitcall_generated" << "_" << partitionName << "." << fileType;

		shared_ptr<CppOutputFile> file = make_shared<CppOutputFile>();
		file->file = outputDir.path.path() / filename;
		file->shouldWriteByteOrderMark = outputDir.shouldWriteByteOrderMark;
		outputFilesByType[fileType] = file;
	}
}

CppPartition::~CppPartition() {
}

shared_ptr<CppOutputFile> CppPartition::getOutputFileByTypeIfExists(const string &fileType) {
	auto iter = outputFilesByType.find(fileType);
	if (iter == outputFilesByType.end()) return nullptr;
	return iter->second;
}

void CppPartition::addClass(shared_ptr<CppClass> clazz) {
	classes.push_back(clazz);
}

void CppPartition::applyFileParseResult(shared_ptr<CppParsedFile> parsedFile) {
	CppFile *file = parsedFile->file.get();

	// Include each parsed header file in the generated .cpp file so that symbols defined in the headers will be available in generated code.
	// Don't include those headers that did not contain any exported elements to reduce compile time.
	if (parsedFile->hasExportedElements()) {
		if (file->isHeaderFile) {
			string path = file->getPath();
			if (!contains(headerFilesUsed, path)) {
				headerFilesUsed.insert(path);
			}
		}
	}

	statistics.numClasses += parsedFile->classes.size();
	statistics.numSourceBytes += file->fileSize;
	statistics.numSourceFiles++;
}

void CppPartition::startFindSourceFiles(ThreadPool::TaskSet *tasks) {
	if (!cppSourceDirIfExists.empty()) {
		rootDir.initByScanningDirectoryForSourceFilesRecursivelyAndConcurrently(cppSourceDirIfExists, Path(), &cppSourceFileSet, tasks, errorList);
	}
}

void CppPartition::finishFindSourceFiles() {
	rootDir.forEachFileRecursively([&](const shared_ptr<CppFile> &sourceFile) {
		Path path = Path(sourceFile->getPath());

		cppProject->checkFileNotInsideAnotherPartition(path, this);

		if (!cppProject->isFileInsideAnyOutputDir(path.toString())) { // Don't process generated files as source files.
			parsedFiles.push_back(make_shared<CppParsedFile>(sourceFile));
		}
	});
}

bool CppPartition::containsPath(const Path &path) {
	if (cppSourceDirIfExists.empty()) return false;
	return cppSourceFileSet.isPathInSet(getRelativePathBetweenAbsolutePaths(path.toString(), cppSourceDirIfExists.toString()));
}

void CppPartition::resolveClassMembers() {
	LOG_DETAIL("Resolving class members");
	forEachClass([&](CppClass *clazz) {
		if (clazz->classType == CppClassDeclarationType::STRUCT) {
			for (CppUnresolvedMember &member : clazz->unresolvedMembers) {
				errorList->runWithExceptionCheck([&] {
					CppSourceCodeView elements = member.sourceCode->getSourceCode();

					if (!funcVarParser->tryIdentifyFunction(elements).isFunction) {
						// The declaration is not a function => parse it as a variable or a list of variables.
						shared_ptr<CppFuncVar> baseType; // Holds the variable type from the first variable in the list so that it can be used for subsequent variables.
						CppElementIterator variableIter(elements, nullptr);
						bool firstVariable = true;
						while (variableIter.isValid()) {
							CppElementIterator::InfiniteLoopGuard variableLoopGuard(&variableIter);

							CppFuncVarParser::ParseVariableResult variableResult = funcVarParser->parseVariableDeclaration(variableIter, true, &baseType);

							if (firstVariable) {
								// If multiple variables are declared on the same line in C++, output the comment to the first variable only.
								variableResult.decl->comment = member.comment;
							}

							clazz->fields.push_back(variableResult.decl);
							LOG_DEBUG(sb() << "Parsing field: " << variableResult.decl->declarationName);

							CppClass *dependeeClass = cppProject->getClassByCppNameIfExists(variableResult.decl->type->typeNames.cppType).get();
							if (dependeeClass) {
								cppProject->addClassDependency(clazz, dependeeClass);
							}

							if (!variableResult.doesListContinue) {
								break;
							}

							variableLoopGuard.checkProgress();
							firstVariable = false;
						}
					}
				});
			}
		}
	});
}

void CppPartition::resolveSuperclasses() {
	LOG_DETAIL("Resolving superclasses");
	forEachClass([&](CppClass *clazz) {
		for (CppSuperclassRef &superclassRef : clazz->superclasses) {
			TypeMapping *typeMapping = typeMap->getTypeMappingByPossiblyPartiallyQualifiedCppTypeOrThrow(superclassRef.partialName, superclassRef.sourceLocation);
			shared_ptr<CppClass> superclass = cppProject->getClassByCppNameIfExists(typeMapping->typeNames.cppType);
			if (!superclass) throw ParseException(superclassRef.sourceLocation, sb() << "Base class must be a custom class: " << typeMapping->typeNames.cppType);
			superclassRef.fullyQualifiedName = typeMapping->typeNames.cppType;

			cppProject->addClassDependency(clazz, superclass.get());
		}
	});
}

void CppPartition::buildStructLayout(CppClass *c) {
	CHECK_NOT_NULL(c);

	if (c->classType != CppClassDeclarationType::STRUCT) {
		return;
	}

	if (c->layout) EXC(sb() << "Attempted to create layout for same class twice: " << c->typeNames.cppType);
	if (!c->superclasses.empty()) EXC(sb() << "Supertypes for a pass-by-value type are not supported: " << c->typeNames.cppType); // The layout is not defined in C++ standard and may depend on compiler.

	LOG_DEBUG(sb() << "Building class layout: " << c->typeNames.cppType);

	shared_ptr<CppStructLayout> layout = make_shared<CppStructLayout>();
	int64_t maxAlignment = max((size_t)1, c->typeMapping->alignment);
	int64_t nextOffset = 0;

	for (const shared_ptr<CppFuncVar> &sourceField : c->fields) {
		if (!sourceField->isStatic) {
			TypeMapping *typeMapping = sourceField->type;

			CppStructLayout::Field f = {};
			f.name = sourceField->declarationName;
			f.pointerDepth = sourceField->pointerDepth;
			f.arraySize = sourceField->arraySize;
			f.sourceLocation = sourceField->sourceLocation;
			f.comment = sourceField->comment;

			f.typeNames = typeMapping->typeNames;
			f.elementSize = typeMapping->size;
			f.alignment = max((int64_t)typeMapping->alignment, (int64_t)sourceField->explicitAlignment);

			if (f.pointerDepth > 0) {
				f.elementSize = config->getPointerSizeBytes();
				f.alignment = f.elementSize;
			}

			if (f.elementSize == 0) {
				errorList->addError(sourceField->sourceLocation, sb() << "Type size unknown: " << typeMapping->typeNames.cppType);
				return;
			}

			int64_t totalFieldSize = f.elementSize;
			if (f.arraySize > 0) {
				totalFieldSize *= f.arraySize;
			}

			if (f.alignment == 0) {
				f.alignment = 1;
			}

			maxAlignment = max(maxAlignment, f.alignment);
			nextOffset = alignOffsetToNextBoundary(nextOffset, f.alignment);

			f.offset = nextOffset;
			layout->fields.push_back(f);

			nextOffset += totalFieldSize;
		}
	}

	nextOffset = alignOffsetToNextBoundary(nextOffset, maxAlignment);
	layout->size = nextOffset;
	layout->alignment = maxAlignment;

	c->layout = layout;
	c->typeMapping->size = layout->size;
	c->typeMapping->alignment = layout->alignment;
}

void CppPartition::buildStructLayouts() {
	LOG_DETAIL(sb() << "Building struct layouts");

	list<CppClass *> buildQueue;
	forEachClass([&](CppClass *c) {
		// Add those classes to the build queue that don't have any dependencies.
		c->unprocessedDependeeClasses.insert(c->dependeeClasses.begin(), c->dependeeClasses.end());
		if (c->unprocessedDependeeClasses.empty()) {
			buildQueue.push_back(c);
		}
	});

	while (!buildQueue.empty()) {
		CppClass *c = buildQueue.front();
		buildQueue.pop_front();

		buildStructLayout(c);

		for (auto &dependent : c->dependentClasses) {
			dependent->unprocessedDependeeClasses.erase(c);
			if (dependent->unprocessedDependeeClasses.empty()) {
				buildQueue.push_back(dependent);
			}
		}
	}

	vector<string> unprocessedClasses;
	forEachClass([&](CppClass *c) {
		if (!c->unprocessedDependeeClasses.empty()) {
			unprocessedClasses.push_back(c->typeNames.cppType);
		}
	});
	if (!unprocessedClasses.empty()) {
		EXC(sb() << "Circular dependencies among classes/structs: " << unprocessedClasses);
	}
}

void CppPartition::resolveFunctions() {
	LOG_DETAIL("Resolving functions");
	for (auto &parsedFile : parsedFiles) {
		for (auto &func : parsedFile->unresolvedFunctions) {
			shared_ptr<CppFuncVar> functionDecl = funcVarParser->tryParseFunctionDeclaration(func->sourceCode->getSourceCode(), func->namespacePrefix);
			if (functionDecl) {
				functionDecl->comment = func->comment;

				if (func->enclosingClassIfExists) {
					func->enclosingClassIfExists->functions.push_back(functionDecl);
				}
				else {
					globalFunctions.push_back(functionDecl);
				}
				statistics.numFunctions++;
			}
		}
	}
}

void CppPartition::linkParsedFiles() {
	if (errorList->hasErrors()) return;

	StopWatch stopWatch = app->createStopWatchForPerformanceMeasurement();

	resolveClassMembers();
	stopWatch.mark("link / resolve class members");
	if (errorList->hasErrors()) return;

	resolveSuperclasses();
	stopWatch.mark("link / superclasses");
	if (errorList->hasErrors()) return;

	buildStructLayouts();
	stopWatch.mark("link / layouts");
	if (errorList->hasErrors()) return;

	resolveFunctions();
	stopWatch.mark("link / resolve functions");
	if (errorList->hasErrors()) return;
}

string CppPartition::getEnumReflectionVariableName(const CppClass *clazz, bool qualified) {
	string namespaceName;
	string unqualifiedName;
	cppParseUtil->cppNamespaceParser.separateNamespaceAndName(namespaceName, unqualifiedName, clazz->typeNames.cppType);
	return (qualified && !namespaceName.empty() ? namespaceName + "::" : "") + "enum_" + unqualifiedName;
}

void CppPartition::forEachParsedFile(const function<void(const shared_ptr<CppParsedFile> &)> &c) {
	for (const auto &parsedFile : parsedFiles) {
		c(parsedFile);
	}
}

void CppPartition::forEachGlobalFunction(const function<void(CppFuncVar *declaration)> &c) {
	for (auto &declaration : globalFunctions) {
		c(declaration.get());
	}
}

void CppPartition::forEachClass(const function<void(CppClass *clazz)> &c) {
	for (auto &entry : classes) {
		c(entry.get());
	}
}

void CppPartition::forEachEnum(const function<void(CppClass *clazz)> &c) {
	for (auto &entry : classes) {
		if (entry->classType == CppClassDeclarationType::ENUM) {
			c(entry.get());
		}
	}
}

void CppPartition::forEachSuperclassDepthFirst(CppClass *clazz, bool recursive, CppAccessModifier stricteningAccessModifier, const function<void(CppClass *, CppAccessModifier)> &c) {
	CHECK_NOT_NULL(clazz);
	for (const CppSuperclassRef &superclassRef : clazz->superclasses) {
		CppAccessModifier accessModifier = getStrictenedAccessModifier(superclassRef.accessModifier, stricteningAccessModifier);

		string superclassName = superclassRef.fullyQualifiedName;
		CppClass *superclass = cppProject->getClassByCppNameIfExists(superclassRef.fullyQualifiedName).get();
		if (!superclass) throw ParseException(clazz->sourceLocation, sb() << "Superclass not found: " << superclassName);

		if (recursive) {
			forEachSuperclassDepthFirst(superclass, recursive, accessModifier, c);
		}

		c(superclass, accessModifier);
	}
}

void CppPartition::forEachMemberFunction(CppClass *clazz, bool includeSuperclasses, CppAccessModifier stricteningAccessModifier, const function<void(CppClass *, CppFuncVar *, CppAccessModifier)> &c) {
	CHECK_NOT_NULL(clazz);
	if (includeSuperclasses) {
		forEachSuperclassDepthFirst(clazz, true, stricteningAccessModifier, [&](CppClass *functionClass, CppAccessModifier accessModifier) {
			forEachMemberFunction(functionClass, false, accessModifier, c);
		});
	}

	for (auto &func : clazz->functions) {
		c(clazz, func.get(), getStrictenedAccessModifier(func->accessModifier, stricteningAccessModifier));
	}
}

void CppPartition::forEachMemberVariable(CppClass *clazz, bool includeSuperclasses, CppAccessModifier stricteningAccessModifier, const function<void(CppClass *, CppStructLayout::Field *, CppAccessModifier)> &c) {
	CHECK_NOT_NULL(clazz);
	if (includeSuperclasses) {
		forEachSuperclassDepthFirst(clazz, true, stricteningAccessModifier, [&](CppClass *functionClass, CppAccessModifier accessModifier) {
			forEachMemberVariable(functionClass, false, accessModifier, c);
		});
	}

	for (auto &field : clazz->getLayoutOrThrow()->fields) {
		c(clazz, &field, getStrictenedAccessModifier(field.accessModifier, stricteningAccessModifier));
	}
}

string CppPartition::getFunctionEntryPoint(CppFuncVar *func, CppClass *enclosingClassIfNotGlobal) {
	string contextName; // Fully-qualified class name, namespace name (for global functions), or other unique context name.
	if (enclosingClassIfNotGlobal) {
		contextName = enclosingClassIfNotGlobal->typeNames.cppType;
	}
	else {
		contextName = func->namespacePrefixIfGlobal + "global";
	}

	auto iter = func->functionEntryPointNameByContext.find(contextName);
	if (iter != func->functionEntryPointNameByContext.end()) return iter->second;

	string transformedContextName = contextName;
	boost::replace_all(transformedContextName, "::", "_");

	string entryPoint = cppProject->reserveCppGlobalIdentifier(sb() << "rabbitcall_" << transformedContextName << "_" << func->declarationName);
	func->functionEntryPointNameByContext[contextName] = entryPoint;
	return entryPoint;
}

vector<CppFuncVar *> CppPartition::getAccessibleMemberFunctions(CppClass *clazz) {
	vector<CppFuncVar *> result;
	forEachMemberFunction(clazz, true, CppAccessModifier::UNKNOWN, [&](CppClass *functionClass, CppFuncVar *func, CppAccessModifier accessModifier) {
		if (accessModifier == CppAccessModifier::PUBLIC) {
			bool foundSimilarFunction = false;
			for (auto &existingFuncListEntry : result) {
				if (func->declarationName == existingFuncListEntry->declarationName && CppItemParseUtil::isSameTypeSignature(func, existingFuncListEntry)) {
					foundSimilarFunction = true;
					existingFuncListEntry = func; // Replace the existing function in the list with the overriding function from the derived class.
					break;
				}
			}
			if (!foundSimilarFunction) {
				result.push_back(func);
			}
		}
	});
	return result;
}


