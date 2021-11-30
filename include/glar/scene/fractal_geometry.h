#ifndef GLAR_SCENE_FRACTAL_GEOMETRY_H_
#define GLAR_SCENE_FRACTAL_GEOMETRY_H_

#include <glad/glad.h>

#include <glar/scene/fractal.h>

namespace glar
{
namespace scene
{
class FractalGeometry
{
public:
  FractalGeometry() = delete;
  explicit FractalGeometry(const Fractal& fractal);
  ~FractalGeometry();

  void UpdateAnimation(float animationTime);
  void Draw();

private:
  void AppendToBuffer(const Fractal::Curve& curve, float animationTime);

  const Fractal& fractal_;

  GLuint vao_;
  GLuint buffers_[2];

  uint32_t indexCount_ = 0;

  // Intermediates for updating buffers
  std::vector<float> vertexBuffer_;
  std::vector<unsigned int> indexBuffer_;
};
}
}

#endif // GLAR_SCENE_FRACTAL_GEOMETRY_H_
