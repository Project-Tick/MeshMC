# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
#
# MMCOSign.cmake — attach a GPG signature trailer to a built .mmco
# target during the build, via scripts/mmco_sign.py.
#
# Usage:
#
#   include(MMCOSign)
#   mmco_sign_plugin(MyPluginTarget
#       KEY     "DEADBEEF..."         # explicit GPG fingerprint (optional)
#       HOMEDIR "${CMAKE_SOURCE_DIR}/.gnupg"  # optional
#   )
#
# The signing step runs as a POST_BUILD custom command. The function is
# a no-op when no key is configured — unsigned developer builds keep
# working, and the launcher will only enforce signatures on non-OSS
# modules at runtime.
#
# Cache-driven workflow for CI / release builds:
#
#   cmake -DMeshMC_PLUGIN_SIGNING_KEY=<fpr>             \
#         -DMeshMC_PLUGIN_SIGNING_HOMEDIR=<dir>         \
#         -DMeshMC_PLUGIN_SIGN_ALL=ON                   \
#         ...
#
# When MeshMC_PLUGIN_SIGN_ALL is ON, plugins/CMakeLists.txt walks every
# in-tree .mmco target and calls mmco_sign_plugin() on it automatically.

include_guard(GLOBAL)

set(MeshMC_PLUGIN_SIGNING_KEY "" CACHE STRING
    "Default GPG key fingerprint used to sign .mmco plugins. Empty disables signing.")
set(MeshMC_PLUGIN_SIGNING_HOMEDIR "" CACHE PATH
    "Optional GnuPG home directory used when signing .mmco plugins.")
set(MeshMC_PLUGIN_SIGN_ALL OFF CACHE BOOL
    "If ON, every in-tree .mmco plugin is automatically signed POST_BUILD.")

find_package(Python3 COMPONENTS Interpreter QUIET)

function(mmco_sign_plugin target)
    set(_options)
    set(_one_value KEY HOMEDIR)
    set(_multi_value)
    cmake_parse_arguments(_arg "${_options}" "${_one_value}"
                          "${_multi_value}" ${ARGN})

    if(NOT _arg_KEY)
        set(_arg_KEY "${MeshMC_PLUGIN_SIGNING_KEY}")
    endif()
    if(NOT _arg_HOMEDIR)
        set(_arg_HOMEDIR "${MeshMC_PLUGIN_SIGNING_HOMEDIR}")
    endif()

    if(NOT _arg_KEY)
        # No key configured → quietly skip. The launcher will only
        # complain at runtime if the plugin's license is not OSS.
        return()
    endif()

    if(NOT TARGET ${target})
        message(WARNING "mmco_sign_plugin: target '${target}' does not exist")
        return()
    endif()

    if(NOT Python3_Interpreter_FOUND)
        message(WARNING
            "mmco_sign_plugin(${target}): Python 3 interpreter not found, "
            "skipping signature step. Install python3 to enable signing.")
        return()
    endif()

    set(_script "${CMAKE_SOURCE_DIR}/scripts/mmco_sign.py")
    if(NOT EXISTS "${_script}")
        message(WARNING
            "mmco_sign_plugin(${target}): ${_script} not found, "
            "skipping signature step.")
        return()
    endif()

    # Build the command list. HOMEDIR is optional.
    set(_cmd
        "${Python3_EXECUTABLE}" "${_script}"
        --key "${_arg_KEY}"
    )
    if(_arg_HOMEDIR)
        list(APPEND _cmd --homedir "${_arg_HOMEDIR}")
    endif()
    list(APPEND _cmd "$<TARGET_FILE:${target}>")

    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${_cmd}
        COMMENT "Signing .mmco module ${target} with key ${_arg_KEY}"
        VERBATIM
    )
endfunction()
