#ifndef GLAR_SCENE_FRACTAL_H_
#define GLAR_SCENE_FRACTAL_H_

#include <vector>

#include <glm/glm.hpp>

namespace glar
{
namespace scene
{
class Fractal
{
private:
  static constexpr float pi = 3.1415926535897932384626433832795f;

public:
  struct CreateInfo
  {
    float height = 10.f;

    int steps = 5;
    float length = 5.f;
    float minLength = 1.f;
    float maxLength = 10.f;
    float scaleCoeff = 0.2f; // scale = e^(-l * coeff)

    int divisionCount = 3;
    float divisionOffsetBegin = 1.f;
    float divisionOffsetEnd = 3.f;

    float lateralAngleBegin = pi / 3.f;
    float lateralAngleEnd = pi / 2.f;
    float curveAngle = pi * 2.f / 3.f;
  };

  struct Curve
  {
  public:
    Curve(const CreateInfo& createInfo)
      : createInfo_(createInfo) {}

    const auto& info() const { return createInfo_; }

    glm::mat4 base = glm::mat4(1.f);
    float startOffset = 0.f;

  private:
    const CreateInfo& createInfo_;
  };

public:
  Fractal() = delete;
  explicit Fractal(const CreateInfo& createInfo);
  ~Fractal();

  const auto& info() const { return createInfo_; }
  const auto& curves() const { return curves_; }

private:
  void CreateCurve(float startOffset, const glm::mat4& transform);

  CreateInfo createInfo_;
  std::vector<Curve> curves_;
};
}
}

#endif // GLAR_SCENE_FRACTAL_H_
