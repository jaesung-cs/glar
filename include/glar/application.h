#ifndef GLAR_APPLICATION_H_
#define GLAR_APPLICATION_H_

#include <cstdint>

struct GLFWwindow;

namespace glar
{
class Application
{
public:
  Application();
  ~Application();

  void Run();

  void Resized(uint32_t width, uint32_t height);

private:
  uint32_t width_ = 1600;
  uint32_t height_ = 900;
  GLFWwindow* window_ = nullptr;
};
}

#endif // GLAR_APPLICATION_H_
