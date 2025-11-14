project(CMDLineTextEditorTest LANGUAGES CXX)

add_executable(CMDLineTextEditorTest)

target_sources(CMDLineTextEditorTest PRIVATE
        "../src/Application.cpp"
        "../src/Command.cpp"
        "../src/Editor.cpp"
        "../src/Workspace.cpp"
        "../src/TreeDrawer.cpp"
        "../src/Logging.cpp"
        "test.cpp"
)

target_include_directories(CMDLineTextEditorTest PRIVATE
        "../src"
        "../vendors/nlohmann-json/single_include"
)

if (MSVC)
    target_compile_options(CMDLineTextEditorTest PRIVATE /utf-8)
endif()

add_custom_command(
        TARGET CMDLineTextEditorTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/testfile
        $<TARGET_FILE_DIR:CMDLineTextEditorTest>/testfile
)