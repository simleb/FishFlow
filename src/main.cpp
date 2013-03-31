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
#include "Input.hpp"
#include "Calc.hpp"
#include "Output.hpp"


int main(int argc, char* argv[])
{
    using namespace FishFlow;

    bool quiet;
    try
    {
        // Parse command line, config files and standard input
        Config config(argc, argv);

        quiet = config.verbosity() == Config::QUIET;

        // Open input stream
        Input input(config);

        // Create calculator
        Calc calc(config);

        // Create output files
        Output output(config);

        // Main loop
        Calc::Input frames;
        while (input >> frames)
        {
            output << calc(frames);
        }

        return 0;
    }
    catch (const Quit& e)
    {
        return 0;
    }
    catch (const std::exception& e)
    {
        if (!quiet) std::cerr << "Fatal: " << e.what() << std::endl;
        return -1;
    }
}
