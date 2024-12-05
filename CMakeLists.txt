cmake_minimum_required(VERSION 3.10)

project(SAI2-COMMON)
set(PROJECT_VERSION 0.1.0) # major version is zero for now since api will
                           # certainly change in future

option(BUILD_EXAMPLES "Build examples" ON)

set(CMAKE_CXX_FLAGS "-std=c++17 -I/usr/include -I/usr/local/include -fPIC")
if(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/homebrew/include")
endif()

# set default build to release with debug info. mimic'ed from RBDL project.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  # Set the default build, and also the possible values of build type for
  # cmake-gui
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "MinSizeRel" "RelWithDebInfo")
endif()

# include Eigen
find_package(Eigen3 REQUIRED)

# jsoncpp
if(CMAKE_SYSTEM_NAME MATCHES Darwin)
  find_library(JSONCPP_LIBRARY jsoncpp)
elseif(CMAKE_SYSTEM_NAME MATCHES Linux)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(JSONCPP jsoncpp)
  link_directories(${JSONCPP_LIBRARY_DIRS})
  set(JSONCPP_LIBRARY ${JSONCPP_LIBRARIES})
endif()

# hiredis
find_library(HIREDIS_LIBRARY hiredis)
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h REQUIRED)

# include Redis
set(REDIS_SOURCE ${PROJECT_SOURCE_DIR}/src/redis/RedisClient.cpp)

# include Timer
set(TIMER_SOURCE ${PROJECT_SOURCE_DIR}/src/timer/LoopTimer.cpp)

# include filter
set(FILTER_SOURCE ${PROJECT_SOURCE_DIR}/src/filters/ButterworthFilter.cpp)

# include logger
set(LOGGER_SOURCE ${PROJECT_SOURCE_DIR}/src/logger/Logger.cpp)

# Add the include directory to the include paths
include_directories(${PROJECT_SOURCE_DIR}/src ${EIGEN3_INCLUDE_DIR}
                    ${JSONCPP_INCLUDE_DIRS})

# Create the library.
add_library(sai2-common STATIC ${REDIS_SOURCE} ${TIMER_SOURCE} ${FILTER_SOURCE}
                               ${LOGGER_SOURCE})

set(SAI2-COMMON_LIBRARIES sai2-common ${JSONCPP_LIBRARY} ${HIREDIS_LIBRARY})

#
# export package
#

# set include directories
set(SAI2-COMMON_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src)

# export package for use from the build tree
export(TARGETS sai2-common FILE ${PROJECT_BINARY_DIR}/SAI2-COMMONTargets.cmake)
export(PACKAGE SAI2-COMMON)

# configure package
set(CONF_INCLUDE_DIRS ${SAI2-COMMON_INCLUDE_DIRS} ${JSONCPP_INCLUDE_DIRS} ${HIREDIS_INCLUDE_DIR})
set(CONF_LIBRARIES ${SAI2-COMMON_LIBRARIES})

# create package configuration and version files
configure_file(SAI2-COMMONConfig.cmake.in
               "${PROJECT_BINARY_DIR}/SAI2-COMMONConfig.cmake" @ONLY)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SAI2-COMMONConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

# add examples
if(BUILD_EXAMPLES)
  add_subdirectory(${PROJECT_SOURCE_DIR}/examples)
endif()
