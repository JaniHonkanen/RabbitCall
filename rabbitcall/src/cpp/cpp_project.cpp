#include "pch.h"


CppProject::CppProject(Config *config, ErrorList *errorList)
	: config(config), errorList(errorList) {

	typeMap = make_shared<TypeMap>(config);

	shared_ptr<TypeMapping> stdFunctionTypeMapping = make_shared<TypeMapping>();
	stdFunctionTypeMapping->typeNames.cppType = "std::function";
	typeMap->addTypeMapping(stdFunctionTypeMapping);

	for (const Config::TypeMapping &m : config->typeMappings) {

		shared_ptr<TypeMapping> typeMapping = make_shared<TypeMapping>();
		typeMapping->typeNames = m.typeNames;
		typeMapping->marshal = m.marshal;
		typeMapping->charType = m.charType;
		typeMapping->size = m.size;
		typeMapping->alignment = m.alignment;
		typeMapping->sourceLocation = m.sourceLocation;
		typeMap->addTypeMapping(typeMapping);
	}
}

CppProject::~CppProject() {
}

bool CppProject::isFileInsideAnyOutputDir(const string &path) {
	for (const string &dir : outputDirs) {
		if (isPathSameOrDescendantOf(path, dir)) {
			return true;
		}
	}
	return false;
}

void CppProject::checkFileNotInsideAnotherPartition(const Path &path, CppPartition *partition) {
	for (const auto &otherPartition : partitions) {
		if (otherPartition.get() != partition) {
			if (otherPartition->containsPath(path)) EXC(sb() << "Two partitions (" << partition->getName() << " and " << otherPartition->getName() << ") cannot contain the same path: " << path);
		}
	}
}

string CppProject::reserveCppGlobalIdentifier(const string &basename) {
	if (usedCppGlobalIdentifiers.insert(basename).second) return basename;
	// Add a number suffix to the name to get a unique identifier.
	for (int i = 1;; i++) {
		string name = basename + to_string(i);
		if (usedCppGlobalIdentifiers.insert(name).second) return name;
	}
}

shared_ptr<CppPartition> CppProject::createPartitionIfDoesNotExist(const Config::Partition &partitionConfig) {
	string name = partitionConfig.name;
	auto iter = partitionsByName.find(name);
	if (iter != partitionsByName.end()) return nullptr;
	shared_ptr<CppPartition> partition = make_shared<CppPartition>(partitionConfig, this);
	partitionsByName[name] = partition;
	partitions.push_back(partition);

	return partition;
}

bool CppProject::addClassIfDoesNotExist(shared_ptr<CppClass> clazz, CppPartition *partition) {
	string name = clazz->typeNames.cppType;
	auto iter = classesByName.find(name);
	if (iter != classesByName.end()) return false;
	classesByName[name] = clazz;
	partition->addClass(clazz);
	return true;
}

shared_ptr<CppClass> CppProject::getClassByCppNameIfExists(const string &cppName) {
	const string &name = cppName;
	auto iter = classesByName.find(name);
	if (iter == classesByName.end()) return nullptr;
	return iter->second;
}

void CppProject::addClassDependency(CppClass *dependent, CppClass *dependee) {
	CHECK_NOT_NULL(dependent);
	CHECK_NOT_NULL(dependee);
	LOG_DEBUG(sb() << "Adding class dependency: " << dependent->typeNames.cppType << " -> " << dependee->typeNames.cppType);
	dependee->dependentClasses.insert(dependent);
	dependent->dependeeClasses.insert(dependee);
}

void CppProject::applyFileParseResult(shared_ptr<CppParsedFile> parsedFile, CppPartition *partition) {
	errorList->addAll(&parsedFile->errorList);

	for (const shared_ptr<CppClass> &clazz : parsedFile->classes) {
		if (!addClassIfDoesNotExist(clazz, partition)) throw ParseException(clazz->sourceLocation, sb() << "Multiple classes exported with the same name: " << clazz->typeNames.cppType);

		// The "explicit alignment" here means the alignas() keyword and the alignment of the type mapping may be updated later when the layout is built.
		shared_ptr<TypeMapping> typeMapping = make_shared<TypeMapping>();
		typeMapping->typeNames = clazz->typeNames;
		typeMapping->alignment = clazz->explicitAlignment;
		typeMapping->isPassByValue = clazz->isPassByValue;
		typeMapping->sourceLocation = clazz->sourceLocation;
		typeMapping->partitionName = partition ? partition->getName() : "";
		typeMap->addTypeMapping(typeMapping);
		clazz->typeMapping = typeMapping;
	}

	partition->applyFileParseResult(parsedFile);
}

void CppProject::linkParsedFiles() {
	typeMap->allTypesIdentified = true;

	for (const auto &partition : partitions) {
		partition->linkParsedFiles();
	}
}

void CppProject::generateOutputFiles() {
	// Check whether CRLF or LF line-breaks are more popular in the source code and use the same ones in generated code.
	bool shouldUseCrLfLineBreaks = lineBreakCountsInSource.crLfCount > lineBreakCountsInSource.lfCount;
	LOG_DEBUG(sb() << "Total line breaks in source: CR-LF: " << lineBreakCountsInSource.crLfCount << ", LF: " << lineBreakCountsInSource.lfCount);

	if (!partitions.empty()) {
		// Consider the first partition the "main" partition whose generated files contain all the common definitions.
		shared_ptr<CppPartition> mainPartition = partitions.at(0);
		
		for (const auto &partition : partitions) {
			auto updateFile = [&](OutputFileGenerator &&generator) {
				generator.init(partition.get(), mainPartition.get());
				
				CppOutputFile *outputFile = generator.getOutputFile();
				if (outputFile && !outputFile->file.empty()) {
					StopWatch fileStopWatch = app->createStopWatchForPerformanceMeasurement();
					shared_ptr<StringBuilder> output = make_shared<StringBuilder>();
					generator.generateOutput(*output);
					fileStopWatch.mark("output / generate");
					generator.updateGeneratedUtf8FileIfModified(outputFile->file, output->buffer.data(), output->buffer.size(), outputFile->shouldWriteByteOrderMark, shouldUseCrLfLineBreaks, config->dryRunMode);
					fileStopWatch.mark("output / update file");
				}
			};

			updateFile(HeaderOutputGenerator());
			updateFile(CppOutputGenerator());
			updateFile(CsOutputGenerator());
			updateFile(HlslOutputGenerator());
			updateFile(GlslOutputGenerator());
		}
	}
}

CppStatistics CppProject::calculateTotalStatistics() {
	CppStatistics stats;
	for (const auto &m : partitions) {
		stats.add(m->getStatistics());
	}
	return stats;
}

void CppProject::processProject() {
	if (processed) EXC("Project already processed");
	processed = true;
	
	StopWatch stopWatch = app->createStopWatchForPerformanceMeasurement();

	LOG_DETAIL("Initializing");

	for (const auto &entry : config->outputDirsByType) {
		const Config::OutputDir &outputDir = entry.second;
		if (outputDir.isValid()) {
			outputDirs.insert(outputDir.path.toString());
		}
	}
	for (const Config::Partition &partitionConfig : config->partitions) {
		string name = partitionConfig.name;
		shared_ptr<CppPartition> partition = createPartitionIfDoesNotExist(partitionConfig);
		if (!partition) EXC(sb() << "Two partitions with the same name: " << name);
	}
	stopWatch.mark("init");
	if (errorList->hasErrors()) return;

	LOG_DETAIL("Finding source files");
	{
		ThreadPool::TaskSet tasks(app->threadPool.get());
		for (const auto &partition : partitions) {
			partition->startFindSourceFiles(&tasks);
		}
		tasks.waitUntilEmpty();

		for (const auto &partition : partitions) {
			partition->finishFindSourceFiles();
		}
	}
	stopWatch.mark("find source files");
	if (errorList->hasErrors()) return;

	LOG_DETAIL("Parsing input files");
	{
		ThreadPool::TaskSet tasks(app->threadPool.get());
		for (const auto &partition : partitions) {
			partition->forEachParsedFile([&](const shared_ptr<CppParsedFile> &parsedFile) {
				tasks.addTask([this, parsedFile]() {
					errorList->runWithExceptionCheck([&] {
						CppFileParser::parseFile(parsedFile.get(), config);
					});
				});
			});
		}
		tasks.waitUntilEmpty();
	}
	stopWatch.mark("read source files / parseFile");
	if (errorList->hasErrors()) return;

	for (const auto &partition : partitions) {
		partition->forEachParsedFile([&](const shared_ptr<CppParsedFile> &parsedFile) {
			applyFileParseResult(parsedFile, partition.get());
			if (parsedFile->file->lineAndColumnNumberMap) {
				lineBreakCountsInSource.add(parsedFile->file->lineAndColumnNumberMap->getLineBreakCounts());
			}
		});
	}
	stopWatch.mark("read source files / apply file");
	if (errorList->hasErrors()) return;

	// Link the source files together and resolve type dependencies.
	LOG_DETAIL("Linking");
	linkParsedFiles();
	stopWatch.mark("link");
	if (errorList->hasErrors()) return;

	LOG_DETAIL("Generating output files");
	generateOutputFiles();
	stopWatch.mark("output");
}

void CppProject::forEachPartition(const function<void(CppPartition *)> &c) {
	for (const shared_ptr<CppPartition> &p : partitions) {
		c(p.get());
	}
}




