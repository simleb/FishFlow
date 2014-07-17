#ifndef FISHFLOW_PLOT_HPP
#define FISHFLOW_PLOT_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/ocl/ocl.hpp>

cv::Mat color(const cv::ocl::oclMat& dm);

void plotVelocity(cv::Mat& frame, const cv::Mat& u, const cv::Mat& v, const cv::Mat& mask);

#endif
