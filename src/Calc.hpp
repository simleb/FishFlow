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

#ifndef FISHFLOW_CALC_HPP
#define FISHFLOW_CALC_HPP

#include <opencv2/opencv.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;


namespace FishFlow
{
    class Config;

    class Calc
    {
    public:
        struct Input
        {
            cv::Mat current;
            cv::Mat old;
        };
        struct Output
        {
            cv::Mat original;
            cv::Mat density;
            cv::Mat mask;
            cv::Mat alignment;
            cv::Mat velocity;
        };

        Calc(const Config& config, const cv::Mat& background);
        const Output& operator()(const Input& frames);

        static po::options_description options();

    private:
        void computeDensity(const cv::Mat& frame);
        void computeDensityMask(const cv::Mat& frame);
        void computeAlignment(const cv::Mat& frame);
        void computeVelocity(const cv::Mat& old, const cv::Mat& current);

        Output _out;
        cv::Mat _background;
        cv::Size _window_size;
        double _scale;
        size_t _nx;
        size_t _ny;
        size_t _width;
        size_t _height;
    };

}

#endif
