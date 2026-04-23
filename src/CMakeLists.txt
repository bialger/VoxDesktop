add_library(vox_core STATIC)

target_include_directories(vox_core PUBLIC ${CMAKE_SOURCE_DIR}/src)

# Default prefix for Qt resources added to vox_core (helps CI/tooling configs that
# require QT_RESOURCE_PREFIX even when individual qt_add_resources calls omit PREFIX).
set_property(TARGET vox_core PROPERTY QT_RESOURCE_PREFIX "/")

target_link_libraries(vox_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
    sodium
)

if(TARGET Qt6::WebSockets)
    target_link_libraries(vox_core PUBLIC Qt6::WebSockets)
    target_compile_definitions(vox_core PUBLIC VOX_HAS_QT_WEBSOCKETS=1)
endif()

if(TARGET Qt6::QSQLiteDriverPlugin)
    target_link_libraries(vox_core PUBLIC Qt6::QSQLiteDriverPlugin)
endif()

if(TARGET Qt6::Multimedia)
    target_link_libraries(vox_core PUBLIC Qt6::Multimedia)
endif()

if(TARGET Qt6::Svg)
    target_link_libraries(vox_core PUBLIC Qt6::Svg)
endif()

add_subdirectory(app)
add_subdirectory(domain)
add_subdirectory(crypto)
add_subdirectory(network)
add_subdirectory(storage)
add_subdirectory(services)
add_subdirectory(ui)
add_subdirectory(resources)
