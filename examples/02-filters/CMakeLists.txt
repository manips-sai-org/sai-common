set(EXAMPLE_NAME 02-filters)

# create an executable
add_executable (${EXAMPLE_NAME} main.cpp)

# and link the library against the executable
target_link_libraries (${EXAMPLE_NAME} ${SAI-COMMON_EXAMPLES_LIBRARIES})

# export resources such as model files.
FILE(COPY plot_signals.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})