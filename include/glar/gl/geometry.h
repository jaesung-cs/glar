#ifndef GLAR_GL_GEOMETRY_H_
#define GLAR_GL_GEOMETRY_H_

#include <vector>
#include <initializer_list>

#include <glad/glad.h>

namespace glar
{
namespace gl
{
struct Attribute
{
  int index;
  int size;
  int stride = 0; // With respect to float
  int offset = 0; // With respect to float
};

class Geometry
{
public:
  Geometry() = delete;
  Geometry(const std::vector<float>& vertexBuffer, std::initializer_list<Attribute> attributes, const std::vector<uint32_t>& indexBuffer, GLenum drawMode);
  ~Geometry();

  void draw();

private:
  GLuint buffers_[2] = { 0, }; // 0: vertex, 1: index
  GLuint vao_ = 0;

  GLenum drawMode_ = GL_TRIANGLES;
  uint32_t elementCount_ = 0;
};
}
}

#endif // GLAR_GL_GEOMETRY_H_
