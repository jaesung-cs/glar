#ifndef GLAR_GL_TEXTURE_H_
#define GLAR_GL_TEXTURE_H_

#include <cstdint>

#include <glad/glad.h>

namespace glar
{
namespace gl
{
class Texture
{
public:
  Texture() = delete;
  Texture(uint32_t width, uint32_t height);
  ~Texture();

  void Bind();
  void Bind(int index);

  void Update(void* pixels, GLenum format);

private:
  GLuint texture_ = 0;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
};
}
}

#endif // GLAR_GL_TEXTURE_H_
