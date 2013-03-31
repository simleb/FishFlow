/* Copyright (c) 2013 Simon Leblanc

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "Calc.hpp"

#include "Config.hpp"
#include <boost/math/special_functions/round.hpp>


namespace FishFlow
{

    Calc::Calc(const Config& config) :
    _scale(config["calc.scale"].as<double>()),
    _nx(config["output.width"].as<size_t>()),
    _ny(config["output.height"].as<size_t>()),
    _width(config["crop.width"].as<size_t>()),
    _height(config["crop.height"].as<size_t>())
    {
        const size_t window_size = config["calc.window_size"].as<size_t>() | 1;
        _window_size = cv::Size(window_size, window_size);

        // Init background image
        cv::Rect ROI(config["crop.xmin"].as<size_t>(), config["crop.ymin"].as<size_t>(),
                     config["crop.width"].as<size_t>(), config["crop.height"].as<size_t>());
        if (config.count("input.background"))
        {
            _background = cv::imread(config["input.background"].as<std::string>(),
                                            CV_LOAD_IMAGE_GRAYSCALE);
            if (_background.size() != ROI.size())
            {
                _background = _background(ROI);
            }
        }
        else
        {
            _background = cv::Mat::ones(ROI.size(), CV_8UC1) * 255;
        }

        // Init matrices
        const int dims[] = { _ny, _nx, 2 };
        _out.density.create(_height, _width, CV_8U);
        _out.velocity.create(3, dims, CV_32F);
        _out.mask.create(2, dims, CV_8U);
    }

    const Calc::Output& Calc::operator()(const Input& frames)
    {
        cv::Mat old, current;
        cv::cvtColor(frames.old, old, CV_BGR2GRAY);
        cv::cvtColor(frames.current, current, CV_BGR2GRAY);

        frames.current.copyTo(_out.original);

        old = 255 + (old - _background);
        current = 255 + (current - _background);

        computeDensity(current);
        computeDensityMask(current);
        computeAlignment(current);
        computeVelocity(old, current);

        return _out;
    }

    void Calc::computeDensity(const cv::Mat& frame)
    {
//        cv::adaptiveThreshold(frame, _out.density, 255,
//                              cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 5, 0);
        cv::threshold(frame, _out.density, 200, 255, cv::THRESH_BINARY);
        cv::GaussianBlur(_out.density, _out.density, cv::Size(101, 101), 0, 0);
        _out.density = (255 - _out.density) * 4;
    }

    void Calc::computeDensityMask(const cv::Mat& frame)
    {
        cv::Mat buffer;
        cv::threshold(frame, buffer, 200, 255, cv::THRESH_BINARY);
        const cv::Mat disk((cv::Mat_<unsigned char>(5,5) << 0,1,1,1,0, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 0,1,1,1,0));
        cv::erode(buffer, buffer, disk, cv::Point(-1, -1), 10);
        cv::bitwise_not(buffer, buffer);
        cv::resize(buffer, _out.mask, _out.mask.size());
    }

    void Calc::computeAlignment(const cv::Mat& frame)
    {

    }

    void Calc::computeVelocity(const cv::Mat& old, const cv::Mat& current)
    {
        // Smooth
//        cv::GaussianBlur(current, current, cv::Size(3, 3), 0, 0);
//        cv::dilate(current, current, cv::Mat());

        // Moments
        cv::Mat_<double> It = current - old;
        cv::Mat_<double> Ix, Iy;
        cv::Sobel(old, Ix, CV_64F, 1, 0);
        cv::Sobel(old, Iy, CV_64F, 0, 1);
        cv::Mat_<double> Ixy = Ix.mul(Iy);
        cv::Mat_<double> Ixt = Ix.mul(It);
        cv::Mat_<double> Iyt = Iy.mul(It);
        Ix = Ix.mul(Ix);
        Iy = Iy.mul(Iy);

        // Convolution
        cv::GaussianBlur(Ix , Ix , _window_size, 0, 0);
        cv::GaussianBlur(Iy , Iy , _window_size, 0, 0);
        cv::GaussianBlur(Ixy, Ixy, _window_size, 0, 0);
        cv::GaussianBlur(Ixt, Ixt, _window_size, 0, 0);
        cv::GaussianBlur(Iyt, Iyt, _window_size, 0, 0);

        // Computation
        for (size_t i = 0; i < _ny; ++i)
        {
            for (size_t j = 0; j < _nx; ++j)
            {
                const size_t ii = boost::math::round(It.size().height * (i + 0.5) / _ny);
                const size_t jj = boost::math::round(It.size().width  * (j + 0.5) / _nx);
                cv::Matx22d M;
                M(0, 0) = Ix(ii, jj);
                M(0, 1) = Ixy(ii, jj);
                M(1, 0) = Ixy(ii, jj);
                M(1, 1) = Iy(ii, jj);
                cv::Matx21d b, x;
                b(0) = -Ixt(ii, jj);
                b(1) = -Iyt(ii, jj);
                cv::solve(M, b, x);
                _out.velocity.at<float>(i, j, 0) = _scale * x(0);
                _out.velocity.at<float>(i, j, 1) = _scale * x(1);
            }
        }

        // Smooth
//        cv::GaussianBlur(_out.velocity, _out.velocity, cv::Size(3, 3), 0, 0);
    }

    po::options_description Calc::options()
    {
        po::options_description options("Calc");
        options.add_options()
        ("calc.window_size", po::value<size_t>()->default_value(45), "window size")
        ("calc.smooth_size", po::value<size_t>()->default_value(30), "smooth size")
        ("calc.scale", po::value<double>()->default_value(100), "scale")
        ("calc.angular_resolution", po::value<size_t>()->default_value(6), "angular resolution");
        return options;
    }


}
