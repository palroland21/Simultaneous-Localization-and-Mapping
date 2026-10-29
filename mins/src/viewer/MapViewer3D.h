#pragma once

#include <string>
#include <sys/types.h>

namespace mins {

class MapViewer3D {
public:
    explicit MapViewer3D(const std::string& configPath = "");
    ~MapViewer3D();

    bool start();
    void stop();
    bool isRunning();

private:
    bool createDefaultConfig();

    std::string configPath_;
    std::string generatedConfigPath_;

    pid_t processId_ = -1;
};

}