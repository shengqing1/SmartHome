cmake_minimum_required(VERSION 3.18)
project(smartHome)

set(CMAKE_CXX_STANDARD 14)
find_path(WIRINGPI_INCLUDE_DIR wiringPi.h)
find_library(WIRINGPI_LIBRARY NAMES wiringPi)
find_package(Threads REQUIRED)

include_directories(${WIRINGPI_INCLUDE_DIR})
link_directories(${WIRINGPI_LIBRARY_DIRS})

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
# Print some message showing some of them
message(STATUS "OpenCV library status:")
message(STATUS "    config: ${OpenCV_DIR}")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

add_executable(smartHome main.cpp mqtt.h mqtt.cpp main.h main.h)
target_link_libraries(smartHome ${OpenCV_LIBS} ${WIRINGPI_LIBRARY} Threads::Threads)