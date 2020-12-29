
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

# Check if MPI option has been enabled
if(WITH_MPI)
  add_definitions(-D_USE_MPI_=1) #enable MPI
endif(WITH_MPI)

#Find threads package
find_package (Threads REQUIRED)    

add_executable(pi ${PROJECT_SOURCE_DIR}/examples/1/pi.cpp)

#target libs
target_link_libraries(pi rupper)
target_link_libraries(pi ${CMAKE_THREAD_LIBS_INIT})

