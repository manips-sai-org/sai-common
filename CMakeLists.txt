CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT (SAI2-COMMON)
SET (PROJECT_VERSION 0.1.0) #major version is zero for now since api will certainly change in future

SET(CMAKE_CXX_FLAGS "-std=c++11 -I/usr/include -I/usr/local/include -fPIC")
if (${CMAKE_SYSTEM_NAME} MATCHES Darwin)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/homebrew/include")
endif ()

# set default build to release with debug info. mimic'ed from RBDL project.
IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	MESSAGE(STATUS "Setting build type to 'Release' as none was specified.")
	# Set the default build, and also the possible values of build type for cmake-gui
	SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
	SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
ENDIF()

# include Eigen
FIND_PACKAGE(Eigen3 REQUIRED)

# jsoncpp
if (CMAKE_SYSTEM_NAME MATCHES Darwin) 
	find_library(JSONCPP_LIBRARY jsoncpp)
elseif (CMAKE_SYSTEM_NAME MATCHES Linux)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(JSONCPP jsoncpp)
	link_directories(${JSONCPP_LIBRARY_DIRS})
	set(JSONCPP_LIBRARY ${JSONCPP_LIBRARIES})
endif ()

# hiredis
find_library(HIREDIS_LIBRARY hiredis)

# set include directories
SET (SAI2-COMMON_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src)

# include Redis
SET(REDIS_SOURCE ${PROJECT_SOURCE_DIR}/src/redis/RedisClient.cpp)

# include Timer
SET(TIMER_SOURCE ${PROJECT_SOURCE_DIR}/src/timer/LoopTimer.cpp)

# include filter
SET(FILTER_SOURCE ${PROJECT_SOURCE_DIR}/src/filters/ButterworthFilter.cpp)

#include logger
SET(LOGGER_SOURCE ${PROJECT_SOURCE_DIR}/src/logger/Logger.cpp)

# Add the include directory to the include paths
INCLUDE_DIRECTORIES (
	${PROJECT_SOURCE_DIR}/src
	${EIGEN3_INCLUDE_DIR}
	${JSONCPP_INCLUDE_DIRS}
	)

# Create the library. 
ADD_LIBRARY (sai2-common STATIC
	${REDIS_SOURCE}
	${TIMER_SOURCE}
	${FILTER_SOURCE}
	${LOGGER_SOURCE}
	)

SET (SAI2-COMMON_LIBRARIES sai2-common 
	${JSONCPP_LIBRARY}
	${HIREDIS_LIBRARY}
	)

# 
# export package
# 

# export package for use from the build tree
EXPORT (TARGETS sai2-common FILE ${PROJECT_BINARY_DIR}/SAI2-COMMONTargets.cmake)
EXPORT (PACKAGE SAI2-COMMON)

# configure package
SET (CONF_INCLUDE_DIRS ${SAI2-COMMON_INCLUDE_DIRS} ${JSONCPP_INCLUDE_DIRS})
SET (CONF_LIBRARIES ${SAI2-COMMON_LIBRARIES})

# create package configuration and version files
CONFIGURE_FILE(SAI2-COMMONConfig.cmake.in "${PROJECT_BINARY_DIR}/SAI2-COMMONConfig.cmake" @ONLY)
INCLUDE(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SAI2-COMMONConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

# add examples
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/examples)
