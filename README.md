# glar
ArUco with OpenGL

## Features
- Camera calibration with ChArUco board and OpenCV
- ArUco marker detection with OpenCV
- ArUco marker pose estimation with OpenCV
- Augmented 3D fractal tree animation rendering using OpenGL

## Requirements
- Windows
- Visual Studio 2022
    - But should work in vs2017/2019 after changing platform target from v143 to v141/v142
- C++17
    - Because of `std::filesystem`
- OpenGL 4.3
    - 4.0+ will be okay
- IP Webcam on your smartphone

## Dependencies
- glad
- glm
- glfw3
- opencv4
- imgui

## Build and Running Instruction
1. (Windows) Install vcpkg globally, following the install guide (https://vcpkg.io/en/getting-started.html)
    - Install vcpkg
    ```
    cd C:\src  # any directory you want to install vcpkg
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    .\bootstrap-vcpkg.bat
    ```
    - Install the following libraries:
    ```
    vcpkg.exe install glad[gl-api-43]:x64-windows
    vcpkg.exe install glm:x64-windows
    vcpkg.exe install glfw3:x64-windows
    vcpkg.exe install opencv4[core,contrib,ffmpeg]:x64-windows
    vcpkg.exe install imgui[glfw-binding,opengl3-binding]:x64-windows
    ```
    - Then let Visual Studio use the install
    ```
    vcpkg integrate install
    ```
2. Change `executableDirpath` and `shaderDirpath` in `src/glar/application.cpp`
3. Build with VS solution file `vs/glar.sln`. The executables can be found in `bin/`.
4. On the first run, `calibration_board.jpg` and `marker23.png` will be generated at `executableDirpath`.
5. You can change `markerSize` in `src/glar/application.cpp` to match with the physical length of printed marker.
6. The `Calibrate` button will capture 5 snapshots **every 2 seconds**.
7. After calibration, you can detection and draw 3d scene on marker.

## TODOs
- MacOS build with CMake
- Hard-coded values (shader and executable directories, markerSize, ...)
- Remove pose estimation noise
    - The estimated pose is unstable frame to frame. Use Kalman Filter for removing noise?
