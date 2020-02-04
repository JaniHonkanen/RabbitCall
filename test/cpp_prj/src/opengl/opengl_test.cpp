#include "pch.h"
#include "main_tests.h"
#include "opengl_test.h"

#pragma warning(push)
#pragma warning(disable : 4005)
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#pragma warning(pop)

using namespace std;

#define CHECK_GLFW_ERROR(p) if ((p) == 0) throw runtime_error("GLFW call failed: " #p)
#define CHECK_OPENGL_ERROR(p) p; checkOpenGlError(#p);

void handleGlfwError(int error, const char* description) {
	throw runtime_error(string("GLFW error: ") + to_string(error) + ": " + description);
}

void checkOpenGlError(const char *location) {
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		throw runtime_error(string("OpenGL error: ") + to_string(error) + " at " + location);
	}
}

void printShaderCompileInfoLog(GLuint shader, const char *shaderName) {
	const int logSize = 4096;
	GLchar log[logSize];
	GLsizei length;
	glGetShaderInfoLog(shader, logSize, &length, log);
	if (length > 0) {
		cout << "Compilation output for " << shaderName << ":" << endl;
		cout << string((const char *)log, (size_t)length) << endl;
	}
}

struct Vertex {
	float x, y;
};

void testOpenGl(const std::string &projectDir) {
	string shaderIncludeFile = readFile(combinePath(projectDir, "csharp_prj/src/rabbitcall/rabbitcall_generated_partition1.glsl"));

	CHECK_GLFW_ERROR(glfwInit());

	glfwSetErrorCallback(handleGlfwError);

	int width = 16, height = 16;
	int numPixels = width * height;
	auto imageData = make_unique<float4[]>(numPixels);
	for (int i = 0; i < numPixels; i++) {
		imageData[i] = float4(10, 10, 10, 10);
	}

	// An error "GLX: Failed to create context: GLXBadFBConfig" probably means that the requested OpenGL version is not available on the OS.
	GLFWwindow *window;
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	CHECK_GLFW_ERROR(window = glfwCreateWindow(width, height, "OpenGL window", NULL, NULL));
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	CHECK_OPENGL_ERROR(glfwSwapInterval(1));

	GLuint frameBuffer;
	CHECK_OPENGL_ERROR(glGenFramebuffers(1, &frameBuffer));
	CHECK_OPENGL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer));

	GLuint texture;
	CHECK_OPENGL_ERROR(glGenTextures(1, &texture));
	CHECK_OPENGL_ERROR(glBindTexture(GL_TEXTURE_2D, texture));
	CHECK_OPENGL_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, imageData.get()));

	CHECK_OPENGL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0));

	GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
	CHECK_OPENGL_ERROR(glDrawBuffers(1, &drawBuffer));

	CHECK_OPENGL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer));
	CHECK_OPENGL_ERROR(glViewport(0, 0, width, height));
	CHECK_OPENGL_ERROR(glClear(GL_COLOR_BUFFER_BIT));

	Vertex vertices[] = {
		{ -0.6f, -0.4f },
		{  0.6f, -0.4f },
		{  0.0f,  0.6f }
	};
	GLuint vertexBuffer;
	CHECK_OPENGL_ERROR(glGenBuffers(1, &vertexBuffer));
	CHECK_OPENGL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
	CHECK_OPENGL_ERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

	string vertexShaderCode =
		"#version 330\n"
		"in vec2 vertexPos;\n"
		"out vec3 color;\n"
		"\n"
		+ shaderIncludeFile +
		"\n"
		"void main() {\n"
		"    gl_Position = vec4(vertexPos, 0.0, 1.0);\n"
		"    color = vec3(constants.g_v1.x, test_VALUE2, constants.g_s.v4.w);\n"
		"}\n";

	string fragmentShaderCode =
		"#version 330\n"
		"in vec3 color;\n"
		"out vec4 fragColor;\n"
		"\n"
		"void main() {\n"
		"    fragColor = vec4(color, 1.0);\n"
		"}\n";

	CHECK_OPENGL_ERROR(GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER));
	const char *vertexShaderCodePtr = vertexShaderCode.c_str();
	CHECK_OPENGL_ERROR(glShaderSource(vertexShader, 1, &vertexShaderCodePtr, NULL));
	CHECK_OPENGL_ERROR(glCompileShader(vertexShader));
	printShaderCompileInfoLog(vertexShader, "vertex shader");

	CHECK_OPENGL_ERROR(GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER));
	const char *fragmentShaderCodePtr = fragmentShaderCode.c_str();
	CHECK_OPENGL_ERROR(glShaderSource(fragmentShader, 1, &fragmentShaderCodePtr, NULL));
	CHECK_OPENGL_ERROR(glCompileShader(fragmentShader));
	printShaderCompileInfoLog(fragmentShader, "fragment shader");

	CHECK_OPENGL_ERROR(GLuint program = glCreateProgram());
	CHECK_OPENGL_ERROR(glAttachShader(program, vertexShader));
	CHECK_OPENGL_ERROR(glAttachShader(program, fragmentShader));
	CHECK_OPENGL_ERROR(glLinkProgram(program));

	GLuint vao;
	CHECK_OPENGL_ERROR(glGenVertexArrays(1, &vao));
	CHECK_OPENGL_ERROR(glBindVertexArray(vao));

	CHECK_OPENGL_ERROR(GLint vertexPosAttrLocation = glGetAttribLocation(program, "vertexPos"));
	if (vertexPosAttrLocation == -1) throw runtime_error("Vertex position attribute not found");

	CHECK_OPENGL_ERROR(GLuint uniformBlockIndex = glGetUniformBlockIndex(program, "GpuConstantBuffer"));
	if (uniformBlockIndex == GL_INVALID_INDEX) throw runtime_error("Uniform block not found");

	GpuConstantBuffer uboData = {};
	uboData.v1 = float2(5, 6);
	uboData.s.v4.w = 8;
	
	GLuint uniformBuffer;
	CHECK_OPENGL_ERROR(glGenBuffers(1, &uniformBuffer));
	CHECK_OPENGL_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer));
	CHECK_OPENGL_ERROR(glBufferData(GL_UNIFORM_BUFFER, sizeof(uboData), &uboData, GL_STATIC_DRAW));
	
	GLuint uboBinding = 0;
	glUniformBlockBinding(program, uniformBlockIndex, uboBinding);
	glBindBufferBase(GL_UNIFORM_BUFFER, uboBinding, uniformBuffer);

	CHECK_OPENGL_ERROR(glVertexAttribPointer(vertexPosAttrLocation, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr));
	CHECK_OPENGL_ERROR(glEnableVertexAttribArray(vertexPosAttrLocation));

	CHECK_OPENGL_ERROR(glUseProgram(program));
	CHECK_OPENGL_ERROR(glDrawArrays(GL_TRIANGLES, 0, 3));

	CHECK_OPENGL_ERROR(glBindTexture(GL_TEXTURE_2D, texture));
	CHECK_OPENGL_ERROR(glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, imageData.get()));

	// Check that the color values written by the shader are as expected.
	float4 centerPixel = imageData[height / 2 * width + width / 2];
	if (centerPixel.x != 5) throw runtime_error(string("Wrong value from uniform buffer: expected 5, got ") + to_string(centerPixel.x));
	if (centerPixel.y != 2) throw runtime_error(string("Wrong value from enum: expected 2, got ") + to_string(centerPixel.y));
	if (centerPixel.z != 8) throw runtime_error(string("Wrong value from struct: expected 8, got ") + to_string(centerPixel.z));

	checkOpenGlError("end");

	glfwDestroyWindow(window);
	glfwTerminate();
}


