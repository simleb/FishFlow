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

#ifndef FISHFLOW_CONFIG_HPP
#define FISHFLOW_CONFIG_HPP

#include <stdexcept>
#include <boost/program_options.hpp>

namespace po = boost::program_options;


namespace FishFlow
{
    class Quit : public std::exception {};

    class Config : public po::variables_map
    {
    public:
        enum verbosity { QUIET, LOW, NORMAL, HIGH, DEBUG };

        Config(int argc, char **argv);
        enum verbosity verbosity() const { return _verbosity; }

    private:
        enum verbosity _verbosity;
    };

    template<class T>
    void replace(std::map<std::string, po::variable_value>& config,
                 const std::string& name, const T& val)
    {
        config[name].value() = boost::any(val);
    }

}

#endif
