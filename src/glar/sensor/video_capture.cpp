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

      std::chrono::high_resolution_clock::time_point streamOpenTime;

      bool opened = false;
      uint64_t frameIndex = 0;
      while (!terminate_)
      {
        using namespace std::chrono_literals;

        if (!opened)
        {
          std::cout << "Opening stream" << std::endl;
          if (vcap.open(videoStreamAddress_))
          {
            const double targetFps = vcap.get(cv::CAP_PROP_FPS);

            // Limit vcap buffer size
            vcap.set(cv::CAP_PROP_BUFFERSIZE, 2);

            std::cout << "Video stream opened:" << std::endl
              << "  FPS: " << targetFps << std::endl;

            {
              std::unique_lock<std::mutex> guard(fpsMutex_);
              targetFps_ = targetFps;
            }

            streamOpenTime = std::chrono::high_resolution_clock::now();
            frameIndex = 0;

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
          {
            UpdateImage(image);
            frameIndex++;
          }

          else
          {
            vcap = {};
            opened = false;
            frameIndex = 0;
          }
        }

        // Update fps
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const auto elapsed = std::chrono::duration<double>(currentTime - streamOpenTime).count();
        const auto fps = frameIndex / elapsed;
        {
          std::unique_lock<std::mutex> guard(fpsMutex_);
          fps_ = fps;
        }
      }
    });
}

VideoCapture::~VideoCapture()
{
  terminate_ = true;
  worker_.join();
}

double VideoCapture::TargetFps() const
{
  std::unique_lock<std::mutex> guard(fpsMutex_);
  return targetFps_;
}

double VideoCapture::Fps() const
{
  std::unique_lock<std::mutex> guard(fpsMutex_);
  return fps_;
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
