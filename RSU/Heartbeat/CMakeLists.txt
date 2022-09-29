cmake_minimum_required(VERSION 3.10)

project(Heartbeat VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#引用头文件
include_directories(include/ 
							)

set(SOURCES_FILES src/Heartbeat.cpp 
                  src/Heartbeat_conf.cpp
                  src/main.cpp
                  src/libemqtt.cpp)

#生成可执行文件
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/output/)
add_executable(${PROJECT_NAME} ${SOURCES_FILES})


