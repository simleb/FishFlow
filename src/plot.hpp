#ifndef FISHFLOW_PLOT_HPP
#define FISHFLOW_PLOT_HPP

#include <opencv2/opencv.hpp>

cv::Mat color(const cv::Mat& m);

void plotVelocity(cv::Mat& frame, const cv::Mat& uv, const cv::Mat& mask);

#endif
