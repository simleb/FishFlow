#include "plot.hpp"

#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/round.hpp>

namespace {

	// Parameters
	const int arrow_thickness = 2;
	const int arrow_head_size = 4;
	const cv::Scalar arrow_color = cv::Scalar(0, 0, 255);
	const float arrow_scale = 10;
	const int grid_nx = 128;
	const int grid_ny = 64;

	const double pi = boost::math::constants::pi<double>();

	void drawArrow(cv::Mat& frame, const cv::Point& p1, const cv::Point& p2) {
		cv::line(frame, p1, p2, arrow_color, arrow_thickness, CV_AA);
		if (p1 != p2) {
			const double theta = atan2(p2.y - p1.y, p2.x - p1.x);
			cv::Point p;
			p.x = p2.x - boost::math::round(arrow_head_size * cos(theta + pi/4));
			p.y = p2.y - boost::math::round(arrow_head_size * sin(theta + pi/4));
			cv::line(frame, p, p2, arrow_color, arrow_thickness, CV_AA);
			p.x = p2.x - boost::math::round(arrow_head_size * cos(theta - pi/4));
			p.y = p2.y - boost::math::round(arrow_head_size * sin(theta - pi/4));
			cv::line(frame, p, p2, arrow_color, arrow_thickness, CV_AA);
		}
	}

	cv::Vec3b colorize(const uchar c) {
		const uchar m = 255;
		switch (c * 8 / m) {
		case 0:
			return cv::Vec3b(4 * c + m / 2, 0, 0);
		case 1:
		case 2:
			return cv::Vec3b(m, 4 * c - m / 2, 0);
		case 3:
		case 4:
			return cv::Vec3b(5 * m / 2 - 4 * c, m, 4 * c - 3 * m / 2);
		case 5:
		case 6:
			return cv::Vec3b(0, 7 * m / 2 - 4 * c, m);
		case 7:
		case 8:
			return cv::Vec3b(0, 0, 9 * m / 2 - 4 * c);
		default:
			return cv::Vec3b(0, 0, 0);
	    }
	}
}

cv::Mat color(const cv::Mat& m) {
	cv::Mat c(m.size(), CV_8UC3);
	std::transform(m.begin<uchar>(), m.end<uchar>(), c.begin<cv::Vec3b>(), &colorize);
	return c;
}

void plotVelocity(cv::Mat& frame, const cv::Mat& uv, const cv::Mat& mask) {
	const int nx = grid_nx % 2 ? grid_nx - 1 : grid_nx;
	const int ny = grid_ny % 2 ? grid_ny - 1 : grid_ny;
	const int w = frame.size().width;
	const int h = frame.size().height;
	for (int i = 0; i < ny / 2; ++i) {
		for (int j = 0; j < nx / 2; ++j) {
			const int ii = h * (2 * i + 1) / ny;
			const int jj = w * (2 * j + 1) / nx;
			if (mask.at<uchar>(ii, jj)) {
				cv::Point p1, p2;
				p1.x = w * (2 * j + 1) / nx;
				p1.y = h * (2 * i + 1) / ny;
				cv::Vec2f v(uv.at<cv::Vec2f>(ii, jj));
				const int vx = boost::math::round(arrow_scale * v(0));
				const int vy = boost::math::round(arrow_scale * v(1));
				p2.x = p1.x + vx;
				p2.y = p1.y + vy;
				drawArrow(frame, p1, p2);
			}
		}
	}
}
