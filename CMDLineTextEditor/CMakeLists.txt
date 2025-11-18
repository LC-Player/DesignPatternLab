project(CMDLineTextEditor LANGUAGES CXX)

add_executable(CMDLineTextEditor)

target_sources(CMDLineTextEditor PRIVATE
    "src/Application.cpp"
    "src/Command.cpp"
    "src/Components/Editor.cpp"
    "src/main.cpp"
    "src/Components/Workspace.cpp"
    "src/TreeDrawer.cpp"
    "src/Components/Logging.cpp"
)

target_include_directories(CMDLineTextEditor PRIVATE
    "src"
    "src/Components"
    "vendors/nlohmann-json/single_include"
)

if (CMAKE_BUILT_TYPE STREQUAL "Debug")
    target_compile_definitions(CMDLineTextEditor PRIVATE _DEBUG)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(CMDLineTextEditor PRIVATE _RELEASE)
else ()
endif ()


if (MSVC)
    target_compile_options(CMDLineTextEditor PRIVATE /utf-8)
endif()