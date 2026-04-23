if(WIN32)
    set(PATH_DELIMITER ";")
else()
    set(PATH_DELIMITER ":")
endif()

set(VOX_QT_VERSION "6.8.2" CACHE STRING "Bundled Qt version")
option(VOX_QT_BUILD_SVG "Build bundled QtSvg module when missing" ON)
option(VOX_QT_BUILD_MULTIMEDIA "Build bundled QtMultimedia module when missing" OFF)

set(QT_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/qt-build")
set(QT_INCLUDE_DIR "${QT_BUILD_DIR}/include")
set(QT_BIN_DIR "${QT_BUILD_DIR}/bin")
set(Qt6_DIR "${QT_BUILD_DIR}/lib/cmake/Qt6" CACHE PATH "Path to bundled Qt6 package config" FORCE)
set(ENV{QTDIR} "${QT_BUILD_DIR}")
set(ENV{PATH} "${QT_BIN_DIR}${PATH_DELIMITER}$ENV{PATH}")

set(VOX_QT_REQUIRED_COMPONENTS
    Core
    Gui
    Widgets
    Network
    Sql
    Concurrent
    Test
    WebSockets
)

function(vox_download_and_extract archive_url archive_path source_dir)
    if(NOT EXISTS "${archive_path}")
        message(STATUS "Downloading Qt source archive: ${archive_url}")
        file(DOWNLOAD "${archive_url}" "${archive_path}" SHOW_PROGRESS)
    endif()

    if(NOT EXISTS "${source_dir}/CMakeLists.txt")
        message(STATUS "Extracting: ${archive_path}")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E tar xvf "${archive_path}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            RESULT_VARIABLE extract_result
            OUTPUT_QUIET
        )
        if(NOT extract_result EQUAL 0)
            message(FATAL_ERROR "Failed to extract Qt archive: ${archive_path}")
        endif()
    endif()
endfunction()

function(vox_build_qtbase)
    if(EXISTS "${Qt6_DIR}/Qt6Config.cmake")
        return()
    endif()

    set(qtbase_archive "qtbase-${VOX_QT_VERSION}.tar.gz")
    set(qtbase_archive_path "${CMAKE_CURRENT_BINARY_DIR}/${qtbase_archive}")
    set(qtbase_url "https://github.com/qt/qtbase/archive/refs/tags/v${VOX_QT_VERSION}.tar.gz")
    set(qtbase_source_dir "${CMAKE_CURRENT_BINARY_DIR}/qtbase-${VOX_QT_VERSION}")
    vox_download_and_extract("${qtbase_url}" "${qtbase_archive_path}" "${qtbase_source_dir}")

    file(MAKE_DIRECTORY "${QT_BUILD_DIR}")
    set(qt_configure_options
        -static
        -c++std c++20
        -static-runtime
        -release
        -nomake examples
        -nomake tests
        -prefix "${QT_BUILD_DIR}"
        -sql-sqlite
    )

    message(STATUS "Configuring Qt base at ${QT_BUILD_DIR}")
    if(WIN32)
        execute_process(
            COMMAND "${qtbase_source_dir}/configure.bat" ${qt_configure_options}
            WORKING_DIRECTORY "${QT_BUILD_DIR}"
            RESULT_VARIABLE configure_result
        )
    else()
        execute_process(
            COMMAND "${qtbase_source_dir}/configure" ${qt_configure_options}
            WORKING_DIRECTORY "${QT_BUILD_DIR}"
            RESULT_VARIABLE configure_result
        )
    endif()

    if(NOT configure_result EQUAL 0)
        message(FATAL_ERROR "Qt base configure failed with code ${configure_result}")
    endif()

    message(STATUS "Building Qt base")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build . --parallel
        WORKING_DIRECTORY "${QT_BUILD_DIR}"
        RESULT_VARIABLE build_result
    )
    if(NOT build_result EQUAL 0)
        message(FATAL_ERROR "Qt base build failed with code ${build_result}")
    endif()
endfunction()

function(vox_build_qt_module module_repo)
    string(REGEX REPLACE "^([0-9]+\\.[0-9]+)\\..*$" "\\1" qt_minor_version "${VOX_QT_VERSION}")
    set(module_archive "${module_repo}-everywhere-src-${VOX_QT_VERSION}.tar.xz")
    set(module_archive_path "${CMAKE_CURRENT_BINARY_DIR}/${module_archive}")
    set(module_source_dir "${CMAKE_CURRENT_BINARY_DIR}/${module_repo}-everywhere-src-${VOX_QT_VERSION}")
    set(module_url "https://download.qt.io/official_releases/qt/${qt_minor_version}/${VOX_QT_VERSION}/submodules/${module_archive}")
    set(module_build_dir "${CMAKE_CURRENT_BINARY_DIR}/${module_repo}-build")

    vox_download_and_extract("${module_url}" "${module_archive_path}" "${module_source_dir}")
    file(MAKE_DIRECTORY "${module_build_dir}")

    if(WIN32)
        set(qt_configure_module_cmd "${QT_BUILD_DIR}/bin/qt-configure-module.bat")
    else()
        set(qt_configure_module_cmd "${QT_BUILD_DIR}/bin/qt-configure-module")
    endif()

    if(NOT EXISTS "${qt_configure_module_cmd}")
        message(FATAL_ERROR "qt-configure-module is missing: ${qt_configure_module_cmd}")
    endif()

    message(STATUS "Configuring Qt module '${module_repo}'")
    execute_process(
        COMMAND "${qt_configure_module_cmd}" "${module_source_dir}"
        WORKING_DIRECTORY "${module_build_dir}"
        RESULT_VARIABLE configure_result
    )
    if(NOT configure_result EQUAL 0)
        message(FATAL_ERROR "Qt module '${module_repo}' configure failed with code ${configure_result}")
    endif()

    message(STATUS "Building Qt module '${module_repo}'")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build . --parallel
        WORKING_DIRECTORY "${module_build_dir}"
        RESULT_VARIABLE build_result
    )
    if(NOT build_result EQUAL 0)
        message(FATAL_ERROR "Qt module '${module_repo}' build failed with code ${build_result}")
    endif()
endfunction()

find_package(Qt6 QUIET COMPONENTS ${VOX_QT_REQUIRED_COMPONENTS})

if(NOT Qt6_FOUND)
    message(STATUS "Required Qt components not found in bundled/system Qt. Bootstrapping Qt.")
    vox_build_qtbase()

    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Sql Concurrent Test)

    if(NOT TARGET Qt6::WebSockets)
        vox_build_qt_module(qtwebsockets)
    endif()

    if(VOX_QT_BUILD_SVG)
        find_package(Qt6 QUIET COMPONENTS Svg)
        if(NOT TARGET Qt6::Svg)
            vox_build_qt_module(qtsvg)
        endif()
    endif()

    if(VOX_QT_BUILD_MULTIMEDIA)
        find_package(Qt6 QUIET COMPONENTS Multimedia)
        if(NOT TARGET Qt6::Multimedia)
            vox_build_qt_module(qtmultimedia)
        endif()
    endif()

    find_package(Qt6 REQUIRED COMPONENTS ${VOX_QT_REQUIRED_COMPONENTS})
else()
    message(STATUS "Qt with required components found.")
endif()

if(VOX_QT_BUILD_SVG)
    find_package(Qt6 QUIET COMPONENTS Svg)
endif()

if(VOX_QT_BUILD_MULTIMEDIA)
    find_package(Qt6 QUIET COMPONENTS Multimedia)
endif()

qt_standard_project_setup()

add_library(qt INTERFACE)

target_include_directories(qt INTERFACE "${QtCMake_SOURCE_DIR}/lib/qt")
target_include_directories(qt INTERFACE "${qt_cmake_SOURCE_DIR}/lib/qt")
target_include_directories(qt INTERFACE "${qtcmake_SOURCE_DIR}/lib/qt")
target_include_directories(qt INTERFACE "${QT_INCLUDE_DIR}")

target_link_libraries(qt INTERFACE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
    Qt6::Test
    Qt6::WebSockets
)

if(TARGET Qt6::Svg)
    target_link_libraries(qt INTERFACE Qt6::Svg)
endif()

if(TARGET Qt6::Multimedia)
    target_link_libraries(qt INTERFACE Qt6::Multimedia)
endif()
