cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(testunit LANGUAGES CXX)

include(CheckLanguage)

include(FetchContent)

include_directories(src)

file(GLOB_RECURSE source_file LIST_DIRECTORIES false src/*.cpp src/*.c)

add_executable(${CMAKE_PROJECT_NAME} ${source_file})

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

#Google Unit Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG master
)
FetchContent_MakeAvailable(googletest)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE gtest_main)
target_compile_features(${CMAKE_PROJECT_NAME} PUBLIC cxx_std_11)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Threads::Threads)
include(GoogleTest)
gtest_discover_tests(${CMAKE_PROJECT_NAME})
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Build type, valid value is Release or Debug" FORCE)
