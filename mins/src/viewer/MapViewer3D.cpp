#include "MapViewer3D.h"

#include <spawn.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

extern char** environ;

namespace mins {

MapViewer3D::MapViewer3D(const std::string& configPath) : configPath_(configPath) {
}

MapViewer3D::~MapViewer3D() {
    stop();
}

bool MapViewer3D::createDefaultConfig() {
    generatedConfigPath_ = "/tmp/mins_viewer3d_" + std::to_string(getpid()) + ".rviz";

    std::ofstream file(generatedConfigPath_);

    if (!file.is_open()) {
        std::cerr << "Failed to create RViz configuration." << std::endl;
        return false;
    }

    file << R"(
Panels:
  - Class: rviz_common/Displays
    Name: Displays
Visualization Manager:
  Class: ""
  Displays:
    - Class: rviz_default_plugins/Grid
      Name: Grid
      Enabled: true
      Cell Size: 1
      Plane: XY
      Plane Cell Count: 20

    - Class: rviz_default_plugins/Path
      Name: MINS Trajectory
      Enabled: true
      Color: 0; 255; 0
      Line Style: Lines
      Line Width: 0.03
      Topic:
        Depth: 5
        Reliability Policy: Best Effort
        Value: /mins/imu/path

    - Class: rviz_default_plugins/PointCloud2
      Name: MINS Visual Points
      Enabled: true
      Style: Points
      Size (Pixels): 3
      Color Transformer: FlatColor
      Color: 255; 170; 0
      Topic:
        Depth: 5
        Reliability Policy: Best Effort
        Value: /mins/cam/slam

  Global Options:
    Fixed Frame: global
    Background Color: 48; 48; 48
    Frame Rate: 30

  Name: root

  Views:
    Current:
      Class: rviz_default_plugins/Orbit
      Distance: 30
      Focal Point:
        X: 0
        Y: 0
        Z: 0
    Saved: []
)";

    file.close();

    return file.good();
}

bool MapViewer3D::start() {
    if (isRunning()) {
        return true;
    }

    if (configPath_.empty() && !createDefaultConfig()) {
        return false;
    }

    const std::string& path = configPath_.empty() ? generatedConfigPath_ : configPath_;

    setenv("LIBGL_ALWAYS_SOFTWARE", "1", 0);
    setenv("QT_QPA_PLATFORM", "xcb", 0);

    std::vector<char*> arguments = {
        const_cast<char*>("rviz2"),
        const_cast<char*>("-d"),
        const_cast<char*>(path.c_str()),
        nullptr
    };

    int result = posix_spawnp(&processId_, "rviz2", nullptr, nullptr, arguments.data(), environ);

    if (result != 0) {
        processId_ = -1;
        std::cerr << "Failed to start RViz2: " << std::strerror(result) << std::endl;
        return false;
    }

    std::cout << "MINS 3D Viewer started." << std::endl;

    return true;
}

bool MapViewer3D::isRunning() {
    if (processId_ <= 0) {
        return false;
    }

    int status = 0;
    pid_t result = waitpid(processId_, &status, WNOHANG);

    if (result == 0) {
        return true;
    }

    processId_ = -1;
    return false;
}

void MapViewer3D::stop() {
    if (isRunning()) {
        kill(processId_, SIGTERM);

        int status = 0;
        bool stopped = false;

        for (int i = 0; i < 20; i++) {
            pid_t result = waitpid(processId_, &status, WNOHANG);

            if (result == processId_ || (result < 0 && errno == ECHILD)) {
                stopped = true;
                break;
            }

            usleep(50000);
        }

        if (!stopped) {
            kill(processId_, SIGKILL);
            waitpid(processId_, &status, 0);
        }

        processId_ = -1;
    }

    if (!generatedConfigPath_.empty()) {
        unlink(generatedConfigPath_.c_str());
        generatedConfigPath_.clear();
    }
}

}