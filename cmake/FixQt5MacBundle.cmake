# Qt 5 macOS bundle post-processing. Must be included at the very end of the
# root CMakeLists.txt so it runs after every other install() rule
# (macdeployqt, runtime dependency set, .mmco modules, ...).
install(CODE [=[
    set(_bundle "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/MeshMC.app")
    if(NOT IS_DIRECTORY "${_bundle}")
        message(FATAL_ERROR "App bundle '${_bundle}' is missing")
    endif()

    file(GLOB_RECURSE _all LIST_DIRECTORIES false "${_bundle}/Contents/*")

    # Collect Mach-O files once.
    set(_macho "")
    foreach(_f IN LISTS _all)
        execute_process(COMMAND /usr/bin/file -b "${_f}"
                        OUTPUT_VARIABLE _t OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(_t MATCHES "Mach-O")
            list(APPEND _macho "${_f}")
        endif()
    endforeach()

    # ---- 1) Rewrite install IDs and Homebrew dependencies ----
    foreach(_f IN LISTS _macho)
        # Install ID (only dylibs/frameworks/plugins have one)
        execute_process(COMMAND otool -D "${_f}"
                        OUTPUT_VARIABLE _id_out OUTPUT_STRIP_TRAILING_WHITESPACE)
        string(REGEX MATCH "/opt/homebrew/[^ ()\n]+" _id "${_id_out}")
        if(_id)
            get_filename_component(_name "${_id}" NAME)
            if(_id MATCHES "/([^/]+\\.framework)/Versions/([^/]+)/([^/]+)$")
                set(_new_id "@rpath/${CMAKE_MATCH_1}/Versions/${CMAKE_MATCH_2}/${CMAKE_MATCH_3}")
            else()
                set(_new_id "@rpath/${_name}")
            endif()
            execute_process(COMMAND install_name_tool -id "${_new_id}" "${_f}"
                            RESULT_VARIABLE _r ERROR_QUIET)
            if(NOT _r EQUAL 0)
                message(FATAL_ERROR "install_name_tool -id failed: ${_f}")
            endif()
        endif()

        # Dependencies
        execute_process(COMMAND otool -L "${_f}"
                        OUTPUT_VARIABLE _deps_out OUTPUT_STRIP_TRAILING_WHITESPACE)
        string(REGEX MATCHALL "/opt/homebrew/[^ ()\n]+" _deps "${_deps_out}")
        list(REMOVE_DUPLICATES _deps)

        foreach(_dep IN LISTS _deps)
            if(_dep STREQUAL "${_id}")
                continue()
            endif()
            if(_dep MATCHES "/([^/]+\\.framework)/Versions/([^/]+)/([^/]+)$")
                set(_new "@executable_path/../Frameworks/${CMAKE_MATCH_1}/Versions/${CMAKE_MATCH_2}/${CMAKE_MATCH_3}")
            elseif(_dep MATCHES "/([^/]+\\.dylib)$")
                set(_dylib "${CMAKE_MATCH_1}")
                if(NOT EXISTS "${_bundle}/Contents/Frameworks/${_dylib}")
                    message(FATAL_ERROR "${_dylib} is not bundled, needed by ${_f}")
                endif()
                set(_new "@executable_path/../Frameworks/${_dylib}")
            else()
                message(FATAL_ERROR "Unexpected Homebrew dependency in ${_f}: ${_dep}")
            endif()
            execute_process(COMMAND install_name_tool -change "${_dep}" "${_new}" "${_f}"
                            RESULT_VARIABLE _r ERROR_QUIET)
            if(NOT _r EQUAL 0)
                message(FATAL_ERROR "install_name_tool -change failed: ${_f}: ${_dep}")
            endif()
        endforeach()
    endforeach()

    # ---- 2) Verify nothing points to Homebrew anymore ----
    foreach(_f IN LISTS _macho)
        execute_process(COMMAND otool -L "${_f}"
                        OUTPUT_VARIABLE _after OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(_after MATCHES "/opt/homebrew/")
            message(FATAL_ERROR "Homebrew dependency remains in ${_f}:\n${_after}")
        endif()
    endforeach()

    # ---- 3) Re-sign inside-out (install_name_tool invalidates signatures) ----
    foreach(_f IN LISTS _macho)
        execute_process(COMMAND codesign --force --sign - "${_f}"
                        RESULT_VARIABLE _r ERROR_QUIET)
        if(NOT _r EQUAL 0)
            message(FATAL_ERROR "codesign failed: ${_f}")
        endif()
    endforeach()
    file(GLOB _fws "${_bundle}/Contents/Frameworks/*.framework")
    foreach(_fw IN LISTS _fws)
        execute_process(COMMAND codesign --force --sign - "${_fw}"
                        RESULT_VARIABLE _r ERROR_QUIET)
        if(NOT _r EQUAL 0)
            message(FATAL_ERROR "codesign failed: ${_fw}")
        endif()
    endforeach()
    execute_process(COMMAND codesign --force --sign - "${_bundle}"
                    RESULT_VARIABLE _r ERROR_QUIET)
    if(NOT _r EQUAL 0)
        message(FATAL_ERROR "codesign failed for the bundle")
    endif()
    execute_process(COMMAND codesign --verify --deep --strict "${_bundle}"
                    RESULT_VARIABLE _r)
    if(NOT _r EQUAL 0)
        message(FATAL_ERROR "bundle signature verification failed")
    endif()

    message(STATUS "Qt5 macOS bundle fixed and signed: ${_bundle}")
]=])
