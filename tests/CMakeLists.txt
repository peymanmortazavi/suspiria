cmake_minimum_required(VERSION 3.12)
project(SuspiriaTests)

find_package(GTest REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(TEST_SOURCES utility/test_utility.cpp)

add_executable(suspiria_tests ${TEST_SOURCES})
target_link_libraries(suspiria_tests suspiria Threads::Threads ${GTEST_BOTH_LIBRARIES})