# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-FileContributor: Project Tick
# SPDX-License-Identifier: Apache-2.0
#
# WinDeployRuntime.cmake -- included from an install(CODE) rule in
# launcher/CMakeLists.txt, right after windeployqt, with:
#
#   MESHMC_DEPLOY_PREFIX   the install prefix
#   MESHMC_DEPLOY_BIN      where meshmc.exe and its DLLs go (relative)
#   MESHMC_DEPLOY_SCAN     files and directories (relative) whose DLLs must
#                          find everything they link; directories are walked
#   MESHMC_DEPLOY_SEARCH   directories to take those libraries from
#   MESHMC_DEPLOY_TOOL     dumpbin (MSVC) or objdump (MinGW)
#
# windeployqt copies Qt's own libraries and nothing else. The QML modules it
# deploys and the KDE plugins installed next to them also link Kirigami, KDE
# Frameworks and their third-party libraries -- meshmc.exe links only some of
# those, so RUNTIME_DEPENDENCY_SET does not bring them all. They are resolved
# here and copied next to meshmc.exe, where Windows looks for them.

set(_wdr_files "")
foreach(_entry IN LISTS MESHMC_DEPLOY_SCAN)
    set(_path "${MESHMC_DEPLOY_PREFIX}/${_entry}")
    if(IS_DIRECTORY "${_path}")
        file(GLOB_RECURSE _dlls "${_path}/*.dll")
        list(APPEND _wdr_files ${_dlls})
    elseif(EXISTS "${_path}")
        list(APPEND _wdr_files "${_path}")
    endif()
endforeach()
if(NOT _wdr_files)
    return()
endif()

set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "${MESHMC_DEPLOY_TOOL}")
file(GET_RUNTIME_DEPENDENCIES
    LIBRARIES ${_wdr_files}
    RESOLVED_DEPENDENCIES_VAR _wdr_resolved
    UNRESOLVED_DEPENDENCIES_VAR _wdr_unresolved
    DIRECTORIES ${MESHMC_DEPLOY_SEARCH}
    PRE_EXCLUDE_REGEXES
        "^(api-ms-win|ext-ms)-.*\\.dll$"
        "^azure.*\\.dll$"
        "^vcruntime.*\\.dll$"
    POST_EXCLUDE_REGEXES
        "[Ss]ystem32"
)

set(_wdr_dest "${MESHMC_DEPLOY_PREFIX}/${MESHMC_DEPLOY_BIN}")
foreach(_dll IN LISTS _wdr_resolved)
    get_filename_component(_name "${_dll}" NAME)
    # Whatever windeployqt or RUNTIME_DEPENDENCY_SET already put there wins.
    if(NOT EXISTS "${_wdr_dest}/${_name}")
        file(INSTALL "${_dll}" DESTINATION "${_wdr_dest}")
    endif()
endforeach()
if(_wdr_unresolved)
    message(WARNING "Libraries the QML modules and KDE plugins link but no "
        "search directory has: ${_wdr_unresolved}")
endif()
