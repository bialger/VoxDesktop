cmake_minimum_required(VERSION 3.25)

project(
    VoxDesktop
    VERSION 0.1.0
    DESCRIPTION "Vox secure desktop messenger client"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
option(VOX_ENABLE_CONTRACT_TESTS "Build real backend contract tests" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    Sodium
    GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    GIT_SUBMODULES_RECURSE TRUE
)
set(SODIUM_DISABLE_TESTS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Sodium)

add_subdirectory(lib)

if(DEFINED ENV{QTDIR} AND NOT DEFINED Qt6_DIR)
    set(Qt6_DIR "$ENV{QTDIR}/lib/cmake/Qt6" CACHE PATH "Path to Qt6Config.cmake")
endif()

if(NOT TARGET Qt6::Core)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Sql Concurrent Test WebSockets)
endif()

find_package(Qt6 REQUIRED COMPONENTS Network Sql Concurrent Test WebSockets)
find_package(Qt6 QUIET COMPONENTS Multimedia Svg)

qt_standard_project_setup()

add_subdirectory(src)
add_subdirectory(bin)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# On Windows with MinGW + VS installed, Clang/clangd may pick up MSVC headers by default.
# Patch compile_commands.json so clangd/clang-tidy use MinGW libstdc++ instead.
# Runs at build time; compile_commands.json exists after cmake generate.
# Set MINGW_TOOLCHAIN env var if your MinGW is elsewhere (default: C:/Program Files/mingw64).
if (WIN32)
    set(_MINGW "C:/Program Files/mingw64")
    if (DEFINED ENV{MINGW_TOOLCHAIN})
        set(_MINGW "$ENV{MINGW_TOOLCHAIN}")
    endif ()
    # Use short path (C:/PROGRA~1/mingw64) to avoid CMake/shell quoting issues with spaces
    string(REPLACE "Program Files" "PROGRA~1" _MINGW_SHORT "${_MINGW}")
    # Force MinGW libstdc++ for clangd (avoids MSVC headers when VS is installed)
    set(_PATCH_FLAGS " --driver-mode=gcc --target=x86_64-w64-windows-gnu --gcc-toolchain=${_MINGW_SHORT} -std=gnu++23 -stdlib=libstdc++ ")
    add_custom_target(patch-compile-commands-for-clang ALL
        COMMAND ${CMAKE_COMMAND}
            -DCC_FILE="${CMAKE_BINARY_DIR}/compile_commands.json"
            -DFLAGS="${_PATCH_FLAGS}"
            -DSOURCE_DIR="${CMAKE_SOURCE_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PatchCompileCommandsForClang.cmake"
        COMMENT "Patching compile_commands.json for clangd/clang-tidy (MinGW toolchain)"
        )
    if (TARGET vox_core)
        add_dependencies(vox_core patch-compile-commands-for-clang)
    endif ()
    unset(_MINGW)
    unset(_PATCH_FLAGS)
endif ()
