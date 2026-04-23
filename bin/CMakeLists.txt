if(WIN32)
    add_executable(vox-desktop WIN32
        main.cpp
        Application.cpp
        windows_icon.rc
    )
else()
    add_executable(vox-desktop
        main.cpp
        Application.cpp
    )
endif()

target_include_directories(vox-desktop PRIVATE
    ${CMAKE_SOURCE_DIR}/bin
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(vox-desktop PRIVATE vox_core)
set_target_properties(vox-desktop PROPERTIES OUTPUT_NAME "vox-desktop")

if(COMMAND qt_import_plugins)
    set(VOX_STATIC_RUNTIME_PLUGINS)
    if(TARGET Qt6::QSQLiteDriverPlugin)
        list(APPEND VOX_STATIC_RUNTIME_PLUGINS Qt6::QSQLiteDriverPlugin)
    endif()
    if(TARGET Qt6::QSchannelBackendPlugin)
        list(APPEND VOX_STATIC_RUNTIME_PLUGINS Qt6::QSchannelBackendPlugin)
    endif()
    if(TARGET Qt6::QTlsBackendCertOnlyPlugin)
        list(APPEND VOX_STATIC_RUNTIME_PLUGINS Qt6::QTlsBackendCertOnlyPlugin)
    endif()

    if(VOX_STATIC_RUNTIME_PLUGINS)
        qt_import_plugins(vox-desktop INCLUDE ${VOX_STATIC_RUNTIME_PLUGINS})
    endif()
endif()
