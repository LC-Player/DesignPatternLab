cmake_minimum_required (VERSION 3.20)
project (DesignPatternTextEditorLab LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ARCH x64)
if(WIN32)
    set(PLATFORM "Windows")
elseif(APPLE)
    set(PLATFORM "macOS")
else()
    set(PLATFORM "Linux")
endif()

add_subdirectory("CMDLineTextEditor")
add_subdirectory("CMDLineTextEditor/tests")