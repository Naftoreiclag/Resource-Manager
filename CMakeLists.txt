#   Copyright 2017 James Fong
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

### Basic CMake project and build information ###

set(PGLOCAL_HUMAN_NAME "Naftoreiclag's Resouce Manager")
set(PGLOCAL_PROJ_NAME "resman")

# CMake minimum version
# 3.7.2 is the first version that FindVulkan is supported
cmake_minimum_required(VERSION 3.7.2)

# Project name/version
project(RESMAN VERSION 0.1.0)
message(STATUS "${PGLOCAL_HUMAN_NAME} version ${RESMAN_VERSION}")

# Where to find modules
list(APPEND CMAKE_MODULE_PATH "${RESMAN_SOURCE_DIR}/cmake")

### Variables starting with "PGLOCAL_" are not CMake built-ins

# Where the .cpp files reside
set(PGLOCAL_SOURCE_DIR "${RESMAN_SOURCE_DIR}/src")

# Where the header files reside
set(PGLOCAL_INCLUDE_DIRS ${PGLOCAL_SOURCE_DIR})

# Name of target executable
set(PGLOCAL_MAIN_TARGET "${PGLOCAL_PROJ_NAME}")

### User options ###

### Build target configuration ###

include("MainSrcList")
set(PGLOCAL_MAIN_SOURCES_LIST ${PGLOCAL_SOURCES_LIST})
set(PGLOCAL_SOURCES_LIST "")

set(PGLOCAL_ALL_REQUIRED_READY TRUE)
message(STATUS "Package status: ")

## Header-Only / Compiled Third-party ##

# EasyLog++ #
message(STATUS "EasyLog++ ============")
find_package(EasyLoggingPP)
if(EASYLOGPP_FOUND)
    message(STATUS "\tInclude Dirs: " ${EASYLOGPP_INCLUDE_DIRS})
    message(STATUS "\tSource Files: " ${EASYLOGPP_SOURCE_FILES})
    list(APPEND PGLOCAL_INCLUDE_DIRS ${EASYLOGPP_INCLUDE_DIRS})
    list(APPEND PGLOCAL_MAIN_SOURCES_LIST ${EASYLOGPP_SOURCE_FILES})
else()
    message("\tNOT FOUND")
    set(PGLOCAL_ALL_REQUIRED_READY FALSE)
endif()

# JsonCpp #
message(STATUS "JsonCpp ==============")
find_package(JsonCpp)
if(JSONCPP_FOUND)
    message(STATUS "\tInclude Dirs: " ${JSONCPP_INCLUDE_DIRS})
    message(STATUS "\tSource Files: " ${JSONCPP_SOURCE_FILES})
    list(APPEND PGLOCAL_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIRS})
    list(APPEND PGLOCAL_MAIN_SOURCES_LIST ${JSONCPP_SOURCE_FILES})
else()
    message("\tNOT FOUND")
    set(PGLOCAL_ALL_REQUIRED_READY FALSE)
endif()

# STB #
message(STATUS "STB ==================")
find_package(STB)
if(STB_FOUND)
    message(STATUS "\tInclude Dirs: " ${STB_INCLUDE_DIRS})
    list(APPEND PGLOCAL_INCLUDE_DIRS ${STB_INCLUDE_DIRS})
else()
    message("\tNOT FOUND")
    set(PGLOCAL_ALL_REQUIRED_READY FALSE)
endif()

# Add build targets
add_executable(${PGLOCAL_MAIN_TARGET} ${PGLOCAL_MAIN_SOURCES_LIST})

# Add required features
set_property(TARGET ${PGLOCAL_MAIN_TARGET} PROPERTY CXX_STANDARD 14)

## Linked Third-party ##

# Assimp #
message(STATUS "Assimp ===============")
find_package(Assimp)
if(ASSIMP_FOUND)
    message(STATUS "\tInclude Dirs: " ${ASSIMP_INCLUDE_DIRS})
    message(STATUS "\tLibraries: " ${ASSIMP_LIBRARIES})
    list(APPEND PGLOCAL_INCLUDE_DIRS ${ASSIMP_INCLUDE_DIRS})
    target_link_libraries(${PGLOCAL_MAIN_TARGET} ${ASSIMP_LIBRARIES})
else()
    message("\tNOT FOUND")
    set(PGLOCAL_ALL_REQUIRED_READY FALSE)
endif()

# Boost #
message(STATUS "Boost Filesystem =====")
find_package(Boost 1.45.0 COMPONENTS system filesystem)
if(Boost_FOUND)
    message(STATUS "\tInclude Dirs: " ${Boost_INCLUDE_DIRS})
    message(STATUS "\tLibraries: " ${Boost_LIBRARIES})
    list(APPEND PGLOCAL_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
    target_link_libraries(${PGLOCAL_MAIN_TARGET} ${Boost_LIBRARIES})
else()
    message("\tNOT FOUND")
    set(PGLOCAL_ALL_REQUIRED_READY FALSE)
endif()

# OGG #
message(STATUS "OGG ==================")
find_package(OGG)
if(OGG_FOUND)
    message(STATUS "\tInclude Dirs: " ${OGG_INCLUDE_DIRS})
    message(STATUS "\tLibraries: " ${OGG_LIBRARIES})
    list(APPEND PGLOCAL_INCLUDE_DIRS ${OGG_INCLUDE_DIRS})
    target_link_libraries(${PGLOCAL_MAIN_TARGET} ${OGG_LIBRARIES})
else()
    message("\tNOT FOUND")
    set(PGLOCAL_ALL_REQUIRED_READY FALSE)
endif()

# VORBIS #
message(STATUS "VORBIS ===============")
find_package(VORBIS)
if(VORBIS_FOUND)
    message(STATUS "\tInclude Dirs: " ${VORBIS_INCLUDE_DIRS})
    message(STATUS "\tLibraries: " ${VORBIS_LIBRARIES})
    list(APPEND PGLOCAL_INCLUDE_DIRS ${VORBIS_INCLUDE_DIRS})
    target_link_libraries(${PGLOCAL_MAIN_TARGET} ${VORBIS_LIBRARIES})
else()
    message("\tNOT FOUND")
    set(PGLOCAL_ALL_REQUIRED_READY FALSE)
endif()

# FLAC #
message(STATUS "FLAC =================")
find_package(FLAC)
if(FLAC_FOUND)
    message(STATUS "\tInclude Dirs: " ${FLAC_INCLUDE_DIRS})
    message(STATUS "\tLibraries: " ${FLAC_LIBRARIES})
    list(APPEND PGLOCAL_INCLUDE_DIRS ${FLAC_INCLUDE_DIRS})
    target_link_libraries(${PGLOCAL_MAIN_TARGET} ${FLAC_LIBRARIES})
else()
    message("\tNOT FOUND")
    set(PGLOCAL_ALL_REQUIRED_READY FALSE)
endif()


# Helpful information
if(PGLOCAL_ALL_REQUIRED_READY)
    message(STATUS "All packages found and are compatible")
else()
    message("One or more packages are missing, build will fail!")
endif()

# Setup include directories
include_directories(${PGLOCAL_INCLUDE_DIRS})
