#ifndef FISHFLOW_PLOT_HPP
#define FISHFLOW_PLOT_HPP

#include <opencv2/opencv.hpp>

cv::Mat color(const cv::Mat& m);

class Plot {
public:
	Plot(int grid_width, int grid_height) : _gw(grid_width), _gh(grid_height) {}

	void plotVelocity(cv::Mat& frame, const cv::Mat& uv, const cv::Mat& mask);

private:
	const int _gw, _gh;
};

#endif
