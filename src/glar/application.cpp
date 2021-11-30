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

#include <glm/glm.hpp>

#include <opencv2/core.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glar/gl/geometry.h>
#include <glar/gl/shader.h>
#include <glar/gl/texture.h>
#include <glar/sensor/video_capture.h>
#include <glar/scene/fractal.h>
#include <glar/scene/fractal_geometry.h>

namespace glar
{
namespace
{
const std::string executableDirpath = "C:\\workspace\\glar\\bin";
const auto iniFilepath = executableDirpath + "\\imgui.ini";
const auto calibFilepath = executableDirpath + "\\calib.txt";

void ErrorCallback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}

void SaveCalibration(cv::Mat cameraMatrix, cv::Mat distortion)
{
  std::ofstream out(calibFilepath);

  for (int r = 0; r < 3; r++)
  {
    for (int c = 0; c < 3; c++)
      out << cameraMatrix.at<double>(r, c) << " ";
    out << std::endl;
  }

  for (int i = 0; i < 5; i++)
    out << distortion.at<double>(i) << ' ';
  out << std::endl;
}

void LoadCalibration(cv::Mat& cameraMatrix, cv::Mat& distortion)
{
  try
  {
    std::ifstream in;
    in.exceptions(std::ios::failbit);
    in.open(calibFilepath);

    cameraMatrix = cv::Mat::zeros(3, 3, CV_64FC1);
    for (int r = 0; r < 3; r++)
    {
      for (int c = 0; c < 3; c++)
        in >> cameraMatrix.at<double>(r, c);
    }

    distortion = cv::Mat::zeros(1, 5, CV_64FC1);
    for (int i = 0; i < 5; i++)
      in >> distortion.at<double>(i);

    std::cout << "Loaded calib.txt:" << std::endl
      << cameraMatrix << std::endl
      << distortion << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Failed to load calibration from file (" << calibFilepath << "): " << e.what() << std::endl
      << "initial values for 640x480" << std::endl;

    cameraMatrix = cv::Mat::zeros(3, 3, CV_64FC1);
    cameraMatrix.at<double>(0, 0) = 473.7766600392583;
    cameraMatrix.at<double>(1, 1) = 474.9662247385612;
    cameraMatrix.at<double>(0, 2) = 314.0060389975553;
    cameraMatrix.at<double>(1, 2) = 242.821899302888;
    cameraMatrix.at<double>(2, 2) = 1.f;

    distortion = cv::Mat::zeros(1, 5, CV_64FC1);
    distortion.at<double>(0) = 0.01452802692558459;
    distortion.at<double>(1) = -0.07787241532742364;
    distortion.at<double>(2) = 0.003182402256963925;
    distortion.at<double>(3) = -0.003855926597756357;
    distortion.at<double>(4) = 0.2134733796822924;

    // Save the initial calibration file
    SaveCalibration(cameraMatrix, distortion);
  }
}
}

Application::Application()
{
  if (!glfwInit())
    throw std::runtime_error("Failed to initialize glfw");

  glfwSetErrorCallback(ErrorCallback);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  window_ = glfwCreateWindow(width_, height_, "glar", NULL, NULL);
  if (!window_)
    throw std::runtime_error("Failed to create window");

  // Callbacks
  glfwSetWindowUserPointer(window_, this);

  glfwMakeContextCurrent(window_);

  if (!gladLoadGL())
    throw std::runtime_error("Failed to initialize GL");

  // OpenGL initialization
  glEnable(GL_DEPTH_TEST);
  glClearDepth(1.f);
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

  CreateDetectionMarker();
  CreateCalibrationBoard();
}

Application::~Application()
{
  glfwTerminate();
}

void Application::CreateDetectionMarker()
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

void Application::CreateCalibrationBoard()
{
  cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
  charucoBoard_ = cv::aruco::CharucoBoard::create(7, 5, 0.04f, 0.02f, dictionary);
  cv::Mat boardImage;
  charucoBoard_->draw(cv::Size(600, 500), boardImage, 10, 1);

  //cv::imwrite(executableDirpath + "\\calibration_board.jpg", boardImage);
}

void Application::Run()
{
  const std::string shaderDirpath = "C:\\workspace\\glar\\src\\glar\\shader";
  gl::Shader cameraShader(shaderDirpath, "camera");
  gl::Shader colorShader(shaderDirpath, "color");
  gl::Shader phongShader(shaderDirpath, "phong");

  // AR matrices
  constexpr float near = 0.01f;
  constexpr float far = 10.f;
  auto model = glm::mat4(1.f);

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

  // Fractal
  scene::Fractal::CreateInfo fractalCreateInfo;
  scene::Fractal fractal(fractalCreateInfo);
  scene::FractalGeometry fractalGeometry(fractal);

  // ArUco dictionary
  cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
  cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);

  // ArUco marker size
  constexpr float markerSize = 0.042; // 4.2cm

  // Video stream
  const std::string videoStreamAddress = "http://192.168.0.12:62225";
  sensor::VideoCapture vcap(videoStreamAddress);

  // Calibration
  std::chrono::high_resolution_clock::time_point calibrationCaptureTime;
  std::vector<std::vector<std::vector<cv::Point2f>>> calibrationCorners;
  std::vector<std::vector<int>> calibrationIds;
  std::vector<cv::Mat> calibrationImages;

  constexpr int requiredCalibrationImages = 5;
  cv::Mat cameraMatrix;
  cv::Mat distortion;

  // The initial calibration matrix
  LoadCalibration(cameraMatrix, distortion);

  // Camera image texture
  gl::Texture cameraTexture;

  uint64_t frameCount = 0;
  const auto startTime = std::chrono::high_resolution_clock::now();
  auto animationStartTime = std::chrono::high_resolution_clock::now();
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

    {
      std::ostringstream ss;
      ss
        << "Graphics FPS: " << frameCount / elapsed << std::endl
        << "Stream   FPS: " << vcap.TargetFps() << std::endl
        << "Actual   FPS: " << vcap.Fps();
      ImGui::Text(ss.str().c_str());
    }

    if (ImGui::CollapsingHeader("Calibration parameters", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::Text("Calibration matrix:");
      for (int i = 0; i < cameraMatrix.rows; i++)
      {
        for (int j = 0; j < cameraMatrix.cols; j++)
        {
          std::ostringstream ss;
          ss << cameraMatrix.at<double>(i, j);
          ImGui::Text(ss.str().c_str());
          if (j + 1 < cameraMatrix.cols)
            ImGui::SameLine();
        }
      }

      ImGui::Separator();

      ImGui::Text("Distortion parameters:");
      for (int i = 0; i < distortion.cols; i++)
      {
        std::ostringstream ss;
        ss << distortion.at<double>(0, i);
        ImGui::Text(ss.str().c_str());
        if (i + 1 < distortion.cols)
          ImGui::SameLine();
      }

      ImGui::Separator();

      // Calibration stage
      if (appMode_ == AppMode::CALIBRATION)
      {
        ImGui::Text("Calibrating, please wait...");

        const auto progress = std::to_string(calibrationImages.size()) + " / " + std::to_string(requiredCalibrationImages);
        ImGui::Text(progress.c_str());

        if (ImGui::Button("Cancel"))
          appMode_ = AppMode::DETECTION;
      }
      else
      {
        if (ImGui::Button("Calibrate"))
        {
          appMode_ = AppMode::CALIBRATION;
          calibrationCaptureTime = std::chrono::high_resolution_clock::now();
        }
      }

      ImGui::Separator();
    }

    if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
    {
      if (appMode_ != AppMode::CALIBRATION)
      {
        static int modeIndex = 0;
        ImGui::RadioButton("Detection", &modeIndex, 0);
        if (ImGui::RadioButton("Augment", &modeIndex, 1))
        {
          if (modeIndex == 1)
            animationStartTime = std::chrono::high_resolution_clock::now();
        }

        switch (modeIndex)
        {
        case 0: appMode_ = AppMode::DETECTION; break;
        case 1: appMode_ = AppMode::AUGMENT; break;
        }
      }
    }
    
    auto image = vcap.Image();

    if (!image.empty())
    {
      // Resize if window size is different
      if (width_ != image.cols || height_ != image.rows)
      {
        width_ = image.cols;
        height_ = image.rows;
        glfwSetWindowSize(window_, width_, height_);
      }

      cameraTexture.UpdateStorage(image.cols, image.rows);

      switch (appMode_)
      {
      case AppMode::CALIBRATION:
      {
        // Add image frame every time interval
        constexpr double calibrationInterval = 2.; // every 2s

        const auto timeSinceLastCapture = std::chrono::duration<double>(currentTime - calibrationCaptureTime).count();
        const auto count = calibrationCorners.size();
        if (calibrationInterval < timeSinceLastCapture)
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
          cameraTexture.Update(image.ptr(), GL_BGR);

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

            std::cout << "Calibration matrix:" << std::endl
              << cameraMatrix << std::endl
              << "Distortion parameters:" << std::endl
              << distortion << std::endl;

            // Save to calib file
            SaveCalibration(cameraMatrix, distortion);

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
          cv::aruco::drawAxis(image, cameraMatrix, distortion, rvecs[i], tvecs[i], markerSize / 2.f);

        // Move to GL texture
        cameraTexture.Update(image.ptr(), GL_BGR);
      }
      break;

      case AppMode::AUGMENT:
      {
        // ArUco image detection
        std::vector<int> markerIds;
        std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
        cv::aruco::detectMarkers(image, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);

        // ArUco pose estimationion
        std::vector<cv::Vec3d> rvecs, tvecs;
        cv::aruco::estimatePoseSingleMarkers(markerCorners, markerSize, cameraMatrix, distortion,
          rvecs, tvecs);

        // Store scene model matrix
        if (tvecs.size() >= 1)
        {
          cv::Mat rot;
          cv::Rodrigues(rvecs[0], rot);

          for (int r = 0; r < 3; r++)
          {
            for (int c = 0; c < 3; c++)
              model[c][r] = rot.at<double>(r, c) * (markerSize / 2.f);
            model[3][r] = tvecs[0](r);
          }
          model[3][3] = 1.f;
        }

        // Move to GL texture
        cameraTexture.Update(image.ptr(), GL_BGR);
      }
      break;
      }
    }

    ImGui::End();

    // Draw camera image
    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (cameraTexture.Valid())
    {
      cameraShader.Use();

      cameraTexture.Bind(0);
      cameraShader.Uniform1i("tex", 0);

      // Don't write depth mask
      // TODO: plane depth in shader instead of not writing to depth buffer
      glDepthMask(GL_FALSE);
      rectGeometry.Draw();
      glDepthMask(GL_TRUE);

      if (appMode_ == AppMode::AUGMENT)
      {
        glm::mat3 intrinsic;
        for (int r = 0; r < 3; r++)
        {
          for (int c = 0; c < 3; c++)
            intrinsic[c][r] = cameraMatrix.at<double>(r, c);
        }
        auto screen = glm::vec4(width_, height_, near, far);

        // Draw axis
        colorShader.Use();
        colorShader.UniformMatrix4f("model", model);
        colorShader.UniformMatrix3f("intrinsic", intrinsic);
        colorShader.Uniform4f("screen", screen);

        axisGeometry.Draw();

        // Update fractal animation
        const auto animationTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - animationStartTime).count();
        fractalGeometry.UpdateAnimation(animationTime);

        // Draw fractal
        phongShader.Use();
        phongShader.UniformMatrix4f("model", model);
        phongShader.UniformMatrix3f("intrinsic", intrinsic);
        phongShader.Uniform4f("screen", screen);

        fractalGeometry.Draw();
      }
    }

    // Render dear imgui into screen
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window_);

    frameCount++;

    // Delay between thread
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s / 120.f);
  }

  // TODO: destroy contexts in destructor?
  glfwDestroyWindow(window_);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}
}
