# cmake --verison
cmake_minimum_required(VERSION 3.5)

# Set the project name
project(mec_adapter)

# Create a sources variable with a link to all cpp files to compile
set(
    SOURCES
        ${PROJECT_SOURCE_DIR}/logic/mec-adapter/src/MECAdapter.cpp
        ${PROJECT_SOURCE_DIR}/logic/mec-adapter/src/libemqtt.cpp
        ${PROJECT_SOURCE_DIR}/logic/mec-adapter/src/MECAdapter_conf.cpp
)

# Specify executable program output path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/output)



# In order to use std::thread
set(THREADS_PREFER_PTHREAD_FLAG ON)

add_compile_options(-std=c++11 -Wall -lpthread)


# Build project
add_executable(${PROJECT_NAME} ${SOURCES})

# Add thread Library
find_package(Threads REQUIRED)

# Link header files
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/logic/mec-adapter/include
        ${CMAKE_SOURCE_DIR}/../include/rapidjson
        ${CMAKE_SOURCE_DIR}/../include/libemqtt.h
        ${CMAKE_SOURCE_DIR}/../include/MECAdapter.h

)
target_link_libraries(${PROJECT_NAME} )

# Link dynamic library files
target_link_libraries(${PROJECT_NAME}

    Threads::Threads
)



