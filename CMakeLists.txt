# Copyright (c) 2012 Simon Leblanc, for the Couzin Lab @ Princeton University
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

cmake_minimum_required(VERSION 2.8)
project(fishFlow CXX)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE Release
        CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

# ---------------------------------------------------------------------------- #
# Find required libraries: OpenCV, HDF5, Boost
# ---------------------------------------------------------------------------- #

# Find OpenCV
find_package(OpenCV REQUIRED core highgui)
include("${OpenCV_DIR}/OpenCVConfig.cmake")
include_directories(${OpenCV_INCLUDE_DIRS})
list(APPEND LIBRARIES ${OpenCV_LIBRARIES})

# Find HDF5
find_package(HDF5 REQUIRED CXX)
include_directories(${HDF5_INCUDE_DIRS})
list(APPEND LIBRARIES ${HDF5_LIBRARIES})

# Find Boost
find_package(Boost REQUIRED program_options)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND LIBRARIES ${Boost_LIBRARIES})

# ---------------------------------------------------------------------------- #
# Find optional libraries: Google Perftools
# ---------------------------------------------------------------------------- #

# Find Google Perftools
find_library(GOOGLE_PERFTOOLS_TCMALLOC tcmalloc)
find_library(GOOGLE_PERFTOOLS_PROFILER profiler)

# ---------------------------------------------------------------------------- #
# List all sources and headers
# ---------------------------------------------------------------------------- #

# Add sources and headers
set(SOURCES
	src/Calc.hpp
	src/Calc.cpp
	src/Config.hpp
	src/Config.cpp
	src/Input.hpp
	src/Input.cpp
    src/Output.hpp
    src/Output.cpp
	src/main.cpp
)

# Create main target
add_executable(fishFlow ${SOURCES})
target_link_libraries(fishFlow ${LIBRARIES})
if(GOOGLE_PERFTOOLS_PROFILER)
    target_link_libraries(fishFlow debug ${GOOGLE_PERFTOOLS_PROFILER})
endif()

# Enable all warnings
if(CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
	add_definitions(-Wall)
endif()

# C++11
#if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
#    list(APPEND CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")
#else()
#    list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
#endif()

