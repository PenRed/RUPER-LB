
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



#############################
#  Load Balance 
#############################

if(WITH_MPI)
    #Find MPI package
    find_package(MPI REQUIRED)
    #Find threads package
    find_package (Threads REQUIRED)    
endif(WITH_MPI)


# Create lib
add_library(rupper STATIC ${PROJECT_SOURCE_DIR}/src/ruperLB.cpp)

target_include_directories(rupper PUBLIC ${PROJECT_SOURCE_DIR}/src/)

if(WITH_MPI)
    target_include_directories(rupper PUBLIC ${MPI_INCLUDE_PATH})
    target_link_libraries(rupper ${MPI_LIBRARIES})
    #Link thread libs
    target_link_libraries (rupper ${CMAKE_THREAD_LIBS_INIT})
endif(WITH_MPI)
