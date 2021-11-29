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
  Texture();
  Texture(uint32_t width, uint32_t height);
  ~Texture();

  bool Valid() const;

  void Bind();
  void Bind(int index);

  void UpdateStorage(uint32_t width, uint32_t height);
  void Update(void* pixels, GLenum format);

private:
  GLuint texture_ = 0;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
};
}
}

#endif // GLAR_GL_TEXTURE_H_
