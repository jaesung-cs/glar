#ifndef GLAR_APPLICATION_H_
#define GLAR_APPLICATION_H_

#include <cstdint>
#include <memory>
#include <string>

#include <opencv2/aruco/charuco.hpp>

struct GLFWwindow;

namespace glar
{
class Application
{
private:
  enum class AppMode
  {
    CALIBRATION,
    DETECTION,
  };

public:
  Application();
  ~Application();

  void Run();

private:
  void CreateDetectionMarker();
  void CreateCalibrationBoard();

  uint32_t width_ = 640;
  uint32_t height_ = 480;
  GLFWwindow* window_ = nullptr;

  AppMode appMode_ = AppMode::DETECTION;

  cv::Ptr<cv::aruco::CharucoBoard> charucoBoard_;
};
}

#endif // GLAR_APPLICATION_H_
