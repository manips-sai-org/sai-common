set(EXAMPLE_NAME 06-zmq_communication)

# create an executable
add_executable (${EXAMPLE_NAME} main.cpp)
add_executable (zmq_client client.cpp)

# and link the library against the executable
target_link_libraries (${EXAMPLE_NAME} ${SAI-COMMON_EXAMPLES_LIBRARIES})
target_link_libraries (zmq_client ${SAI-COMMON_EXAMPLES_LIBRARIES})