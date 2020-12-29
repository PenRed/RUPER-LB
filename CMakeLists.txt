
#
#
#    Copyright (C) 2020 Universitat Politècnica de València - UPV
#
#    This file is part of RUPER-LB: Runtime Unpredictable Performance Load Balancer.
#
#    RUPER-LB is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    RUPER-LB is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with RUPER-LB.  If not, see <https://www.gnu.org/licenses/>. 
#
#
#    contact emails:
#
#        vicent.gimenez.alventosa@gmail.com
#    
#

#----------------------------------------------------------------------------
# Check CMake version
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Project name
project(RUPER-LB LANGUAGES CXX)
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
option(WITH_MPI "Build with MPI " OFF)
option(WITH_HTTP "Build with HTTP " OFF)
#-----------------------------------------------------------------------------

#Add ruper-lb directory
add_subdirectory(${PROJECT_SOURCE_DIR}/src)
 
#Add exemple directories
add_subdirectory(${PROJECT_SOURCE_DIR}/examples/1)
