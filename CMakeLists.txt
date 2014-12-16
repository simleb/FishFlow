# Copyright (c) 2013 Simon Leblanc
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
	src/plot.hpp
	src/plot.cpp
	src/main.cpp
)

# Create main target
add_executable(fishFlow ${SOURCES})
target_link_libraries(fishFlow ${LIBRARIES})
if(GOOGLE_PERFTOOLS_PROFILER)
	target_link_libraries(fishFlow debug ${GOOGLE_PERFTOOLS_PROFILER})
endif()

# Enable all warnings
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang|GNU|Intel")
	list(APPEND CMAKE_CXX_FLAGS "-Wall")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	list(APPEND CMAKE_CXX_FLAGS "/W4")
endif()
