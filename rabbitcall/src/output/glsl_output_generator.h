#pragma once

class GlslOutputGenerator : public OutputFileGenerator {

	void outputStructOrInterfaceBlock(CppClass *clazz, StringBuilder &output);
	void outputEnum(CppClass *clazz, StringBuilder &output);

public:
	GlslOutputGenerator() = default;
	DISABLE_COPY_AND_MOVE(GlslOutputGenerator);
	virtual ~GlslOutputGenerator() = default;

	string getFileType() override { return "glsl"; }
	void generateOutput(StringBuilder &output) override;
};
