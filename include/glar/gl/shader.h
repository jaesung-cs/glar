#ifndef GLAR_GL_SHADER_H_
#define GLAR_GL_SHADER_H_

#include <string>

#include <glad/glad.h>

#include <glm/fwd.hpp>

namespace glar
{
namespace gl
{
class Shader
{
public:
  Shader() = delete;
  Shader(const std::string& dirpath, const std::string& name);
  ~Shader();

  void Use();
  void UniformMatrix4f(const std::string& name, const glm::mat4& m);
  void UniformMatrix3f(const std::string& name, const glm::mat3& m);
  void Uniform4f(const std::string& name, const glm::vec4& v);
  void Uniform1i(const std::string& name, int value);

private:
  GLuint program_ = 0;
};
}
}

#endif // GLAR_GL_SHADER_H_
