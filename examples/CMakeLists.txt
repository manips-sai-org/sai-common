# for ubuntu 18.04, seems necessary to explicitely find and link against pthread
FIND_PACKAGE(Threads REQUIRED)

SET(SAI2-COMMON_EXAMPLES_LIBRARIES ${SAI2-COMMON_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

ADD_SUBDIRECTORY(01-timer)
ADD_SUBDIRECTORY(02-filters)
ADD_SUBDIRECTORY(03-logger)
ADD_SUBDIRECTORY(04-redis_communication)
ADD_SUBDIRECTORY(05-timer_overtime_monitoring)
