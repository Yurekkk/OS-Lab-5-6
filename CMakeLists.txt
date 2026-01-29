cmake_minimum_required(VERSION 3.10)

project(main)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    libs
    libs/imgui
    libs/imgui/backends
    libs/implot
)

add_library(sqlite3 libs/sqlite3.c)

add_executable(logger logger.cpp)
add_executable(emulator emulator.c)

target_link_libraries(emulator m)
target_link_libraries(logger 
    sqlite3
    pthread
    dl
)

if(WIN32)
    # Требуем Windows 7+ API и линкуем Winsock2 для httplib
    add_definitions(-D_WIN32_WINNT=0x0A00)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    target_link_libraries(logger ws2_32)
endif()



# GUI

add_executable(gui gui/gui.cpp)
find_package(OpenGL REQUIRED)

# Поиск и линковка GLFW (кроссплатформенное окно + ввод)
if(WIN32)
    find_library(GLFW3_LIB glfw3
        PATHS "${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw/build/src"
        NO_DEFAULT_PATH
    )
    target_link_libraries(gui ${GLFW3_LIB} OpenGL::GL)
endif()

if(UNIX AND NOT APPLE)
    find_package(glfw3 REQUIRED)
    target_link_libraries(gui glfw OpenGL::GL)
endif()

# Добавляем исходники ImGui
target_sources(gui PRIVATE
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

# Добавляем исходники ImPlot (графики и визуализация временных рядов)
target_sources(gui PRIVATE
    libs/implot/implot.cpp
    libs/implot/implot_items.cpp
)

# Линковка системных библиотек
if(WIN32)
    target_link_libraries(gui 
        ws2_32
        gdi32 
        opengl32 
        winmm
    )
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(gui 
        X11
        dl
        pthread
        rt
    )
endif()