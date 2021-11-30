#include <glar/scene/fractal_geometry.h>

#include <iostream>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace glar
{
namespace scene
{
namespace
{
float rand01()
{
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_real_distribution<float> dis(0.f, 1.f);

  return dis(gen);
}

float random(float a, float b)
{
  return a + (b - a) * rand01();
}
}

FractalGeometry::FractalGeometry(const Fractal& fractal)
  : fractal_(fractal)
{
  const auto& curves = fractal.curves();
  const auto& info = fractal.info();

  // 4 vertices for each step, plus 4 base
  const auto curveVertexCount = 4 * (info.steps + 1);
  const auto maxVertexCount = curves.size() * curveVertexCount;

  // 24 indices for each step
  const auto curveIndexCount = 24 * info.steps;
  const auto maxIndexCount = curves.size() * curveIndexCount;

  // 3 for position, 3 for color
  const auto vertexByteSize = 6 * sizeof(float);

  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

  glGenBuffers(2, buffers_);

  glBindBuffer(GL_ARRAY_BUFFER, buffers_[0]);
  glBufferData(GL_ARRAY_BUFFER, vertexByteSize * maxVertexCount, NULL, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers_[1]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * maxIndexCount, NULL, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 0));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 3));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // Fill the buffer
  UpdateAnimation(0.f);
}

FractalGeometry::~FractalGeometry()
{
  glDeleteVertexArrays(1, &vao_);
  glDeleteBuffers(2, buffers_);
}

void FractalGeometry::Draw()
{
  glBindVertexArray(vao_);
  glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void FractalGeometry::UpdateAnimation(float animationTime)
{
  vertexBuffer_.clear();
  indexBuffer_.clear();

  for (const auto& curve : fractal_.curves())
    AppendToBuffer(curve, animationTime);

  // Move to gl buffer
  glBindBuffer(GL_ARRAY_BUFFER, buffers_[0]);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertexBuffer_.size(), vertexBuffer_.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers_[1]);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(float) * indexBuffer_.size(), indexBuffer_.data());
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  indexCount_ = indexBuffer_.size();
}

void FractalGeometry::AppendToBuffer(const Fractal::Curve& curve, float animationTime)
{
  const auto indexOffset = vertexBuffer_.size() / 6;

  const auto& info = curve.info();

  // Animation time to length
  constexpr float pi = 3.1415926535897932384626433832795f;
  constexpr float period = 5.f;
  const auto length = (-std::cos(2.f * pi * animationTime / period) + 1.f) / 2.f * info.maxLength + 2.f;

  // Counter clockwise
  constexpr auto radius = 0.5f;
  const std::vector<glm::vec3> ring = {
    glm::vec3(-1.f, -1.f, 0.f) * radius,
    glm::vec3(1.f, -1.f, 0.f) * radius,
    glm::vec3(1.f, 1.f, 0.f) * radius,
    glm::vec3(-1.f, 1.f, 0.f) * radius,
  };

  const auto curveLength = std::min<float>(info.steps, std::max(length - curve.startOffset, 0.f));
  if (curveLength <= 0.f)
    return;

  const auto steps = static_cast<int>(curveLength);

  const auto restLength = curveLength - steps;
  glm::mat4 transform = glm::mat4(1.f);
  glm::vec3 color = glm::vec3(0.5f, 0.5f, 0.5f);
  for (int i = 0; i <= steps; i++)
  {
    float ringScaleFactor = 1.f;

    // To make tip sharp
    if (i == steps)
      ringScaleFactor = restLength;

    // Root
    if (i == 0 && curve.startOffset == 0.f)
      ringScaleFactor = 2.f;

    for (const auto& ringVertex : ring)
    {
      const auto v = curve.base * transform * glm::vec4(ringVertex * ringScaleFactor, 1.f);

      vertexBuffer_.push_back(v.x);
      vertexBuffer_.push_back(v.y);
      vertexBuffer_.push_back(v.z);
      vertexBuffer_.push_back(color.r);
      vertexBuffer_.push_back(color.g);
      vertexBuffer_.push_back(color.b);
    }

    if (i < steps)
    {
      transform =
        glm::translate(glm::vec3(0.f, 0.f, info.height / info.steps))
        * glm::toMat4(glm::angleAxis(info.curveAngle / info.steps, glm::vec3(1.f, 0.f, 0.f)))
        * glm::scale(glm::vec3(std::exp(-info.scaleCoeff)))
        * transform;
    }
  }

  // Vertex at end
  const auto v = glm::vec3(curve.base * transform * glm::vec4(0.f, 0.f, restLength * info.height / info.steps, 1.f));
  vertexBuffer_.push_back(v.x);
  vertexBuffer_.push_back(v.y);
  vertexBuffer_.push_back(v.z);
  vertexBuffer_.push_back(color.r);
  vertexBuffer_.push_back(color.g);
  vertexBuffer_.push_back(color.b);

  for (int i = 0; i < steps; i++)
  {
    for (int j = 0; j < ring.size(); j++)
    {
      const auto m = ring.size();
      const auto j0 = j;
      const auto j1 = (j + 1) % m;

      indexBuffer_.push_back(indexOffset + i * m + j0);
      indexBuffer_.push_back(indexOffset + i * m + j1);
      indexBuffer_.push_back(indexOffset + i * m + j0 + m);

      indexBuffer_.push_back(indexOffset + i * m + j0 + m);
      indexBuffer_.push_back(indexOffset + i * m + j1);
      indexBuffer_.push_back(indexOffset + i * m + j1 + m);
    }
  }

  // Faces at end
  for (int j = 0; j < ring.size(); j++)
  {
    const auto m = ring.size();
    const auto j0 = j;
    const auto j1 = (j + 1) % m;

    indexBuffer_.push_back(indexOffset + steps * m + j0);
    indexBuffer_.push_back(indexOffset + steps * m + j1);
    indexBuffer_.push_back(indexOffset + steps * m + m);
  }
}
}
}
