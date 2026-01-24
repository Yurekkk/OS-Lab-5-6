cmake_minimum_required(VERSION 3.10)

project(main)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(libs)

add_library(sqlite3 libs/sqlite3.c)

add_executable(logger logger.cpp)
add_executable(emulator emulator.c)

target_link_libraries(emulator m)
target_link_libraries(logger sqlite3)

if(WIN32)
    # Требуем Windows 7+ API и линкуем Winsock2 для httplib
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    target_link_libraries(logger ws2_32)
endif()
