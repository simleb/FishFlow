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

#include "Config.hpp"

#include <fstream>
#include <unistd.h>
#include <boost/algorithm/string.hpp>
#include "Input.hpp"
#include "Calc.hpp"
#include "Output.hpp"


namespace FishFlow
{

    Config::Config(int argc, char **argv)
    {
        using std::vector;
        using std::string;

        po::options_description cmdline_options("Command line options");
        cmdline_options.add_options()
        ("version,v", "print version string")
        ("help,h", "produce this help message")
        ("config_file,f", po::value< vector<string> >(), "config file")
        ("verbosity", po::value<string>()->default_value("Normal"), "One of: Quiet, Low, Normal, High, Debug");

        po::positional_options_description p;
        p.add("config_file", -1);

        po::options_description config_file_options;
        config_file_options.add(Input::options(*this));
        config_file_options.add(Calc::options());
        config_file_options.add(Output::options());

        cmdline_options.add(config_file_options);

        // Parse command line
        po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), *this);

        // Parse config files
        if (count("config_file"))
        {
            vector<string> files = operator[]("config_file").as< vector<string> >();
            for (vector<string>::const_iterator file = files.begin(); file != files.end(); ++file)
            {
                std::ifstream file_stream(file->c_str());
                po::store(po::parse_config_file(file_stream, config_file_options), *this);
            }
        }

        // Parse stdin
        if (!isatty(STDIN_FILENO))
        {
            po::store(po::parse_config_file(std::cin, config_file_options), *this);
        }

        // Set verbosity
        const string verbosity = this->operator[]("verbosity").as<string>();
        if (boost::iequals(verbosity, "Normal"))
            _verbosity = NORMAL;
        else if (boost::iequals(verbosity, "High"))
            _verbosity = HIGH;
        else if (boost::iequals(verbosity, "Low"))
            _verbosity = LOW;
        else if (boost::iequals(verbosity, "Quiet"))
            _verbosity = QUIET;
        else if (boost::iequals(verbosity, "Debug"))
            _verbosity = DEBUG;
        else
        {
            std::cerr << "Warning: Unknown verbosity level '" << verbosity << "'." << std::endl;
            std::cerr << "         Using default 'Normal' level instead." << std::endl;
        }

        // Respond to basic commands
        if (count("version"))
        {
            std::cout << "Version zero!" << std::endl;
            throw Quit();
        }

        if (count("help"))
        {
            std::cout << "Usage: fishFlow [options] [config_file] ..." << std::endl << std::endl;
            std::cout << "Synopsis: Compute velocity from videos of fish schools using optical flow";
            std::cout << std::endl << std::endl;
            std::cout << cmdline_options << std::endl;
            throw Quit();
        }

        // Run validation functions
        Input::validateInputFile(*this);
        Input::validateFrameCount(*this);
        Input::validateCrop(*this);
        Input::validateBackground(*this);
    }

}
