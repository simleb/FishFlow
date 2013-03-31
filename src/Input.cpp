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

#include "Input.hpp"

#include <iostream>
#include <iomanip>
#include <sys/ioctl.h>
#include <unistd.h>
#include "Config.hpp"


namespace FishFlow
{

    Input::Input(const Config& config) :
    _capture(config["input.file"].as<std::string>()),
    _frame(0),
    _from(config["frame.from"].as<size_t>()),
    _to(config["frame.to"].as<size_t>()),
    _by(config["frame.by"].as<size_t>()),
    _ROI(config["crop.xmin"].as<size_t>(), config["crop.ymin"].as<size_t>(),
         config["crop.width"].as<size_t>(), config["crop.height"].as<size_t>()),
    _show_progress(config.verbosity() >= Config::NORMAL)
    {
        if (!_capture.isOpened())
        {
            throw std::runtime_error("Video capture could not be open.");
        }

        if (config.count("output.background.file"))
            computeBackgroundImage(config);

        skipFrames(_from);
    }


    void Input::skipFrames(const size_t n)
    {
        // Skip frames instead of setting CV_CAP_PROP_POS_FRAMES to avoid issue with keyframes
        for (size_t i = 0; i < n; ++i) _capture.grab();
    }

    
    void Input::computeBackgroundImage(const Config& config)
    {
        const cv::Size max_size(config["crop.max_width"].as<size_t>(),
                                config["crop.max_height"].as<size_t>());
        const cv::Rect rect(config["output.background.cropped"].as<bool>() ?
                            _ROI : cv::Rect(cv::Point(0, 0), max_size));
        cv::Mat background(rect.size(), CV_32FC1, cv::Scalar::all(0)), color, gray;
        while (_capture.read(color))
        {
            cv::cvtColor(color(rect), gray, CV_BGR2GRAY, 1);
            cv::accumulate(gray, background);
        }
        background /= _capture.get(CV_CAP_PROP_FRAME_COUNT);
        cv::imwrite(config["output.background.file"].as<std::string>(), background);
        _capture.set(CV_CAP_PROP_POS_FRAMES, 0);
    }


    Input& Input::operator>>(Calc::Input& frames)
    {
        cv::Mat buffer;
        if (frames.old.empty())
        {
            _capture >> buffer;
            buffer(_ROI).copyTo(frames.old);

            skipFrames(_by);
            _frame += _by;
        }
        else
        {
            frames.current.copyTo(frames.old);
        }

        _capture >> buffer;
        buffer(_ROI).copyTo(frames.current);

        skipFrames(_by);
        _frame += _by;

        return *this;
    }


    Input::operator bool() const
    {
        if (_show_progress) showProgress();
        return _frame <= _to;
    }


    void Input::showProgress() const
    {
        if (isatty(STDOUT_FILENO))
        {
            // Get terminal width
            size_t cols = 80;
            struct winsize w;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
            if (w.ws_col > 0) cols = w.ws_col;

            // Display percentage
            const size_t frame = _frame - _by;
            std::cout << '\r' << std::setw(3) << (frame - _from) * 100 / (_to - _from) << '%' << std::flush;

            // Display progress bar
            if (cols > 11)
            {
                const size_t pos = 2 + (frame - _from) * (cols - 10) / (_to - _from);
                std::cout << " [";
                for (size_t i = 1; i < pos - 1; ++i) std::cout << '~';
                std::cout << "><>";
                for (size_t i = pos + 2; i < cols - 6; ++i) std::cout << ' ';
                std::cout << ']' << std::flush;
            }

            // End the line if progress reaches 100%
            if (frame >= _to) std::cout << std::endl;
        }
        else
        {
            // Display plain percentage on a new line
            std::cout << (_frame - _by - _from) * 100 / (_to - _from) << std::endl;
        }
    }


    po::options_description Input::options(Config& config)
    {
        po::options_description options("Input");
        options.add_options()
        ("input.file,i", po::value<std::string>(), "path of the input file")
        ("input.background,b", po::value<std::string>(), "path of the background image")
        ("frame.from", po::value<size_t>()->default_value(1), "first frame (starts at 0)")
        ("frame.to" , po::value<size_t>(), "last frame (not included)")
        ("frame.count" , po::value<size_t>(), "number of frames")
        ("frame.by" , po::value<size_t>()->default_value(1), "by frames")
        ("crop.xmin", po::value<size_t>()->default_value(0), "min x coord of crop rectangle")
        ("crop.ymin", po::value<size_t>()->default_value(0), "min y coord of crop rectangle")
        ("crop.xmax", po::value<size_t>(), "max x coord of crop rectangle")
        ("crop.ymax", po::value<size_t>(), "max y coord of crop rectangle")
        ("crop.width", po::value<size_t>(), "width of crop rectangle")
        ("crop.height", po::value<size_t>(), "height of crop rectangle");
        return options;
    }

    void Input::validateInputFile(Config& config)
    {
        using namespace std;

        const string path(config.count("input.file") ? config["input.file"].as<string>() : "");
        if (path.empty())
        {
            throw runtime_error("Input file was not specified.");
        }

        try
        {
            cv::VideoCapture cap(path);
            if (!cap.isOpened()) throw runtime_error("Bad");

            replace(config, "frame.max_count", static_cast<size_t>(cap.get(CV_CAP_PROP_FRAME_COUNT)));
            replace(config, "crop.max_width", static_cast<size_t>(cap.get(CV_CAP_PROP_FRAME_WIDTH)));
            replace(config, "crop.max_height", static_cast<size_t>(cap.get(CV_CAP_PROP_FRAME_HEIGHT)));
        }
        catch (const std::exception& e)
        {
            if (config.verbosity() >= Config::HIGH)
            {
                cerr << std::endl;
                cerr << "  The input video '" << path << "' could not be open." << endl;
                cerr << "  This might be due to a few things." << endl;
                cerr << "    1) An error in the file specified (wrong directory, typo, spaces in name, file does not exist...)." << endl;
                cerr << "       Double check the path you entered: " << path << endl;
                cerr << "    2) A problem of permissions. Make sure you have read access to the input file." << endl;
                cerr << "    3) A non-supported video format." << endl;
                cerr << "       Make sure your video file can be read properly using your default media player (QuickTime, Totem, Windows Media Player)." << endl;
                cerr << "       You can try to convert the video to a different format using ffmpeg for instance." << endl;
                cerr << "       E.g. > ffmpeg -i \"" << path << "\" video.avi" << endl << endl;
            }
            throw runtime_error("The input video '" + path + "' could not be open.");
        }

        replace(config, "input.valid", true);
    }


    void Input::validateBackground(Config& config)
    {
        if (!config.count("input.background")) return;

        using namespace std;

        const string path(config["input.background"].as<string>());

        cv::Mat background;
        try
        {
            background = cv::imread(path);
        }
        catch (const exception& e)
        {
            throw runtime_error("The background image '" + path + "' could not be open.");
        }

        const cv::Size max_size(config["crop.max_width"].as<size_t>(),
                                config["crop.max_height"].as<size_t>());
        const cv::Size crop_size(config["crop.width"].as<size_t>(),
                                 config["crop.height"].as<size_t>());
        if (background.size() != max_size && background.size() != crop_size)
        {
            if (config.verbosity() >= Config::HIGH)
            {
                cerr << std::endl;
                cerr << "  The background image size is incorrect." << endl;
                cerr << "  It should either have the same size as the input video" << endl;
                cerr << "  or as the crop region." << endl;
                cerr << endl;
            }
            throw runtime_error("The background image size is incorrect.");
        }
    }


    /* Formula: count == (to - from) / by */
    void Input::validateFrameCount(Config& config)
    {
        using namespace std;
        
        const size_t from = config["frame.from"].as<size_t>();
        const size_t by = config["frame.by"].as<size_t>();
        const size_t max_count = config["frame.max_count"].as<size_t>();
        size_t to = max_count;
        size_t count = max_count - 1;

        if (config.count("frame.to"))
            to = config["frame.to"].as<size_t>();

        if (config.count("frame.count"))
            count = config["frame.count"].as<size_t>();

        stringstream help_string;
        help_string << endl;
        help_string << "  The parameters specified for the frames to process are not consistent." << endl;
        help_string << "  Two frames are needed to compute optical flow." << endl;
        help_string << "  Here are the rules:" << endl;
        help_string << "    1) 0 < from < to < " << max_count << ": the number of frames in the video" << endl;
        help_string << "    2) by > 0" << endl;
        help_string << "    3) If count != (to - from) / by, the smallest time interval is chosen" << endl;
        help_string << endl;

        if (from == 0)
        {
            if (config.verbosity() >= Config::HIGH) cerr << help_string.str();
            throw runtime_error("frame.from == 0 !");
        }
        if (by < 1)
        {
            if (config.verbosity() >= Config::HIGH) cerr << help_string.str();
            throw runtime_error("frame.by < 1 !");
        }
        if (from > to)
        {
            if (config.verbosity() >= Config::HIGH) cerr << help_string.str();
            throw runtime_error("frame.from > frame.to !");
        }
        if (to > max_count)
        {
            if (config.verbosity() >= Config::HIGH) cerr << help_string.str();
            throw runtime_error("frame.to is greater than the number of frames in the video !");
        }
        if ((to - from) / by != count)
        {
            if (config.count("frame.to") && config.count("frame.count"))
            {
                if ((to - from) / by > count)
                {
                    to = from + count * by;
                    if (config.verbosity() >= Config::LOW)
                    {
                        cerr << "Warning: frame.to > frame.from + frame.count * frame.by" << endl;
                        cerr << "         Setting it to the smallest value (" << to << ")" << endl;
                    }
                }
                else
                {
                    count = (to - from) / by;
                    if (config.verbosity() >= Config::LOW)
                    {
                        cerr << "Warning: frame.count > (frame.to - frame.from) / frame.by;" << endl;
                        cerr << "         Setting it to the smallest value (" << count << ")" << endl;
                    }
                }
            }
            else if (config.count("frame.count"))
            {
                to = from + count;
            }
            else
            {
                count = (to - from) / by;
            }
        }
        if (from == to)
        {
            if (config.verbosity() >= Config::LOW)
            {
                cerr << "Warning: frame.from == frame.to" << endl;
                cerr << "         At least two frames are necessary to";
                cerr << " compute optical flow" << endl;
                cerr << "         No output produced." << endl;
            }
            throw Quit();
        }
        if (to > max_count)
        {
            if (config.verbosity() >= Config::LOW)
            {
                cerr << "Warning: frame.to is larger than the number of frames in the input video." << endl;
                cerr << "         Setting it to the maximum (" << count << ")" << endl;
            }
            to = count;
        }

        replace(config, "frame.to", to);
        replace(config, "frame.count", count);
    }


    void Input::validateCrop(Config& config)
    {
        const size_t max_width = config["crop.max_width"].as<size_t>();
        const size_t max_height = config["crop.max_height"].as<size_t>();
        const size_t xmin = config["crop.xmin"].as<size_t>();
        const size_t ymin = config["crop.ymin"].as<size_t>();
        size_t xmax = max_width;
        size_t ymax = max_height;
        size_t width = max_width - xmin;
        size_t height = max_height - ymin;

        if (config.count("crop.xmax"))
            xmax = config["crop.xmax"].as<size_t>();
        if (config.count("crop.ymax"))
            ymax = config["crop.ymax"].as<size_t>();

        if (config.count("crop.width"))
            width = config["crop.width"].as<size_t>();
        if (config.count("crop.height"))
            height = config["crop.height"].as<size_t>();

        std::stringstream help_string;
        help_string << std::endl;
        help_string << "  The parameters specified for the crop region are not consistent." << std::endl;
        help_string << "  Here are the rules:" << std::endl;
        help_string << "    1) If (xmax - xmin) != width or (ymax - ymin) != height, the smallest rectangle is used (with a warning)." << std::endl;
        help_string << "    2) xmin < xmax && ymin < ymax" << std::endl;
        help_string << "    3) 0 < width <= " << max_width << ": the width of the input video" << std::endl;
        help_string << "    4) 0 < height <= " << max_height << ": the height of the input video" << std::endl;
        help_string << std::endl;

        if (xmin >= xmax)
        {
            if (config.verbosity() >= Config::HIGH) std::cerr << help_string.str();
            throw std::runtime_error("xmin >= xmax !");
        }

        if (ymin >= ymax)
        {
            if (config.verbosity() >= Config::HIGH) std::cerr << help_string.str();
            throw std::runtime_error("ymin >= ymax !");
        }

        if (width == 0)
        {
            if (config.verbosity() >= Config::HIGH) std::cerr << help_string.str();
            throw std::runtime_error("width == 0 !");
        }

        if (height == 0)
        {
            if (config.verbosity() >= Config::HIGH) std::cerr << help_string.str();
            throw std::runtime_error("height == 0 !");
        }

        if (xmax - xmin > width)
        {
            xmax = xmin + width;
            if (config.count("crop.xmax") && config.verbosity() >= Config::LOW)
            {
                std::cerr << "Warning: in crop parameters: xmax - xmin > width" << std::endl;
                std::cerr << "         Setting xmax to be equal to width (" << xmax << ")" << std::endl;
            }
            
        }
        else if (xmax - xmin < width)
        {
            width = xmax - xmin;
            if (config.count("crop.width") && config.verbosity() >= Config::LOW)
            {
                std::cerr << "Warning: in crop parameters: xmax - xmin < width" << std::endl;
                std::cerr << "         Setting width to be equal to xmax - xmin (" << width << ")" << std::endl;
            }
        }

        if (ymax - ymin > height)
        {
            ymax = ymin + height;
            if (config.count("crop.ymax") && config.verbosity() >= Config::LOW)
            {
                std::cerr << "Warning: in crop parameters: ymax - ymin > height" << std::endl;
                std::cerr << "         Setting ymax to be equal to height (" << ymax << ")" << std::endl;
            }
        }
        else if (ymax - ymin < height)
        {
            height = ymax - ymin;
            replace(config, "crop.height", height);
            if (config.count("crop.height") && config.verbosity() >= Config::LOW)
            {
                std::cerr << "Warning: in crop parameters: ymax - ymin < height" << std::endl;
                std::cerr << "         Setting height to be equal to ymax - ymin (" << height << ")" << std::endl;
            }
        }

        if (xmax > max_width)
        {
            if (config.verbosity() >= Config::LOW)
            {
                std::cerr << "Warning: crop.xmax is larger than the width of the input video." << std::endl;
                std::cerr << "         Setting it to the maximum (" << max_width << ")" << std::endl;
            }
            xmax = max_width;
            width = xmax - xmin;
        }

        if (ymax > max_height)
        {
            if (config.verbosity() >= Config::LOW)
            {
                std::cerr << "Warning: crop.ymax is larger than the height of the input video." << std::endl;
                std::cerr << "         Setting it to the maximum (" << max_height << ")" << std::endl;
            }
            ymax = max_height;
            height = ymax - ymin;
        }

        replace(config, "crop.xmax", xmax);
        replace(config, "crop.ymax", ymax);
        replace(config, "crop.width", width);
        replace(config, "crop.height", height);
    }


}
