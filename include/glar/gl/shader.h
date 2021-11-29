#ifndef GLAR_GL_SHADER_H_
#define GLAR_GL_SHADER_H_

#include <string>

#include <glad/glad.h>

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
  void Uniform1i(const std::string& name, int value);

private:
  GLuint program_ = 0;
};
}
}

#endif // GLAR_GL_SHADER_H_
