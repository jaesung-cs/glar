#include <glar/application.h>

#include <stdexcept>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <opencv2/core.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glar/gl/geometry.h>
#include <glar/sensor/video_capture.h>

namespace glar
{
namespace
{
const std::string executableDirpath = "C:\\workspace\\glar\\bin";
const auto iniFilepath = executableDirpath + "\\imgui.ini";

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

  // ImGui initialization
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  ImGui_ImplGlfw_InitForOpenGL(window_, true);
  ImGui_ImplOpenGL3_Init("#version 430");
  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui .ini file location
  io.IniFilename = iniFilepath.c_str();

  createDetectionMarker();
  createCalibrationBoard();
}

Application::~Application()
{
  glfwTerminate();
}

void Application::createDetectionMarker()
{
  // Create ArUco marker
  constexpr int markerId = 23;

  cv::Mat markerImage;
  cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
  cv::aruco::drawMarker(dictionary, markerId, 200, markerImage, 1);

  const std::string imageFilename = "marker" + std::to_string(markerId) + ".png";
  // TODO: change marker image filepath
  const std::string imageFilepath = executableDirpath + "\\" + imageFilename;
  cv::imwrite(imageFilepath, markerImage);
}

void Application::createCalibrationBoard()
{
  cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
  charucoBoard_ = cv::aruco::CharucoBoard::create(7, 5, 0.04f, 0.02f, dictionary);
  cv::Mat boardImage;
  charucoBoard_->draw(cv::Size(600, 500), boardImage, 10, 1);

  cv::imwrite(executableDirpath + "\\calibration_board.jpg", boardImage);
}

void Application::Run()
{
  // Camera image texture
  constexpr uint32_t cameraTextureWidth = 640;
  constexpr uint32_t cameraTextureHeight = 480;

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

  // Rect geometry
  gl::Geometry rectGeometry(
    { 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f },
    { {0, 2, 2, 0} },
    { 0, 1, 2, 2, 1, 3 },
    GL_TRIANGLES
  );

  // Axis geometry
  gl::Geometry axisGeometry(
    {
      0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
      1.f, 0.f, 0.f, 1.f, 0.f, 0.f,
      0.f, 0.f, 0.f, 0.f, 1.f, 0.f,
      0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
      0.f, 0.f, 0.f, 0.f, 0.f, 1.f,
      0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
    },
    {
      {0, 3, 6, 0},
      {1, 3, 6, 3},
    },
    { 0, 1, 2, 3, 4, 5 },
    GL_LINES
  );

  // ArUco dictionary
  cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
  cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);

  // ArUco marker size
  constexpr float markerSize = 0.05; // m

  // Video stream
  const std::string videoStreamAddress = "http://192.168.0.12:2225";
  sensor::VideoCapture vcap(videoStreamAddress);

  // Calibration
  std::chrono::high_resolution_clock::time_point calibrationCaptureTime;
  std::vector<std::vector<std::vector<cv::Point2f>>> calibrationCorners;
  std::vector<std::vector<int>> calibrationIds;
  std::vector<cv::Mat> calibrationImages;

  constexpr int requiredCalibrationImages = 5;
  cv::Mat cameraMatrix;
  cv::Mat distortion;

  // The first calibration result
  /*
  [474.9396379159643, 0, 295.6886455987841;
  0, 479.7568856113904, 268.3747071473783;
  0, 0, 1]
  [-0.08718103293876581, 0.5801102550706361, 0.02068251001707334, -0.02022431125525433, -1.189943292885548]
  */
  cameraMatrix = cv::Mat::zeros(3, 3, CV_32FC1);
  cameraMatrix.at<float>(0, 0) = 474.9396379159643;
  cameraMatrix.at<float>(1, 1) = 479.7568856113904;
  cameraMatrix.at<float>(0, 2) = 295.6886455987841;
  cameraMatrix.at<float>(1, 2) = 268.3747071473783;
  cameraMatrix.at<float>(2, 2) = 1.f;

  distortion = cv::Mat::zeros(1, 5, CV_32FC1);
  distortion.at<float>(0) = -0.08718103293876581;
  distortion.at<float>(1) = 0.5801102550706361;
  distortion.at<float>(2) = 0.02068251001707334;
  distortion.at<float>(3) = -0.02022431125525433;
  distortion.at<float>(4) = -1.189943292885548;

  uint64_t frameCount = 0;
  const auto startTime = std::chrono::high_resolution_clock::now();
  uint64_t seconds = 0;
  while (!glfwWindowShouldClose(window_))
  {
    glfwPollEvents();

    const auto currentTime = std::chrono::high_resolution_clock::now();
    const auto elapsed = std::chrono::duration<double>(currentTime - startTime).count();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Control");

    if (appMode_ == AppMode::CALIBRATION)
    {
      ImGui::Text("Calibrating, please wait...");

      const auto progress = std::to_string(calibrationImages.size()) + " / " + std::to_string(requiredCalibrationImages);
      ImGui::Text(progress.c_str());

      if (ImGui::Button("Cancel"))
      {
        appMode_ = AppMode::DETECTION;
      }
    }
    else
    {
      if (ImGui::Button("Calibrate"))
      {
        appMode_ = AppMode::CALIBRATION;
        calibrationCaptureTime = std::chrono::high_resolution_clock::now();
      }
    }

    ImGui::End();

    auto image = vcap.image();

    if (!image.empty())
    {
      switch (appMode_)
      {
      case AppMode::CALIBRATION:
      {
        // Add image frame every time interval
        constexpr double calibrationInterval = 2.; // every 2s

        const auto timeSinceLastCapture = std::chrono::duration<double>(currentTime - calibrationCaptureTime).count();
        const auto count = calibrationCorners.size();
        if (count * calibrationInterval < timeSinceLastCapture)
        {
          calibrationCaptureTime = currentTime;

          // Detect aruco markers
          std::vector<std::vector<cv::Point2f>> corners, rejected;
          std::vector<int> ids;
          cv::aruco::detectMarkers(image, dictionary, corners, ids, parameters, rejected);

          // Interpolate charuco corners
          if (!ids.empty())
          {
            cv::Mat currentCharucoCorners, currentCharucoIds;
            cv::aruco::interpolateCornersCharuco(corners, ids, image, charucoBoard_,
              currentCharucoCorners, currentCharucoIds);

            calibrationImages.push_back(image.clone());
            calibrationCorners.push_back(corners);
            calibrationIds.push_back(ids);

            // Draw detected markers
            cv::aruco::drawDetectedMarkers(image, corners);
            cv::aruco::drawDetectedCornersCharuco(image, currentCharucoCorners, currentCharucoIds);
          }

          // Move to GL texture
          glBindTexture(GL_TEXTURE_2D, cameraTexture);
          glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cameraTextureWidth, cameraTextureHeight, GL_BGR, GL_UNSIGNED_BYTE, image.ptr());

          // Calibrate if sufficient calibration images are collected
          if (calibrationImages.size() >= requiredCalibrationImages)
          {
            // Prepare data for calibration
            std::vector<std::vector<cv::Point2f>> allCornersConcatenated;
            std::vector<int> allIdsConcatenated;
            std::vector<int> markerCounterPerFrame;

            markerCounterPerFrame.reserve(calibrationCorners.size());
            for (int i = 0; i < calibrationCorners.size(); i++)
            {
              markerCounterPerFrame.push_back(calibrationCorners[i].size());
              for (int j = 0; j < calibrationCorners[i].size(); j++)
              {
                allCornersConcatenated.push_back(calibrationCorners[i][j]);
                allIdsConcatenated.push_back(calibrationIds[i][j]);
              }
            }

            // Calibrate camera using aruco markers
            const auto imgSize = calibrationImages[0].size();
            double arucoRepErr;
            arucoRepErr = cv::aruco::calibrateCameraAruco(allCornersConcatenated, allIdsConcatenated,
              markerCounterPerFrame, charucoBoard_, imgSize,
              cameraMatrix, distortion);

            // Prepare data for charuco calibration
            int nFrames = (int)calibrationCorners.size();
            std::vector<cv::Mat> allCharucoCorners;
            std::vector<cv::Mat> allCharucoIds;
            std::vector<cv::Mat> filteredImages;
            allCharucoCorners.reserve(nFrames);
            allCharucoIds.reserve(nFrames);

            for (int i = 0; i < nFrames; i++)
            {
              // Interpolate using camera parameters
              cv::Mat currentCharucoCorners, currentCharucoIds;
              cv::aruco::interpolateCornersCharuco(calibrationCorners[i], calibrationIds[i], calibrationImages[i], charucoBoard_,
                currentCharucoCorners, currentCharucoIds,
                cameraMatrix, distortion);

              allCharucoCorners.push_back(currentCharucoCorners);
              allCharucoIds.push_back(currentCharucoIds);
            }

            // Calibrate camera using charuco
            const auto repError = cv::aruco::calibrateCameraCharuco(allCharucoCorners, allCharucoIds, charucoBoard_, imgSize,
                cameraMatrix, distortion);

            std::cout << cameraMatrix << std::endl << distortion << std::endl;

            appMode_ = AppMode::DETECTION;

            // Clear calibration resources
            calibrationImages.clear();
            calibrationCorners.clear();
            calibrationIds.clear();
          }
        }
      }
      break;

      case AppMode::DETECTION:
      {
        // ArUco image detection
        std::vector<int> markerIds;
        std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
        cv::aruco::detectMarkers(image, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);

        // Draw to image
        cv::aruco::drawDetectedMarkers(image, markerCorners, markerIds);

        // ArUco pose estimationion
        std::vector<cv::Vec3d> rvecs, tvecs;
        cv::aruco::estimatePoseSingleMarkers(markerCorners, markerSize, cameraMatrix, distortion,
          rvecs, tvecs);

        // Draw axis
        for (int i = 0; i < rvecs.size(); i++)
          cv::aruco::drawAxis(image, cameraMatrix, distortion, rvecs[i], tvecs[i], 0.1);

        // Move to GL texture
        glBindTexture(GL_TEXTURE_2D, cameraTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cameraTextureWidth, cameraTextureHeight, GL_BGR, GL_UNSIGNED_BYTE, image.ptr());
      }
      break;
      }
    }

    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw image
    glUseProgram(cameraShader);

    glBindTexture(GL_TEXTURE_2D, cameraTexture);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(cameraShader, "tex"), 0);

    rectGeometry.draw();

    // Render dear imgui into screen
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window_);

    frameCount++;

    if (seconds < static_cast<uint64_t>(elapsed))
    {
      std::cout << "FPS: " << frameCount / elapsed << std::endl;
      seconds++;
    }

    // Delay between thread
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s / 120.f);
  }

  if (cameraTexture)
    glDeleteTextures(1, &cameraTexture);

  glDeleteProgram(cameraShader);

  // TODO: destroy contexts in destructor?
  glfwDestroyWindow(window_);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void Application::Resized(uint32_t width, uint32_t height)
{
  std::cout << "Resized " << width << ' ' << height << std::endl;
  width_ = width;
  height_ = height;
}
}
