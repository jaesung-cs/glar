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

  cv::Mat image();

  // Accessed by worker
  void updateImage(cv::Mat image);

private:
  std::string videoStreamAddress_;

  std::thread worker_;
  std::atomic_bool terminate_ = false;

  // Double buffering
  cv::Mat image_;
  std::mutex mutex_;
};
}
}

#endif // GLAR_SENSOR_VIDEO_CAPTURE_H_
