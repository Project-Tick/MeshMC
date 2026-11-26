if(__QMAKEQUERY_CMAKE__)
    return()
endif()
set(__QMAKEQUERY_CMAKE__ TRUE)

get_target_property(QMAKE_EXECUTABLE Qt${Launcher_QT_MAJOR}::qmake LOCATION)

function(QUERY_QMAKE)
    set(options "")
    set(oneValueArgs QUERY RESULT)
    set(multiValueArgs )

    cmake_parse_arguments(OPT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    execute_process(COMMAND "${QMAKE_EXECUTABLE}" "-query" "${OPT_QUERY}" RESULT_VARIABLE return_code OUTPUT_VARIABLE output )
    if(NOT return_code)
        string(STRIP "${output}" output)
        file(TO_CMAKE_PATH "${output}" output)
        set(${OPT_RESULT} ${output} PARENT_SCOPE)
        message(STATUS "QUERY_QMAKE(${OPT_VARIABLE}): SUCCEEDED - ${output}")
    else()
        message(STATUS "QUERY_QMAKE(${OPT_VARIABLE}): FAILED - ${return_code}")
    endif(NOT return_code)
endfunction(QUERY_QMAKE)
