cmake_minimum_required(VERSION 3.12)
project(MessageCrypt VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

add_executable(message_crypt main.cpp lib/mongoose/mongoose.c)
target_include_directories(message_crypt PRIVATE "lib")