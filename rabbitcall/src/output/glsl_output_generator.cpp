#include "pch.h"


void GlslOutputGenerator::outputStructOrInterfaceBlock(CppClass *clazz, StringBuilder &output) {
	if (!clazz->typeMapping->isPassByValue) return;
	CppExportParameters::GlslParameters glslParameters = clazz->exportParameters->glslParameters;

	output.appendLine("");
	outputComment(clazz->comment.get(), output);
	{
		string storageType = glslParameters.storage;
		if (storageType.empty()) storageType = "struct";

		output.appendIndent();

		// Build the layout attribute if needed.
		{
			StringJoiner joiner(&output, ", ");
			joiner.prefixIfNotEmpty = "layout(";
			joiner.suffixIfNotEmpty = ") ";

			if (storageType != "struct") {
				joiner.append("std140");
			}

			if (!glslParameters.binding.empty()) {
				joiner.append(sb() << "binding = " << glslParameters.binding);
			}

			joiner.finish();
		}

		output << storageType;
		output << " ";
		output << clazz->typeMapping->typeNames.glslType;
		output << " {\n";
	}
	output.changeIndent(+1);

	int64_t offset = 0;
	for (auto &field : clazz->getLayoutOrThrow()->fields) {
		if (field.pointerDepth > 0) throw ParseException(field.sourceLocation, "Pointer not allowed in a struct that is exported to GLSL");
		if (field.elementSize % 4 != 0) throw ParseException(field.sourceLocation, "GLSL struct member size must be a multiple of 4 bytes");

		string typeName = field.typeNames.glslType;
		if (typeName.empty()) throw ParseException(field.sourceLocation, sb() << "GLSL data type not defined for C++ type (in configuration or in exported struct): " << field.typeNames.cppType);

		bool isArray = field.arraySize > 0;
		if (isArray && field.elementSize % 16 != 0) throw ParseException(field.sourceLocation, "Array not allowed in a struct that is exported to GLSL, unless the element size is divisible by 16 bytes (because GLSL would round the element size up to 16 bytes anyway)");
		int64_t size = field.elementSize * max((int64_t)1, field.arraySize);

		int64_t alignment = 4;
		if (size > 4) alignment = 8;
		if (size > 8) alignment = 16;
		offset = alignOffsetToNextBoundary(offset, alignment);

		if (offset != field.offset) throw ParseException(field.sourceLocation, sb() << "Field '" << field.name << "' offset would be different in GLSL (" << offset << ") than in C++ (" << field.offset << ")");

		outputComment(field.comment.get(), output);

		output.appendIndent();
		output << typeName;
		output << " ";
		output << glslParameters.fieldPrefix;
		output << field.name;
		if (isArray) {
			output << "[" << field.arraySize << "]";
		}
		output << ";\n";

		offset += size;
	}
	output.changeIndent(-1);

	output.appendIndent();
	output << "}";
	if (!glslParameters.instanceName.empty()) {
		output << " " << glslParameters.instanceName;
	}
	output << ";\n";
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GlslOutputGenerator::outputEnum(CppClass *clazz, StringBuilder &output) {
	CppExportParameters::GlslParameters glslParameters = clazz->exportParameters->glslParameters;
	string prefix = glslParameters.fieldPrefix;
	if (prefix.empty()) {
		prefix = sb() << clazz->typeNames.cppType << "_";
	}

	output.appendLine("");

	if (outputComment(clazz->comment.get(), output)) {
		output.appendLine("");
	}

	for (auto &field : clazz->enumFields) {
		if (field.value.empty()) throw ParseException(field.sourceLocation, "Uninitialized enum fields not supported in GLSL");
		outputComment(field.comment.get(), output);
		output.appendIndent() << "const int " << prefix << field.name << " = " << field.value << ";\n";
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GlslOutputGenerator::generateOutput(StringBuilder &output) {
	StopWatch stopWatch = app->createStopWatchForPerformanceMeasurement();

	partition->forEachClass([&](CppClass *clazz) {
		if (clazz->exportParameters->glslParameters.exportGlsl) {
			if (clazz->classType == CppClassDeclarationType::CLASS || clazz->classType == CppClassDeclarationType::STRUCT) {
				outputStructOrInterfaceBlock(clazz, output);
			}
			else if (clazz->classType == CppClassDeclarationType::ENUM) {
				outputEnum(clazz, output);
			}
		}
		});

	stopWatch.mark("output / generate .glsl");
}


