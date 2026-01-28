cmake_minimum_required(VERSION 3.10)

project(main)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    libs
    libs/imgui
    libs/imgui/backends
    libs/implot
    libs/glfw/include
    libs/glfw/build/src
)

add_library(sqlite3 libs/sqlite3.c)
find_package(OpenGL REQUIRED)

add_executable(logger logger.cpp)
add_executable(emulator emulator.c)

target_link_libraries(emulator m)
target_link_libraries(logger sqlite3)

if(WIN32)
    # Требуем Windows 7+ API и линкуем Winsock2 для httplib
    add_definitions(-D_WIN32_WINNT=0x0A00)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    target_link_libraries(logger ws2_32)
endif()



# GUI

set(IMGUI_SOURCES
    libs/imgui/imgui.cpp
    libs/imgui/imgui_demo.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
    libs/implot/implot.cpp
    libs/implot/implot_items.cpp
)

add_executable(gui gui.cpp ${IMGUI_SOURCES})

find_library(GLFW3_LIB glfw3
    PATHS "D:\\OS\\Lab-5-6\\libs\\glfw\\build\\src"
    NO_DEFAULT_PATH
)

# Добавляем исходники ImGui
target_sources(gui PRIVATE
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)

# Добавляем исходники ImPlot
target_sources(gui PRIVATE
    libs/implot/implot.cpp
    libs/implot/implot_items.cpp
)

target_link_libraries(gui
    ${GLFW3_LIB}
    OpenGL::GL
    gdi32 
    opengl32 
    winmm
)

if(WIN32)
    target_link_libraries(gui ws2_32)
endif()