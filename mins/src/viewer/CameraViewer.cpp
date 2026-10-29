#include "CameraViewer.h"

#if __has_include(<cv_bridge/cv_bridge.h>)
#include <cv_bridge/cv_bridge.h>
#else
#include <cv_bridge/cv_bridge.hpp>
#endif

#include <sensor_msgs/image_encodings.hpp>
#include <exception>

namespace mins {

static const std::array<std::string, 3> topics = {
    "/cam0/image_raw",
    "/cam1/image_raw",
    "/mins/cam0/track_img"
};

static const std::array<std::string, 3> windows = {
    "MINS - Camera 0 Original",
    "MINS - Camera 1 Original",
    "MINS - Stereo Feature Tracking"
};

static const std::array<cv::Size, 3> windowSizes = {cv::Size(480, 306), cv::Size(480, 306), cv::Size(1128, 360)};

CameraViewer::CameraViewer(const rclcpp::Node::SharedPtr& node) {
    for (std::size_t i = 0; i < CAMERA_COUNT; i++) {
        auto callback = [this, i](sensor_msgs::msg::Image::ConstSharedPtr msg) {
            receiveImage(i, msg);
        };
        subscriptions_[i] = node->create_subscription<sensor_msgs::msg::Image>(topics[i], rclcpp::SensorDataQoS(), callback);

        cv::namedWindow(windows[i], cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_NORMAL);
        cv::resizeWindow(windows[i], windowSizes[i]);
    }
}

CameraViewer::~CameraViewer() {
    for (const auto& window : windows) {
        cv::destroyWindow(window);
    }
}

void CameraViewer::receiveImage(std::size_t camera, sensor_msgs::msg::Image::ConstSharedPtr msg) {
    try {
        auto converted = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);

        std::lock_guard<std::mutex> lock(imageMutex_);
        images_[camera] = converted->image.clone();
    } catch (const std::exception& e) {
        RCLCPP_WARN(rclcpp::get_logger("CameraViewer"), "Image conversion failed: %s", e.what());
    }
}

bool CameraViewer::render() {
    std::array<cv::Mat, CAMERA_COUNT> frames;

    {
        std::lock_guard<std::mutex> lock(imageMutex_);

        for (std::size_t i = 0; i < CAMERA_COUNT; i++) {
            frames[i] = images_[i].clone();
        }
    }

    for (std::size_t i = 0; i < CAMERA_COUNT; i++) {
        bool hasImage = !frames[i].empty();
        if (frames[i].empty()) {
            frames[i] = cv::Mat::zeros(windowSizes[i], CV_8UC3);
            cv::putText(frames[i], "Waiting for images...", cv::Point(30, windowSizes[i].height / 2), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        }

        cv::imshow(windows[i], frames[i]);
        if (hasImage && !windowSized_[i]) {
            cv::resizeWindow(windows[i], windowSizes[i]);
            windowSized_[i] = true;
        }
    }

    int key = cv::waitKey(1);

    return key != 27 && key != 'q' && key != 'Q';
}

}
