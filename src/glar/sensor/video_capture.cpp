#include <glar/sensor/video_capture.h>

#include <chrono>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

namespace glar
{
namespace sensor
{
VideoCapture::VideoCapture(const std::string& address)
{
  videoStreamAddress_ = address + "/video";

  worker_ = std::thread([&]
    {
      cv::VideoCapture vcap;
      cv::Mat image;

      double fps = 0.;
      std::chrono::high_resolution_clock::time_point streamOpenTime;

      uint64_t seconds = 0;
      bool opened = false;
      while (!terminate_)
      {
        using namespace std::chrono_literals;

        if (!opened)
        {
          std::cout << "Opening stream" << std::endl;
          if (vcap.open(videoStreamAddress_))
          {
            fps = vcap.get(cv::CAP_PROP_FPS);

            // Limit vcap buffer size
            vcap.set(cv::CAP_PROP_BUFFERSIZE, 2);

            std::cout << "Video stream opened:" << std::endl
              << "  FPS: " << fps << std::endl;

            streamOpenTime = std::chrono::high_resolution_clock::now();

            opened = true;
          }
          else
          {
            std::cout << "Failed to open video stream. Waiting for 1s..." << std::endl;
            vcap = {};
            std::this_thread::sleep_for(1s);
          }
        }

        if (opened)
        {
          const auto currentTime = std::chrono::high_resolution_clock::now();
          const auto elapsed = std::chrono::duration<double>(currentTime - streamOpenTime).count();

          if (vcap.read(image))
            UpdateImage(image);

          else
          {
            vcap = {};
            opened = false;
          }
        }
      }
    });
}

VideoCapture::~VideoCapture()
{
  terminate_ = true;
  worker_.join();
}

cv::Mat VideoCapture::Image()
{
  std::unique_lock<std::mutex> guard(mutex_);
  auto result = image_;
  image_ = cv::Mat();
  return result;
}

void VideoCapture::UpdateImage(cv::Mat image)
{
  std::unique_lock<std::mutex> guard(mutex_);
  image_ = image;
}
}
}
