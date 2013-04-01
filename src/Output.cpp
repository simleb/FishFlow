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

#include "Output.hpp"

#include "Config.hpp"
#include <boost/algorithm/clamp.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/round.hpp>


namespace FishFlow
{

    Output::Output(const Config& config) :
    _frame(0),
    _count(config["frame.count"].as<size_t>()),
    _arrow_thickness(config["plot.style.arrow.thickness"].as<size_t>()),
    _arrow_head_size(config["plot.style.arrow.head_size"].as<size_t>()),
    _arrow_overlap(config["plot.style.arrow.overlap"].as<bool>()),
    _nx(config["output.width"].as<size_t>()),
    _ny(config["output.height"].as<size_t>()),
    _width(config["crop.width"].as<size_t>()),
    _height(config["crop.height"].as<size_t>()),
    _write_file(false),
    _8U_dtype(H5::PredType::NATIVE_UCHAR),
    _Vec2f_dtype(sizeof(cv::Vec2f))
    {
        // Initialize output videos plot types
        if (config.count("output.video.velocity"))
        {
            _plot_type.push_back(VELOCITY);
            _plot_path.push_back(config["output.video.velocity"].as<std::string>());
        }
        if (config.count("output.video.density"))
        {
            _plot_type.push_back(DENSITY);
            _plot_path.push_back(config["output.video.density"].as<std::string>());
        }
        if (config.count("output.video.velocity+original"))
        {
            _plot_type.push_back(ORIGINAL|VELOCITY|USE_MASK);
            _plot_path.push_back(config["output.video.velocity+original"].as<std::string>());
        }
        if (config.count("output.video.density+original"))
        {
            _plot_type.push_back(ORIGINAL|DENSITY);
            _plot_path.push_back(config["output.video.density+original"].as<std::string>());
        }
        if (config.count("output.video.velocity+density"))
        {
            _plot_type.push_back(VELOCITY|DENSITY);
            _plot_path.push_back(config["output.video.velocity+density"].as<std::string>());
        }
        if (config.count("output.video.velocity+density+original"))
        {
            _plot_type.push_back(VELOCITY|DENSITY|ORIGINAL);
            _plot_path.push_back(config["output.video.velocity+density+original"].as<std::string>());
        }

        for (size_t i = 0; i < _plot_type.size(); ++i)
        {
            // Add avi extension if needed
            if (!boost::algorithm::ends_with(_plot_path[i], ".avi"))
            {
                _plot_path[i] += ".avi";
            }

            // Create video writer
            _video_writer.push_back(cv::VideoWriter());
            _video_writer[i].open(_plot_path[i], CV_FOURCC('M','J','P','G'), 30, cv::Size(_width, _height));
        }

        // If output file was specified, use it unless it's empty
        // If no plots are specified, use default output name
        std::string filename;
        if (config.count("output.file"))
        {
            filename = config["output.file"].as<std::string>();
            if (filename.empty())
            {
                // throw std::runtime_error("The file path of the output hdf5 file is empty.");
                filename = config["input.file"].as<std::string>();
                size_t pos = filename.find_last_of('.');
                if (pos != std::string::npos)
                {
                    filename = filename.substr(0, pos + 1) + "h5";
                }
                else
                {
                    filename += ".h5";
                }
            }
            size_t pos = filename.find_last_of('.');
            if (pos != std::string::npos && filename.substr(pos + 1) != "h5")
            {
                filename += ".h5";
            }
            _write_file = true;
        }
        else if (_plot_type.empty())
        {
            filename = config["input.file"].as<std::string>();
            
            // Add extension or replace existing extension with .h5
            size_t pos = filename.find_last_of('.');
            if (pos != std::string::npos)
            {
                filename = filename.substr(0, pos + 1) + "h5";
            }
            else
            {
                filename += ".h5";
            }
            _write_file = true;
        }

        // Initialize hdf5 file
        if (_write_file)
        {
            _file = H5::H5File(filename, H5F_ACC_TRUNC);
            const hsize_t dims[3] = { _ny, _nx, _count };
            _mem_dspace = H5::DataSpace(2, dims);
            _file_dspace = H5::DataSpace(3, dims);
            _Vec2f_dtype.insertMember("x", 0, H5::PredType::NATIVE_FLOAT);
            _Vec2f_dtype.insertMember("y", sizeof(float), H5::PredType::NATIVE_FLOAT);
            _velocity_dset = _file.createDataSet("velocity", _Vec2f_dtype, _file_dspace);
            _density_dset = _file.createDataSet("density", _8U_dtype, _file_dspace);
        }
    }

    Output& Output::operator<<(const Calc::Output& out)
    {
        if (_write_file)
        {
            writeToFile(out);
        }
        
        for (size_t i = 0; i < _plot_type.size(); ++i)
        {
            cv::Mat composite;
            const bool transparent = _plot_type[i] & ORIGINAL;
            
            if (transparent)
            {
                composite = out.original;
            }
            else
            {
                composite = cv::Mat::zeros(out.original.size(), CV_8UC3);
            }
            
            if (_plot_type[i] & DENSITY)
            {
                plotDensity(composite, out.density, transparent);
            }
            
            if (_plot_type[i] & (VELOCITY | USE_MASK))
            {
                plotVelocity(composite, out.velocity, out.mask);
            }
            else if (_plot_type[i] & VELOCITY)
            {
                plotVelocity(composite, out.velocity);
            }

            _video_writer[i] << composite;
        }

        ++_frame;

        return *this;
    }

    void Output::writeToFile(const Calc::Output& out)
    {
        cv::Mat buffer(_ny, _nx, CV_8U);
        const hsize_t count[3] = { _ny, _nx, 1 };
        const hsize_t start[3] = { 0, 0, _frame };
        _file_dspace.selectHyperslab(H5S_SELECT_SET, count, start);

        // Velocity
        _velocity_dset.write(out.velocity.ptr(), _Vec2f_dtype, _mem_dspace, _file_dspace);

        // Density
        cv::resize(out.density, buffer, buffer.size());
        _density_dset.write(buffer.ptr(), _8U_dtype, _mem_dspace, _file_dspace);
    }

    namespace
    {

        cv::Vec3b colorize(const unsigned char c)
        {
            const unsigned char maxval = 255;
            switch (c * 8 / maxval)
            {
                case 0:
                    return cv::Vec3b(4*c + maxval/2, 0, 0);
                case 1:
                case 2:
                    return cv::Vec3b(maxval, 4*c - maxval/2, 0);
                case 3:
                case 4:
                    return cv::Vec3b(5*maxval/2 - 4*c, maxval, 4*c - 3*maxval/2);
                case 5:
                case 6:
                    return cv::Vec3b(0, 7*maxval/2 - 4*c, maxval);
                case 7:
                case 8:
                    return cv::Vec3b(0, 0, 9*maxval/2 - 4*c);
                default:
                    return cv::Vec3b(0, 0, 0);
            }
        }

    }

    void Output::plotDensity(cv::Mat& frame, const cv::Mat& density, bool transparent)
    {
        if (transparent)
        {
            cv::Mat buffer(frame.size(), CV_8UC3);
            std::transform(density.begin<unsigned char>(), density.end<unsigned char>(),
                           buffer.begin<cv::Vec3b>(), &colorize);
            cv::addWeighted(frame, 0.5, buffer, 0.5, 0, frame);
        }
        else
        {
            std::transform(density.begin<unsigned char>(), density.end<unsigned char>(),
                           frame.begin<cv::Vec3b>(), &colorize);
        }
    }

    void Output::plotVelocity(cv::Mat& frame, const cv::Mat& velocity, const cv::Mat& mask)
    {
        const bool no_mask = mask.empty();
        const size_t nx = _nx % 2 ? _nx - 1 : _nx;
        const size_t ny = _ny % 2 ? _ny - 1 : _ny;
        for (int i = 0; i < ny / 2; ++i)
        {
            for (int j = 0; j < nx / 2; ++j)
            {
                if (no_mask || mask.at<unsigned char>(2 * i + 1, 2 * j + 1))
                {
                    cv::Point p1, p2;
                    p1.x = _width * (2 * j + 1) / nx;
                    p1.y = _height * (2 * i + 1) / ny;
                    const int vx = boost::math::round(velocity.at<float>(2 * i + 1, 2 * j + 1, 0));
                    const int vy = boost::math::round(velocity.at<float>(2 * i + 1, 2 * j + 1, 1));
                    if (_arrow_overlap)
                    {
                        p2.x = p1.x + vx;
                        p2.y = p1.y + vy;
                    }
                    else
                    {
                        const int dx = _width  / nx;
                        const int dy = _height / ny;
                        p2.x = p1.x + boost::algorithm::clamp(vx, -dx, dx);
                        p2.y = p1.y + boost::algorithm::clamp(vy, -dy, dy);
                    }
                    drawArrow(frame, p1, p2);
                }
            }
        }
    }

    namespace
    {
        const double pi = boost::math::constants::pi<double>();
    }
    
    void Output::drawArrow(cv::Mat& frame, const cv::Point& p1, const cv::Point& p2)
    {
        cv::line(frame, p1, p2, cv::Scalar(0, 0, 255), _arrow_thickness, CV_AA);
        const double theta = atan2(p2.y - p1.y, p2.x - p1.x);
        cv::Point p;
        p.x = p2.x - boost::math::round(_arrow_head_size * cos(theta + pi/4));
        p.y = p2.y - boost::math::round(_arrow_head_size * sin(theta + pi/4));
        cv::line(frame, p, p2, cvScalar(0, 0, 255), _arrow_thickness, CV_AA);
        p.x = p2.x - boost::math::round(_arrow_head_size * cos(theta - pi/4));
        p.y = p2.y - boost::math::round(_arrow_head_size * sin(theta - pi/4));
        cv::line(frame, p, p2, cvScalar(0, 0, 255), _arrow_thickness, CV_AA);
    }

    po::options_description Output::options()
    {
        po::options_description options("Output");
        options.add_options()
        ("output.file,o", po::value<std::string>()->implicit_value(""),
         "path of the output hdf5 file")
        ("output.width", po::value<size_t>()->default_value(128),
         "horizontal resolution of the output grid")
        ("output.height", po::value<size_t>()->default_value(64),
         "vertical resolution of the output grid")
        ("output.video.velocity", po::value<std::string>(),
         "path to output velocity video")
        ("output.video.density", po::value<std::string>(),
         "path to output density video")
        ("output.video.velocity+original", po::value<std::string>(),
         "path to output velocity+original video")
        ("output.video.density+original", po::value<std::string>(),
         "path to output density+original video")
        ("output.video.velocity+density", po::value<std::string>(),
         "path to output velocity+density video")
        ("output.video.velocity+density+original", po::value<std::string>(),
         "path to output velocity+density+original video")
        ("plot.style.arrow.thickness", po::value<size_t>()->default_value(2),
         "thickness of arrows on plots")
        ("plot.style.arrow.head_size", po::value<size_t>()->default_value(4),
         "size of arrow heads on plots")
        ("plot.style.arrow.overlap", po::value<bool>()->default_value(true),
         "allow arrows to overlap?");

        return options;
    }

}
