cmake_minimum_required(VERSION 3.12)
project(Suspiria VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

set(LIBRARY_SOURCE_FILES "lib/mongoose/mongoose.c")
add_library(suspiria ${LIBRARY_SOURCE_FILES}
        src/networking/server/http.cpp
        src/networking/server/router.cpp
        src/networking/utility.cpp
        )
target_include_directories(suspiria
        PUBLIC
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        "lib"
)

add_executable(suspiria_server server.cpp)
target_link_libraries(suspiria_server suspiria)