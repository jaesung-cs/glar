#include <glar/gl/texture.h>

namespace glar
{
namespace gl
{
Texture::Texture(uint32_t width, uint32_t height)
  : width_(width)
  , height_(height)
{
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, width, height);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Texture::~Texture()
{
  glDeleteTextures(1, &texture_);
}

void Texture::Bind()
{
  glBindTexture(GL_TEXTURE_2D, texture_);
}

void Texture::Bind(int index)
{
  Bind();
  glActiveTexture(GL_TEXTURE0 + index);
}

void Texture::Update(void* pixels, GLenum format)
{
  Bind();
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, format, GL_UNSIGNED_BYTE, pixels);
}
}
}
