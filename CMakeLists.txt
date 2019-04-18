cmake_minimum_required(VERSION 3.12)
project(Suspiria VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

set(LIBRARY_SOURCE_FILES "")
add_library(suspiria ${LIBRARY_SOURCE_FILES}
        src/networking/http.cpp
        src/networking/graph_router.cpp
        src/networking/utility.cpp
        src/networking/http_parser.c)
target_include_directories(suspiria
        PUBLIC
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        "lib"
)

if (UNIX)
    target_link_libraries(suspiria pthread)
endif ()


add_executable(suspiria_server server.cpp)
target_link_libraries(suspiria_server suspiria)

# Testing
include(CTest)
if (BUILD_TESTING)
    add_subdirectory(tests)
    enable_testing()
    add_test(suspiria_tests tests/suspiria_tests)
endif ()