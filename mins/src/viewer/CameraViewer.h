#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <opencv2/opencv.hpp>

#include <array>
#include <cstddef>
#include <mutex>
#include <string>

namespace mins {

class CameraViewer {
public:
    explicit CameraViewer(const rclcpp::Node::SharedPtr& node);
    ~CameraViewer();

    bool render();

private:
    void receiveImage(std::size_t camera, sensor_msgs::msg::Image::ConstSharedPtr msg);

    static constexpr std::size_t CAMERA_COUNT = 3;

    std::array<rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr, CAMERA_COUNT> subscriptions_;
    std::array<cv::Mat, CAMERA_COUNT> images_;
    std::array<bool, CAMERA_COUNT> windowSized_{};

    std::mutex imageMutex_;
};

}
