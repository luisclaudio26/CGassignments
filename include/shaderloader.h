#ifndef _SHADER_LOADER_H_
#define _SHADER_LOADER_H_

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdlib>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace ShaderLoader
{
	const GLchar* load_code(const std::string& path);
	GLuint load_shader(const std::string& file, GLenum shaderType);
	GLuint load(const std::string& path);
}

#endif
