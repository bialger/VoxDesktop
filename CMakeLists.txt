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
