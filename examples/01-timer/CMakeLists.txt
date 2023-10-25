set(EXAMPLE_NAME 01-timer)

# create an executable
add_executable (${EXAMPLE_NAME} main.cpp)

# and link the library against the executable
target_link_libraries (${EXAMPLE_NAME} ${SAI2-COMMON_EXAMPLES_LIBRARIES})