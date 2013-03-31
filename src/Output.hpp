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

#ifndef FISHFLOW_OUTPUT_HPP
#define FISHFLOW_OUTPUT_HPP

#include "Calc.hpp"
#include <H5Cpp.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;


namespace FishFlow
{
    class Config;

    class Output
    {
    public:
        Output(const Config& config);
        Output& operator<<(const Calc::Output& out);

        static po::options_description options();

    private:
        void writeToFile(const Calc::Output& out);

        void plotDensity(cv::Mat& frame, const cv::Mat& density, bool transparent = false);
        void plotAlignment(cv::Mat& frame, const cv::Mat& alignement, bool transparent = false);
        void plotVelocity(cv::Mat& frame, const cv::Mat& velocity, const cv::Mat& mask = cv::Mat());

        void drawArrow(cv::Mat& frame, const cv::Point& p1, const cv::Point& p2);

        size_t _frame;
        size_t _count;
        size_t _arrow_thickness;
        size_t _arrow_head_size;
        bool _arrow_overlap;
        size_t _nx;
        size_t _ny;
        size_t _width;
        size_t _height;
        enum PlotType {
            ORIGINAL  = 1 << 0,
            ALIGNMENT = 1 << 1,
            DENSITY   = 1 << 2,
            VELOCITY  = 1 << 3,
            USE_MASK  = 1 << 4
        };
        std::vector<uchar> _plot_type;
        std::vector<std::string> _plot_path;
        std::vector<cv::VideoWriter> _video_writer;

        // Hdf5
        bool _write_file;
        H5::H5File _file;
        H5::DataSet _velocity_dset;
        H5::DataSet _density_dset;
        H5::DataSet _alignment_dset;
        H5::DataType _8U_dtype;
        H5::CompType _Vec2f_dtype;
        H5::DataSpace _mem_dspace;
        H5::DataSpace _file_dspace;

    };
    
}

#endif
