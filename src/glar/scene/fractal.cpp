#include <glar/scene/fractal.h>

#include <random>
#include <iostream>

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

Fractal::Fractal(const CreateInfo& createInfo)
  : createInfo_(createInfo)
{
  CreateCurve(0.f, glm::mat4(1.f));

  std::cout << curves_.size() << " curves created" << std::endl;
}

Fractal::~Fractal() = default;

void Fractal::CreateCurve(float startOffset, const glm::mat4& transform)
{
  Curve curve(createInfo_);
  curve.startOffset = startOffset;
  curve.base = transform;
  curves_.push_back(curve);

  for (int i = 0; i < createInfo_.divisionCount; i++)
  {
    const auto divisionOffset = random(createInfo_.divisionOffsetBegin, createInfo_.divisionOffsetEnd);
    const auto offset = startOffset + divisionOffset;
    if (offset + createInfo_.minLength < createInfo_.maxLength)
    {
      auto lateralAngle = random(createInfo_.lateralAngleBegin, createInfo_.lateralAngleEnd);
      lateralAngle *= (rand01() < 0.5f) ? -1.f : 1.f;

      const auto divisionAngle = createInfo_.curveAngle * (divisionOffset / createInfo_.length);

      const auto divisionStep = static_cast<int>(divisionOffset);
      glm::mat4 divisionTransform = glm::mat4(1.f);

      // Rotation
      constexpr auto divisionAngleOffset = -pi / 2.f;

      divisionTransform = 
        glm::toMat4(glm::angleAxis(divisionAngle + divisionAngleOffset, glm::vec3(1.f, 0.f, 0.f)))
        * glm::toMat4(glm::angleAxis(lateralAngle, glm::vec3(0.f, 0.f, 1.f)))
        * divisionTransform;

      // Float part
      const auto t = divisionOffset - divisionStep;
      const auto translation = glm::vec3(0.f, 0.f, t * (createInfo_.height / createInfo_.steps) * std::exp(-createInfo_.scaleCoeff * t));
      divisionTransform = glm::translate(translation) * divisionTransform;

      // Integer part
      for (int j = 0; j < divisionStep; j++)
      {
        // Scale
        divisionTransform = glm::scale(glm::vec3(std::exp(-createInfo_.scaleCoeff))) * divisionTransform;

        // Rotation
        divisionTransform =
          glm::toMat4(glm::angleAxis(createInfo_.curveAngle / createInfo_.steps, glm::vec3(1.f, 0.f, 0.f)))
          * divisionTransform;

        // Translation
        divisionTransform = glm::translate(glm::vec3(0.f, 0.f, createInfo_.height / createInfo_.steps)) * divisionTransform;
      }

      CreateCurve(offset, transform * divisionTransform);
    }
  }
}
}
}
