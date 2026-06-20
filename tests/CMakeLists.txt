include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

enable_testing()

set(TEST_SUITE_SOURCES
    test_suites/GTestMain.cpp
    test_suites/ProjectUnitTestSuite.cpp
    test_suites/ProjectIntegrationTestSuite.cpp
    test_suites/ProjectVectorTestSuite.cpp
)

function(vox_add_gtest target label)
    add_executable(${target}
        ${TEST_SUITE_SOURCES}
        ${ARGN}
    )

    target_link_libraries(${target} PRIVATE
        vox_core
        GTest::gtest
        Qt6::Core
        Qt6::Network
        Qt6::Sql
    )

    target_include_directories(${target} PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/tests
    )

    if(TARGET Qt6::QSQLiteDriverPlugin)
        target_link_libraries(${target} PRIVATE Qt6::QSQLiteDriverPlugin)
    endif()

    if(COMMAND qt_import_plugins)
        set(VOX_TEST_STATIC_PLUGINS)
        if(TARGET Qt6::QSQLiteDriverPlugin)
            list(APPEND VOX_TEST_STATIC_PLUGINS Qt6::QSQLiteDriverPlugin)
        endif()
        if(TARGET Qt6::QSchannelBackendPlugin)
            list(APPEND VOX_TEST_STATIC_PLUGINS Qt6::QSchannelBackendPlugin)
        endif()
        if(TARGET Qt6::QTlsBackendCertOnlyPlugin)
            list(APPEND VOX_TEST_STATIC_PLUGINS Qt6::QTlsBackendCertOnlyPlugin)
        endif()
        if(VOX_TEST_STATIC_PLUGINS)
            qt_import_plugins(${target} INCLUDE ${VOX_TEST_STATIC_PLUGINS})
        endif()
    endif()

    gtest_discover_tests(${target}
        DISCOVERY_MODE PRE_TEST
        PROPERTIES LABELS ${label}
    )
endfunction()

vox_add_gtest(vox_unit_tests unit
    integration/FakeVoxServer.cpp
    unit/CryptoHelpersTests.cpp
    unit/JsonCodecTests.cpp
    unit/DatabaseTests.cpp
    unit/ServiceTests.cpp
)

vox_add_gtest(vox_integration_tests integration
    integration/FakeVoxServer.cpp
    integration/IntegrationFlowTests.cpp
    integration/NegativeIntegrationTests.cpp
)

vox_add_gtest(vox_vector_tests vectors
    vectors/HkdfVectorTests.cpp
    vectors/ProtocolVectorTests.cpp
)

add_executable(vox_qt_tests
    qt/ConversationListModelQtTests.cpp
    qt/QtUiTests.hpp
)

set_target_properties(vox_qt_tests PROPERTIES AUTOMOC ON)

target_link_libraries(vox_qt_tests PRIVATE
    vox_core
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
)

if(TARGET Qt6::QSQLiteDriverPlugin)
    target_link_libraries(vox_qt_tests PRIVATE Qt6::QSQLiteDriverPlugin)
endif()

if(COMMAND qt_import_plugins)
    set(VOX_QT_TEST_STATIC_PLUGINS)
    if(TARGET Qt6::QSQLiteDriverPlugin)
        list(APPEND VOX_QT_TEST_STATIC_PLUGINS Qt6::QSQLiteDriverPlugin)
    endif()
    if(TARGET Qt6::QSchannelBackendPlugin)
        list(APPEND VOX_QT_TEST_STATIC_PLUGINS Qt6::QSchannelBackendPlugin)
    endif()
    if(TARGET Qt6::QTlsBackendCertOnlyPlugin)
        list(APPEND VOX_QT_TEST_STATIC_PLUGINS Qt6::QTlsBackendCertOnlyPlugin)
    endif()
    if(VOX_QT_TEST_STATIC_PLUGINS)
        qt_import_plugins(vox_qt_tests INCLUDE ${VOX_QT_TEST_STATIC_PLUGINS})
    endif()
endif()

target_include_directories(vox_qt_tests PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/tests
)

add_test(NAME vox_qt_tests COMMAND vox_qt_tests)
set_tests_properties(vox_qt_tests PROPERTIES LABELS "qt")

if(VOX_ENABLE_CONTRACT_TESTS)
    vox_add_gtest(vox_contract_tests contract
        integration/FakeVoxServer.cpp
        integration/ContractTests.cpp
    )
endif()
