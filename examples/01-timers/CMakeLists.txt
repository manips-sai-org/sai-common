set(EXAMPLE_NAME 01-timers)

# create an executable
add_executable (${EXAMPLE_NAME} main.cpp)

# and link the library against the executable
target_link_libraries (${EXAMPLE_NAME} ${SAI2-COMMON_EXAMPLES_LIBRARIES})

# export resources such as model files.
set(RESOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/resources)
file(MAKE_DIRECTORY ${RESOURCE_DIR})
FILE(COPY world.urdf rrbot.urdf DESTINATION ${RESOURCE_DIR})