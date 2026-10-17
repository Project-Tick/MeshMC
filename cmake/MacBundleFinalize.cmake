# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-FileContributor: Project Tick
# SPDX-License-Identifier: Apache-2.0
#
# MacBundleFinalize.cmake -- the last step of the macOS install, included from
# an install(CODE) rule at the end of the top-level CMakeLists.txt with
# MESHMC_BUNDLE set to the installed MeshMC.app.
#
# macdeployqt (launcher/CMakeLists.txt) makes meshmc, the crash reporter and
# the Qt, QML and KDE plugins it is handed load their libraries from the
# bundle. Two things are left after it:
#
#   * Code installed later or beside it -- the .mmco plugins -- can still name
#     a library by its absolute path on the build machine. KDE Craft gives its
#     libraries absolute install names, so a plugin linking KI18n loads
#     /…/CraftRoot/lib/libKF6I18n.6.dylib: it works where it was built and
#     nowhere else. Such a reference is pointed at the bundle's copy under
#     Contents/Frameworks, through @rpath, and an install that leaves one it
#     cannot point there fails here instead of shipping.
#   * Run paths into the build machine's library directory. Craft's libraries
#     each carry an LC_RPATH of /…/CraftRoot/lib, which macdeployqt keeps; any
#     @rpath reference then finds the Craft copy first, wherever a copy also
#     sits in the bundle, and two QtQml frameworks in one process abort the
#     QML engine ("Cannot add multiple registrations for QtQml"). Absolute run
#     paths are removed; a file that still needs @rpath gets one into
#     Contents/Frameworks.
#   * install_name_tool invalidates a binary's signature, and arm64 refuses
#     to load unsigned code. Whatever the install touched afterwards is signed
#     again ad hoc, then the bundle itself; a release signs it properly later
#     (.github/actions/package/macos).

if(NOT IS_DIRECTORY "${MESHMC_BUNDLE}/Contents")
    message(FATAL_ERROR "MacBundleFinalize: no app bundle at '${MESHMC_BUNDLE}'")
endif()
set(_mbf_contents "${MESHMC_BUNDLE}/Contents")
set(_mbf_frameworks "${_mbf_contents}/Frameworks")

# Mach-O files: executables, loose libraries and plugins, framework binaries.
file(GLOB_RECURSE _mbf_files LIST_DIRECTORIES false
    "${_mbf_contents}/MacOS/*"
    "${_mbf_contents}/*.dylib"
    "${_mbf_contents}/*.so"
    "${_mbf_contents}/*.mmco"
)
file(GLOB _mbf_framework_dirs LIST_DIRECTORIES true "${_mbf_frameworks}/*.framework")
foreach(_fw IN LISTS _mbf_framework_dirs)
    get_filename_component(_fw_name "${_fw}" NAME_WE)
    file(GLOB _fw_binaries LIST_DIRECTORIES false "${_fw}/Versions/*/${_fw_name}")
    list(APPEND _mbf_files ${_fw_binaries})
endforeach()

set(_mbf_machos "")
foreach(_file IN LISTS _mbf_files)
    if(IS_SYMLINK "${_file}" OR _file MATCHES "/Versions/Current/")
        continue()
    endif()
    file(READ "${_file}" _magic LIMIT 4 HEX)
    # 64-bit Mach-O (little endian) or a universal binary.
    if(_magic STREQUAL "cffaedfe" OR _magic STREQUAL "cafebabe")
        list(APPEND _mbf_machos "${_file}")
    endif()
endforeach()

set(_mbf_unresolved "")
set(_mbf_rewritten 0)
foreach(_file IN LISTS _mbf_machos)
    execute_process(COMMAND otool -L "${_file}" OUTPUT_VARIABLE _deps_out ERROR_QUIET)
    execute_process(COMMAND otool -D "${_file}" OUTPUT_VARIABLE _id_out ERROR_QUIET)
    execute_process(COMMAND otool -l "${_file}" OUTPUT_VARIABLE _load_cmds ERROR_QUIET)
    string(REGEX MATCHALL "\t[^\n]* \\(compatibility version" _dep_lines "${_deps_out}")
    string(REPLACE "\n" ";" _id_lines "${_id_out}")

    get_filename_component(_dir "${_file}" DIRECTORY)
    set(_changed FALSE)
    set(_uses_rpath FALSE)
    foreach(_line IN LISTS _dep_lines)
        string(REGEX REPLACE "^\t(.*) \\(compatibility version$" "\\1" _dep "${_line}")
        # A library's own install name is listed too; it never loads anything.
        if(_dep IN_LIST _id_lines OR _dep MATCHES "^/(System|usr/lib)/")
            continue()
        endif()
        # Bundle-relative references have to land on a file in the bundle.
        if(_dep MATCHES "^@(rpath|loader_path|executable_path)/(.*)$")
            set(_base "${_mbf_frameworks}")
            if(CMAKE_MATCH_1 STREQUAL "rpath")
                set(_uses_rpath TRUE)
            elseif(CMAKE_MATCH_1 STREQUAL "loader_path")
                set(_base "${_dir}")
            else()
                set(_base "${_mbf_contents}/MacOS")
            endif()
            if(NOT EXISTS "${_base}/${CMAKE_MATCH_2}")
                list(APPEND _mbf_unresolved "${_file}: ${_dep}")
            endif()
            continue()
        endif()

        # .../QtCore.framework/Versions/A/QtCore or .../libfoo.6.dylib
        get_filename_component(_dep_rel "${_dep}" NAME)
        if(_dep MATCHES "/([^/]+\\.framework/.+)$")
            set(_dep_rel "${CMAKE_MATCH_1}")
        endif()
        if(NOT EXISTS "${_mbf_frameworks}/${_dep_rel}")
            list(APPEND _mbf_unresolved "${_file}: ${_dep}")
            continue()
        endif()
        execute_process(COMMAND install_name_tool -change "${_dep}" "@rpath/${_dep_rel}" "${_file}"
            RESULT_VARIABLE _rc ERROR_QUIET)
        if(NOT _rc EQUAL 0)
            list(APPEND _mbf_unresolved "${_file}: ${_dep} (install_name_tool failed)")
            continue()
        endif()
        set(_changed TRUE)
        set(_uses_rpath TRUE)
    endforeach()

    string(REGEX MATCHALL "\n *path [^\n]* \\(offset" _rpath_lines "${_load_cmds}")
    set(_rpaths "")
    foreach(_line IN LISTS _rpath_lines)
        string(REGEX REPLACE "^\n *path (.*) \\(offset$" "\\1" _rpath "${_line}")
        if(_rpath MATCHES "^/")
            execute_process(COMMAND install_name_tool -delete_rpath "${_rpath}" "${_file}"
                OUTPUT_QUIET ERROR_QUIET)
            set(_changed TRUE)
        else()
            list(APPEND _rpaths "${_rpath}")
        endif()
    endforeach()

    if(_uses_rpath)
        file(RELATIVE_PATH _to_frameworks "${_dir}" "${_mbf_frameworks}")
        set(_bundle_rpath "@loader_path")
        if(_to_frameworks)
            set(_bundle_rpath "@loader_path/${_to_frameworks}")
        endif()
        if(NOT _bundle_rpath IN_LIST _rpaths)
            execute_process(COMMAND install_name_tool
                -add_rpath "${_bundle_rpath}" "${_file}" OUTPUT_QUIET ERROR_QUIET)
            set(_changed TRUE)
        endif()
    endif()
    if(_changed)
        math(EXPR _mbf_rewritten "${_mbf_rewritten} + 1")
    endif()
endforeach()
message(STATUS "MacBundleFinalize: pointed ${_mbf_rewritten} binaries in MeshMC.app at Contents/Frameworks")

if(_mbf_unresolved)
    list(JOIN _mbf_unresolved "\n  " _mbf_unresolved_text)
    message(FATAL_ERROR
        "MeshMC.app loads libraries it does not contain (from the build "
        "machine, or a bundle path that is not there):\n  ${_mbf_unresolved_text}")
endif()

# Sign ad hoc whatever no longer verifies: loose binaries one by one, each
# framework as a whole, then the bundle.
function(_mbf_sign_if_invalid path)
    execute_process(COMMAND codesign --verify "${path}"
        RESULT_VARIABLE _rc OUTPUT_QUIET ERROR_QUIET)
    if(NOT _rc EQUAL 0)
        execute_process(COMMAND codesign --force --sign - "${path}"
            RESULT_VARIABLE _rc OUTPUT_QUIET ERROR_VARIABLE _err)
        if(NOT _rc EQUAL 0)
            message(WARNING "Could not sign ${path} ad hoc: ${_err}")
        endif()
    endif()
endfunction()

foreach(_file IN LISTS _mbf_machos)
    if(NOT _file MATCHES "\\.framework/")
        _mbf_sign_if_invalid("${_file}")
    endif()
endforeach()
foreach(_fw IN LISTS _mbf_framework_dirs)
    _mbf_sign_if_invalid("${_fw}")
endforeach()
_mbf_sign_if_invalid("${MESHMC_BUNDLE}")
