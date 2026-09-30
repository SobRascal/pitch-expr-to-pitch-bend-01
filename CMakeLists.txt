cmake_minimum_required(VERSION 3.21)

project(
    PitchExprToPitchBend01
    VERSION 0.3.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CLAP_BUILD_TESTS OFF CACHE BOOL "Build CLAP tests" FORCE)
set(CLAP_HELPERS_BUILD_TESTS OFF CACHE BOOL "Build clap-helpers tests" FORCE)
set(CLAP_HELPERS_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "Download clap-helpers dependencies" FORCE)

include(FetchContent)

FetchContent_Declare(
    clap
    GIT_REPOSITORY https://github.com/free-audio/clap.git
    GIT_TAG a47f6badb49d948fd009998f28309cdab78979c9
    GIT_SHALLOW FALSE
)

FetchContent_Declare(
    clap_helpers
    GIT_REPOSITORY https://github.com/free-audio/clap-helpers.git
    GIT_TAG 629ae7526740e3d0ee4d64c230df183ecdb2ff5a
    GIT_SHALLOW FALSE
)

FetchContent_MakeAvailable(clap clap_helpers)

set(PITCH_EXPR_TO_PITCH_BEND_SOURCES
    src/plugin.cpp
    src/translator.cpp
)

if(WIN32)
    list(APPEND PITCH_EXPR_TO_PITCH_BEND_SOURCES
        src/information_gui_win32.cpp
    )
else()
    list(APPEND PITCH_EXPR_TO_PITCH_BEND_SOURCES
        src/information_gui_stub.cpp
    )
endif()

add_library(PitchExprToPitchBend01 MODULE
    ${PITCH_EXPR_TO_PITCH_BEND_SOURCES}
)

target_include_directories(PitchExprToPitchBend01 PRIVATE include)

target_link_libraries(PitchExprToPitchBend01
    PRIVATE
        clap
        clap-helpers
)

if(WIN32)
    target_link_libraries(PitchExprToPitchBend01 PRIVATE user32 gdi32)
endif()

target_include_directories(PitchExprToPitchBend01 SYSTEM PRIVATE
    "${clap_SOURCE_DIR}/include"
    "${clap_helpers_SOURCE_DIR}/include"
)

target_compile_definitions(PitchExprToPitchBend01
    PRIVATE
        $<$<CONFIG:Debug>:PITCH_EXPR_DEBUG=1>
)

if(MSVC)
    target_compile_options(PitchExprToPitchBend01 PRIVATE /W4 /permissive-)
else()
    target_compile_options(PitchExprToPitchBend01 PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
endif()

set_target_properties(PitchExprToPitchBend01 PROPERTIES
    PREFIX ""
    SUFFIX ".clap"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

include(CTest)

if(BUILD_TESTING)
    add_executable(PitchExprToPitchBend01Tests
        tests/translator_tests.cpp
        src/translator.cpp
    )

    target_include_directories(PitchExprToPitchBend01Tests PRIVATE include)
    target_link_libraries(PitchExprToPitchBend01Tests PRIVATE clap)

    if(MSVC)
        target_compile_options(PitchExprToPitchBend01Tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(PitchExprToPitchBend01Tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
    endif()

    set_target_properties(PitchExprToPitchBend01Tests PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )

    add_test(NAME translator COMMAND PitchExprToPitchBend01Tests)

    if(WIN32)
        add_executable(PitchExprToPitchBend01GuiTests
            tests/information_gui_tests.cpp
            src/information_gui_win32.cpp
        )

        target_include_directories(PitchExprToPitchBend01GuiTests PRIVATE include)
        target_link_libraries(PitchExprToPitchBend01GuiTests
            PRIVATE
                clap
                user32
                gdi32
        )

        if(MSVC)
            target_compile_options(PitchExprToPitchBend01GuiTests PRIVATE /W4 /permissive-)
        else()
            target_compile_options(PitchExprToPitchBend01GuiTests PRIVATE
                -Wall -Wextra -Wpedantic -Wconversion -Wshadow
            )
        endif()

        set_target_properties(PitchExprToPitchBend01GuiTests PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        )

        add_test(NAME information_gui COMMAND PitchExprToPitchBend01GuiTests)
    endif()
endif()
