#include <glar/application.h>

#include <stdexcept>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <opencv2/core.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

namespace glar
{
namespace
{
constexpr int markerId = 23;

void ErrorCallback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}
}

Application::Application()
{
  if (!glfwInit())
    throw std::runtime_error("Failed to initialize glfw");

  glfwSetErrorCallback(ErrorCallback);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  window_ = glfwCreateWindow(width_, height_, "glar", NULL, NULL);
  if (!window_)
    throw std::runtime_error("Failed to create window");

  glfwMakeContextCurrent(window_);

  if (!gladLoadGL())
    throw std::runtime_error("Failed to initialize GL");

  // Create ArUco marker
  cv::Mat markerImage;
  cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
  cv::aruco::drawMarker(dictionary, markerId, 200, markerImage, 1);

  const std::string imageFilename = "marker" + std::to_string(markerId) + ".png";
  // TODO: change marker image filepath
  const std::string imageFilepath = "C:\\workspace\\glar\\bin\\" + imageFilename;
  cv::imwrite(imageFilepath, markerImage);
}

Application::~Application()
{
  glfwTerminate();
}

void Application::Run()
{
  cv::VideoCapture vcap;
  cv::Mat image;

  const std::string videoStreamAddress = "http://192.168.0.12:2225/video";

  while (!glfwWindowShouldClose(window_))
  {
    glfwPollEvents();

    if (!vcap.open(videoStreamAddress))
      std::cout << "Error opening video stream or file" << std::endl;

    if (vcap.isOpened())
    {
      vcap.read(image);
      std::cout << image.size().width << ' ' << image.size().height << std::endl;
      cv::imshow("Stream image", image);

      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1s);
    }

    glClearColor(0.75f, 0.75f, 0.75f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glfwSwapBuffers(window_);
  }

  glfwDestroyWindow(window_);
}
}
