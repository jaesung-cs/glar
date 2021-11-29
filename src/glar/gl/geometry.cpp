#include <glar/gl/geometry.h>

namespace glar
{
namespace gl
{
Geometry::Geometry(const std::vector<float>& vertexBuffer, std::initializer_list<Attribute> attributes, const std::vector<uint32_t>& indexBuffer, GLenum drawMode)
  : drawMode_(drawMode)
{
  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

  glGenBuffers(2, buffers_);
  glBindBuffer(GL_ARRAY_BUFFER, buffers_[0]);
  glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(float), vertexBuffer.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers_[1]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.size() * sizeof(uint32_t), indexBuffer.data(), GL_STATIC_DRAW);

  for (const auto& attribute : attributes)
  {
    glVertexAttribPointer(attribute.index, attribute.size, GL_FLOAT, GL_FALSE, attribute.stride * sizeof(float), (void*)(attribute.offset * sizeof(float)));
    glEnableVertexAttribArray(attribute.index);
  }

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  elementCount_ = indexBuffer.size();
}

Geometry::~Geometry()
{
  glDeleteBuffers(2, buffers_);
}

void Geometry::Draw()
{
  glBindVertexArray(vao_);
  glDrawElements(drawMode_, elementCount_, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}
}
}
