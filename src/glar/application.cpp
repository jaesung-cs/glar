#include <glar/application.h>

#include <stdexcept>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <fstream>
#include <sstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <opencv2/core.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
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

void ResizeCallback(GLFWwindow* window, int width, int height)
{
  const auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
  app->Resized(width, height);
}

GLuint LoadShaderModule(const std::string& shaderFilepath, GLenum shaderStage)
{
  std::string code;
  {
    std::ifstream in(shaderFilepath);
    std::stringstream ss;
    ss << in.rdbuf();
    code = ss.str();
  }

  GLuint shader = glCreateShader(shaderStage);
  const char* codeString = code.c_str();
  glShaderSource(shader, 1, &codeString, NULL);
  glCompileShader(shader);

  GLint success;
  GLchar infoLog[1024];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(shader, 1024, NULL, infoLog);
    std::cout << "Failed to compile shader (" << shaderStage << "), error:" << std::endl
      << infoLog << std::endl;
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}
}

Application::Application()
{
  if (!glfwInit())
    throw std::runtime_error("Failed to initialize glfw");

  glfwSetErrorCallback(ErrorCallback);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  window_ = glfwCreateWindow(width_, height_, "glar", NULL, NULL);
  if (!window_)
    throw std::runtime_error("Failed to create window");

  // Callbacks
  glfwSetWindowUserPointer(window_, this);
  glfwSetWindowSizeCallback(window_, ResizeCallback);

  glfwMakeContextCurrent(window_);

  if (!gladLoadGL())
    throw std::runtime_error("Failed to initialize GL");

  // OpenGL initialization
  glEnable(GL_DEPTH_TEST);
  glClearColor(0.75f, 0.75f, 0.75f, 1.f);

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

  // Camera image texture
  constexpr uint32_t cameraTextureWidth = 1920;
  constexpr uint32_t cameraTextureHeight = 1080;

  GLuint cameraTexture;
  glGenTextures(1, &cameraTexture);
  glBindTexture(GL_TEXTURE_2D, cameraTexture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, cameraTextureWidth, cameraTextureHeight);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  const std::string shaderDirpath = "C:\\workspace\\glar\\src\\glar\\shader";
  const auto vertexShader = LoadShaderModule(shaderDirpath + "\\camera.vert", GL_VERTEX_SHADER);
  const auto fragmentShader = LoadShaderModule(shaderDirpath + "\\camera.frag", GL_FRAGMENT_SHADER);

  const auto cameraShader = glCreateProgram();
  glAttachShader(cameraShader, vertexShader);
  glAttachShader(cameraShader, fragmentShader);
  glLinkProgram(cameraShader);

  GLint success;
  GLchar infoLog[1024];
  glGetProgramiv(cameraShader, GL_LINK_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(cameraShader, 1024, NULL, infoLog);
    std::cout << "Failed to link shader program, error:" << std::endl
      << infoLog << std::endl;
    glDeleteShader(cameraShader);
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // Rect buffer
  const std::vector<float> rectVertex = {
    0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f,
  };
  const std::vector<uint32_t> rectIndex = {
    0, 1, 2, 2, 1, 3,
  };

  GLuint rectVao;
  glGenVertexArrays(1, &rectVao);
  glBindVertexArray(rectVao);

  GLuint rectBuffers[2]; // 0: vertex, 1: index
  glGenBuffers(2, rectBuffers);
  glBindBuffer(GL_ARRAY_BUFFER, rectBuffers[0]);
  glBufferData(GL_ARRAY_BUFFER, rectVertex.size() * sizeof(rectVertex[0]), rectVertex.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rectBuffers[1]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, rectIndex.size() * sizeof(rectIndex[0]), rectIndex.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);

  uint64_t frameCount = 0;
  const auto startTime = std::chrono::high_resolution_clock::now();
  uint64_t seconds = 0;

  while (!glfwWindowShouldClose(window_))
  {
    glfwPollEvents();

    if (!vcap.isOpened() && !vcap.open(videoStreamAddress))
      std::cout << "Error opening video stream or file" << std::endl;

    bool imageReady = false;
    if (vcap.isOpened())
    {
      vcap.read(image);

      // Move to GL texture
      glBindTexture(GL_TEXTURE_2D, cameraTexture);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cameraTextureWidth, cameraTextureHeight, GL_BGR, GL_UNSIGNED_BYTE, image.ptr());
      imageReady = true;
    }
    else
      imageReady = false;

    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (imageReady)
    {
      glUseProgram(cameraShader);

      glBindTexture(GL_TEXTURE_2D, cameraTexture);
      glActiveTexture(GL_TEXTURE0);
      glUniform1i(glGetUniformLocation(cameraShader, "tex"), 0);

      glBindVertexArray(rectVao);
      glDrawElements(GL_TRIANGLES, rectIndex.size(), GL_UNSIGNED_INT, 0);
    }

    glfwSwapBuffers(window_);

    frameCount++;
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const auto elapsed = std::chrono::duration<double>(currentTime - startTime).count();

    if (seconds < static_cast<uint64_t>(elapsed))
    {
      std::cout << "FPS: " << frameCount / elapsed << std::endl;
      seconds++;
    }
  }

  vcap.release();

  if (cameraTexture)
    glDeleteTextures(1, &cameraTexture);

  glDeleteBuffers(2, rectBuffers);
  glDeleteVertexArrays(1, &rectVao);

  glDeleteProgram(cameraShader);

  glfwDestroyWindow(window_);
}

void Application::Resized(uint32_t width, uint32_t height)
{
  std::cout << "Resized " << width << ' ' << height << std::endl;
  width_ = width;
  height_ = height;
}
}
