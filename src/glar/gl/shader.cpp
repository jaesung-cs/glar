#include <glar/gl/shader.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace glar
{
namespace gl
{
namespace
{
GLuint LoadShaderModule(const std::string& shaderFilepath, GLenum shaderStage)
{
  std::string code;
  {
    std::ifstream in(shaderFilepath);
    std::stringstream ss;
    ss << in.rdbuf();
    code = ss.str();
  }

  GLuint shader = glCreateShader(shaderStage);
  const char* codeString = code.c_str();
  glShaderSource(shader, 1, &codeString, NULL);
  glCompileShader(shader);

  GLint success;
  GLchar infoLog[1024];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(shader, 1024, NULL, infoLog);
    std::cout << "Failed to compile shader (" << shaderStage << "), error:" << std::endl
      << infoLog << std::endl;
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}
}

Shader::Shader(const std::string& dirpath, const std::string& name)
{
  const auto vertexShader = LoadShaderModule(dirpath + "\\" + name + ".vert", GL_VERTEX_SHADER);
  const auto fragmentShader = LoadShaderModule(dirpath + "\\" + name + ".frag", GL_FRAGMENT_SHADER);

  program_ = glCreateProgram();
  glAttachShader(program_, vertexShader);
  glAttachShader(program_, fragmentShader);
  glLinkProgram(program_);

  GLint success;
  GLchar infoLog[1024];
  glGetProgramiv(program_, GL_LINK_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(program_, 1024, NULL, infoLog);
    std::cout << "Failed to link shader program, error:" << std::endl
      << infoLog << std::endl;
    glDeleteShader(program_);
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
  glDeleteProgram(program_);
}

void Shader::use()
{
  glUseProgram(program_);
}

void Shader::uniform1i(const std::string& name, int value)
{
  glUniform1i(glGetUniformLocation(program_, "tex"), 0);
}
}
}
