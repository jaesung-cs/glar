#include <glar/gl/shader.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <glm/glm.hpp>

namespace glar
{
namespace gl
{
namespace
{
bool FileExists(const std::string& filepath)
{
  std::ifstream in(filepath);
  return in.is_open();
}

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

  GLuint geometryShader = 0;
  const auto geomShaderFilepath = dirpath + "\\" + name + ".geom";
  if (FileExists(geomShaderFilepath))
  {
    geometryShader = LoadShaderModule(geomShaderFilepath, GL_GEOMETRY_SHADER);
    glAttachShader(program_, geometryShader);
  }

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

  if (geometryShader)
    glDeleteShader(geometryShader);
}

Shader::~Shader()
{
  glDeleteProgram(program_);
}

void Shader::Use()
{
  glUseProgram(program_);
}

void Shader::UniformMatrix4f(const std::string& name, const glm::mat4& m)
{
  glUniformMatrix4fv(glGetUniformLocation(program_, name.c_str()), 1, GL_FALSE, &m[0][0]);
}

void Shader::UniformMatrix3f(const std::string& name, const glm::mat3& m)
{
  glUniformMatrix3fv(glGetUniformLocation(program_, name.c_str()), 1, GL_FALSE, &m[0][0]);
}

void Shader::Uniform4f(const std::string& name, const glm::vec4& v)
{
  glUniform4fv(glGetUniformLocation(program_, name.c_str()), 1, &v[0]);
}

void Shader::Uniform1i(const std::string& name, int value)
{
  glUniform1i(glGetUniformLocation(program_, name.c_str()), value);
}
}
}
