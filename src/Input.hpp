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

#ifndef FISHFLOW_INPUT_HPP
#define FISHFLOW_INPUT_HPP

#include "Calc.hpp"
#include <opencv2/opencv.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;


namespace FishFlow
{
    class Config;

    class Input
    {
    public:
        Input(const Config& config);
        Input& operator>>(Calc::Input& frames);
        operator bool() const;

        static po::options_description options(Config& config);

        // Validation functions
        static void validateInputFile(Config& config);
        static void validateBackground(Config& config);
        static void validateFrameCount(Config& config);
        static void validateCrop(Config& config);

    private:
        void showProgress() const;
        void skipFrames(const size_t n);
        void computeBackgroundImage(const Config& config);

        cv::VideoCapture _capture;
        size_t _frame;
        size_t _start;
        size_t _stop;
        size_t _step;
        cv::Rect _ROI;
        bool _show_progress;
    };
    
}

#endif
