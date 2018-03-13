#include "../include/shaderloader.h"

namespace ShaderLoader
{
	const GLchar* load_code(const std::string& path)
	{
		std::fstream file;
		file.open(path, std::ios_base::in);

		if(!file.is_open()) {
			std::cout<<"No such .vs/.fs file!"<<std::endl;
			exit(0);
		}

		std::stringstream ss;

		while(!file.eof())
		{
			std::string buffer;
			getline(file, buffer);
			ss<<buffer<<std::endl;
		}

		file.close();

		//Copy to temporary place, which must be
		//deleted after (is it possible to change the
		//ownership of the char vector underlying ss, so
		//we don't have to copy?)
		std::string code = ss.str();
		GLchar* temp = new GLchar[code.length() + 1];
		strcpy(temp, code.c_str());

		return temp;
	}

	GLuint load_shader(const std::string& file, GLenum shaderType)
	{
		const GLchar* code = ShaderLoader::load_code(file);

		//Load code and compile
		GLuint id = glCreateShader(shaderType);
		glShaderSource(id, 1, &code, NULL);
		glCompileShader(id);

		//display any warning messages
		char buffer[1000];
		glGetShaderInfoLog(id, 1000, NULL, buffer);
		std::cout<<"Shader log: "<<buffer<<std::endl;

		delete[] code;

		return id;
	}

	GLuint load(const std::string& path)
	{
		std::string v_path(path);
		v_path.append(".vs");
		GLuint v_shader_id = ShaderLoader::load_shader(v_path, GL_VERTEX_SHADER);

		std::string f_path(path);
		f_path.append(".fs");
		GLuint f_shader_id = ShaderLoader::load_shader(f_path, GL_FRAGMENT_SHADER);

		//Create program, attach and link compiled shaders
		GLuint program_id = glCreateProgram();
		glAttachShader(program_id, v_shader_id);
		glAttachShader(program_id, f_shader_id);
		glLinkProgram(program_id);

		return program_id;
	}
}
