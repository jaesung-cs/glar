#include <glar/gl/texture.h>

#include <iostream>

namespace glar
{
namespace gl
{
Texture::Texture()
{
}

Texture::Texture(uint32_t width, uint32_t height)
{
  UpdateStorage(width, height);
}

Texture::~Texture()
{
  if (texture_)
    glDeleteTextures(1, &texture_);
}

bool Texture::Valid() const
{
  return texture_ != 0;
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

void Texture::UpdateStorage(uint32_t width, uint32_t height)
{
  if (width_ != width || height_ != height)
  {
    std::cout << "Update storage" << std::endl;

    width_ = width;
    height_ = height;

    if (texture_)
      glDeleteTextures(1, &texture_);

    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, width, height);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
}

void Texture::Update(void* pixels, GLenum format)
{
  Bind();
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, format, GL_UNSIGNED_BYTE, pixels);
}
}
}
