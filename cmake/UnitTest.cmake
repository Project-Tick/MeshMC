find_package(Qt${QT_VERSION_MAJOR}Test REQUIRED)
include(ECMAddTests)

set(TEST_RESOURCE_PATH ${CMAKE_CURRENT_LIST_DIR})

message(${TEST_RESOURCE_PATH})

# Thin wrapper around ecm_add_test(): the executable and the test are both called
# <name>_test. ECM creates them (console application, QT_FORCE_ASSERTS, buildtests
# target, QT_PLUGIN_PATH for the test); this adds what is specific to MeshMC:
# the optional test data and the shared test headers.
function(add_unit_test name)
    set(options "")
    set(oneValueArgs DATA)
    set(multiValueArgs SOURCES LIBS)

    cmake_parse_arguments(OPT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    set(_test_sources ${OPT_SOURCES})
    if(WIN32)
        list(APPEND _test_sources ${TEST_RESOURCE_PATH}/UnitTest/test.rc)
    endif()

    ecm_add_test(${_test_sources}
        TEST_NAME ${name}_test
        LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::Test ${OPT_LIBS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    if(NOT "${OPT_DATA}" STREQUAL "")
        set(TEST_DATA_PATH "${CMAKE_CURRENT_BINARY_DIR}/data")
        set(TEST_DATA_PATH_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${OPT_DATA}")
        message("From ${TEST_DATA_PATH_SRC} to ${TEST_DATA_PATH}")
        string(REGEX REPLACE "[/\\:]" "_" DATA_TARGET_NAME "${TEST_DATA_PATH_SRC}")
        if(UNIX)
            # on unix we get the third / from the filename
            set(TEST_DATA_URL "file://${TEST_DATA_PATH}")
        else()
            # we don't on windows, so we have to add it ourselves
            set(TEST_DATA_URL "file:///${TEST_DATA_PATH}")
        endif()
        if(NOT TARGET "${DATA_TARGET_NAME}")
            add_custom_target(${DATA_TARGET_NAME})
            add_dependencies(${name}_test ${DATA_TARGET_NAME})
            add_custom_command(
                TARGET ${DATA_TARGET_NAME}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} "-DTEST_DATA_URL=${TEST_DATA_URL}" -DSOURCE=${TEST_DATA_PATH_SRC} -DDESTINATION=${TEST_DATA_PATH} -P ${TEST_RESOURCE_PATH}/UnitTest/generate_test_data.cmake
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
        endif()
    endif()

    if(MSVC)
        target_link_options(${name}_test PRIVATE "/MANIFEST:NO")
    endif()

    target_include_directories(${name}_test PRIVATE "${TEST_RESOURCE_PATH}/UnitTest/")
endfunction()
