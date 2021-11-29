#ifndef GLAR_SENSOR_VIDEO_CAPTURE_H_
#define GLAR_SENSOR_VIDEO_CAPTURE_H_

#include <string>
#include <thread>
#include <mutex>
#include <atomic>

#include <opencv2/opencv.hpp>

namespace glar
{
namespace sensor
{
/**
* Multithreaded video sensor wrapper
*/
class VideoCapture
{
public:
  VideoCapture() = delete;
  explicit VideoCapture(const std::string& address);
  ~VideoCapture();

  double TargetFps() const;
  double Fps() const;

  cv::Mat Image();

  // Accessed by worker
  void UpdateImage(cv::Mat image);

private:
  std::string videoStreamAddress_;

  double fps_ = 0.;
  double targetFps_ = 0.;
  mutable std::mutex fpsMutex_;

  std::thread worker_;
  std::atomic_bool terminate_ = false;

  // TODO: Double buffering
  cv::Mat image_;
  std::mutex mutex_;
};
}
}

#endif // GLAR_SENSOR_VIDEO_CAPTURE_H_
