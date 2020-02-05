#   Copyright 2018 Princeton University
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

cmake_minimum_required(VERSION 2.6)
project(pyr)
find_package(PkgConfig)

#set(CMAKE_BUILD_TYPE "Debug")
#set(CMAKE_C_FLAGS "-O0 -g")

if(APPLE)
	set(CMAKE_OSX_ARCHITECTURES i386 x86_64)
endif()
##set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -g")

pkg_check_modules(LIBNL3 libnl-genl-3.0)

message(STATUS "found :${LIBNL3_LIBRARIES} with flags ${LIBNL3_CFLAGS}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g ${LIBNL3_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g ${LIBNL3_CFLAGS}")
#set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(pyronia_opt SHARED stack_log_wrappers.c)
add_library(pyronia SHARED memdom_avl_tree.c policy_avl_tree.c security_context.c serialization.c si_comm.c pyronia_lib.c benchmarking_util.c)

target_link_libraries(pyronia ${LIBNL3_LIBRARIES} smv pthread crypto ssl)
target_link_libraries(pyronia_opt pyronia dl)
