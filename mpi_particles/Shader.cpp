// functions to load shaders

#include "Shader.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#ifdef _WIN32
// don't complain if we use standard IO functions instead of windows-only
#pragma warning( disable: 4996 )
#endif

// load and compile a single shader from sh.file
// sh.id is an existing shader object
// shader type is defined by shader object type
static bool loadShader(const ShaderInfo &sh) {
	// open file
	FILE *fp = fopen(sh.file, "rb");
	if (! fp) {
		perror(sh.file);		// print what happened
		return false;			// return failure
	}

	// get file size: seek to end of file is more cross-platform than fstat
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// read entire file and compile as shader
	GLchar *shader = new GLchar[size];
	fread(shader, 1, size, fp);
	fclose(fp);
	glShaderSource(sh.id, 1, (const GLchar**)(&shader), &size);
	glCompileShader(sh.id);
	delete[] shader;

	// report compile errors
	GLint success;
	glGetShaderiv(sh.id, GL_COMPILE_STATUS, &success);
	if (! success) {
		// how big is the message?
		GLsizei infoLen;
		glGetShaderiv(sh.id, GL_INFO_LOG_LENGTH, &infoLen);

		// print the message
		char *infoLog = new char[infoLen];
		glGetShaderInfoLog(sh.id, infoLen, 0, infoLog);
		fprintf(stderr, "%s:\n%s", sh.file, infoLog);

		// free the message buffer
		delete[] infoLog;
		return false;			// return failure
	 }

	return true;				// success
}


// load a set of shaders
bool loadShaders(unsigned int progID, const ShaderList &components) {
	// load and link shader code
	for(ShaderList::const_iterator ci = components.begin(); ci != components.end(); ++ci) {
		if (! loadShader(*ci)) return false; // return failure
		glAttachShader(progID, ci->id);
	}
	glLinkProgram(progID);

	// report link errors
	GLint success;
	glGetProgramiv(progID, GL_LINK_STATUS, &success);
	if (! success) {
		// how big is the message?
		GLsizei infoLen;
		glGetProgramiv(progID, GL_INFO_LOG_LENGTH, &infoLen);

		// print the message
		char *infoLog = new char[infoLen];
		glGetProgramInfoLog(progID, infoLen, 0, infoLog);
		fprintf(stderr, "link:\n%s", infoLog);

		// free the message buffer
		delete[] infoLog;
		return false;			// return failure
	}

	return true;				// success
}
