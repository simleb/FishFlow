# FishFlow

## What is FishFlow

FishFlow is a tool to compute the [optical flow][] of moving particles.
It loads images from a video file and outputs data and videos of the optical flow.

It works by detecting spatial and temporal correlations of pixel intensities.

It was built in order to extract the motion of schooling fish in a laboratory setting, hence the name. All the default parameters are tuned for these specific videos but they can be adjusted to meet other needs.

[optical flow]: http://en.wikipedia.org/wiki/Optical_flow

## Build information

The build process is managed by [CMake][]. The easiest way to build is to open in your terminal the root directory of the FishFlow repository and to type:

	mkdir build
	cd build
	cmake ..
	make

CMake Tips: You can use different generators. For instance to generate a Xcode project, use `cmake -G Xcode ..`. You can also use the ncurses interface to cmake with `ccmake ..`.

But first you need to make sure that the following dependencies are installed on your computer:

- [CMake][] for the build process
- [OpenCV][] for the image processing
- [HDF5][] (including the C++ bindings) for the data output
- [Boost][] (including Program Options) for the options input

[CMake]: http://cmake.org
[OpenCV]: http://opencv.org
[HDF5]: http://hdfgroup.org/HDF5/
[Boost]: http://boost.org

## How to use it

FishFlow is a command line tool. If the executable is in your current directory, type `./fishFlow` to run it.

You can show usage information and a list of options with `./fishFlow -h` or `./fishFlow --help`.

Note: If you want to drop the `./` prefix, you need to install the executable somewhere in your `$PATH`.
For instance you can do `cp fishFlow /usr/local/bin`. After this, `fishFlow -h` will work from any directory. From now on, I will assume you installed it properly.

### Options

Options can be specified in three ways:

- On the command line
- In a config file
- On the standard input

Note for C++ Programmers: The options are handled by the Boost library [Program Options][]. Please refer to Boost's documentation for more information.

[Program Options]: www.boost.org/doc/html/program_options.html

Type `fishFlow -h` to see the list of options. An option looks like a list of words separated by dots. The first words can be seen as *sections* while the last word is the specific option within that section. For instance `frame.count` is the total number of frames from the input video that you want to process. You can specify it in a config file by writing:

	frame.count = 100

or

	[frame]
		count = 100

then run `fishFlow my_config_file.ini` (or `fishFlow < my_config_file.ini`).

Or you can write it directly on the command line like this:

	fishFlow --frame.count 100

Have a look at `config.ini` for a more complete example of config file.

### Required options

At the minimum, you need to provide an input file. If this is all you specify, FishFlow will produce an output data file in the same directory and with the same name as the input video (but with an additional `.flow.h5` extension).

## License

The source for FishFlow is released under the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
