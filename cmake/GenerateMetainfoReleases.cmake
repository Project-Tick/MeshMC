# SPDX-FileCopyrightText: 2026 Project Tick
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# GenerateMetainfoReleases.cmake
#
# Generates the AppStream <releases> block for the MeshMC metainfo file from
# CHANGELOG.md (the single source of truth). This is implemented in pure CMake
# script so it has zero external dependencies (no Python, no shell tools) and
# therefore works identically in every build environment: GitLab CI, the
# Flatpak/KDE SDK sandbox (offline), Nix sandbox, vcpkg/Windows and macOS.
#
# Usage (function form, from the main CMakeLists after the variable is set):
#
#   include(GenerateMetainfoReleases)
#   meshmc_generate_metainfo_releases(
#       CHANGELOG "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md"
#       OUTPUT_VARIABLE MeshMC_METAINFO_RELEASES)
#
# The CHANGELOG format is fixed (produced by GitLab's changelog tooling):
#
#   ## 8.1.0 (2026-06-17)
#
#   ### changed (1 change)
#
#   - [Bump version 8.0.0 -> 8.1.0](https://gitlab.com/.../commit/abc123)
#
# Mapping to AppStream:
#   ## X.Y.Z (YYYY-MM-DD)  -> <release version="X.Y.Z" date="YYYY-MM-DD">
#   ### <type> (N changes) -> <p>Type</p> + <ul>
#   - [text](url)          -> <li>text</li>  (commit link dropped)

# Escape the five predefined XML entities for safe insertion into the document.
function(_meshmc_xml_escape IN_VAR OUT_VAR)
    set(_s "${${IN_VAR}}")
    string(REPLACE "&" "&amp;" _s "${_s}")
    string(REPLACE "<" "&lt;" _s "${_s}")
    string(REPLACE ">" "&gt;" _s "${_s}")
    string(REPLACE "\"" "&quot;" _s "${_s}")
    string(REPLACE "'" "&apos;" _s "${_s}")
    set(${OUT_VAR} "${_s}" PARENT_SCOPE)
endfunction()

# Reduce inline markdown in a changelog item to plain text.
function(_meshmc_strip_markdown IN_VAR OUT_VAR)
    set(_s "${${IN_VAR}}")
    # Markdown link: [text](url) -> text
    string(REGEX REPLACE "\\[([^]]+)\\]\\([^)]*\\)" "\\1" _s "${_s}")
    # Bold: **text** -> text
    string(REGEX REPLACE "\\*\\*([^*]+)\\*\\*" "\\1" _s "${_s}")
    # Inline code: `text` -> text
    string(REGEX REPLACE "`([^`]+)`" "\\1" _s "${_s}")
    # Trim surrounding whitespace.
    string(STRIP "${_s}" _s)
    set(${OUT_VAR} "${_s}" PARENT_SCOPE)
endfunction()

# Capitalize the first letter of a section title ("changed" -> "Changed").
function(_meshmc_normalize_title IN_VAR OUT_VAR)
    set(_s "${${IN_VAR}}")
    string(STRIP "${_s}" _s)
    if(_s STREQUAL "")
        set(${OUT_VAR} "" PARENT_SCOPE)
        return()
    endif()
    string(SUBSTRING "${_s}" 0 1 _first)
    string(SUBSTRING "${_s}" 1 -1 _rest)
    string(TOUPPER "${_first}" _first)
    set(${OUT_VAR} "${_first}${_rest}" PARENT_SCOPE)
endfunction()

function(meshmc_generate_metainfo_releases)
    set(_options "")
    set(_one_value CHANGELOG OUTPUT_VARIABLE INDENT)
    set(_multi_value "")
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    if(NOT ARG_CHANGELOG)
        message(FATAL_ERROR "meshmc_generate_metainfo_releases: CHANGELOG is required")
    endif()
    if(NOT ARG_OUTPUT_VARIABLE)
        message(FATAL_ERROR "meshmc_generate_metainfo_releases: OUTPUT_VARIABLE is required")
    endif()
    if(NOT ARG_INDENT)
        set(ARG_INDENT "  ")
    endif()
    if(NOT EXISTS "${ARG_CHANGELOG}")
        message(FATAL_ERROR "meshmc_generate_metainfo_releases: changelog not found: ${ARG_CHANGELOG}")
    endif()

    set(_i "${ARG_INDENT}")

    file(READ "${ARG_CHANGELOG}" _changelog)
    # Normalize CRLF and split into a list of lines. A literal ';' would be
    # interpreted as a CMake list separator while we iterate, so temporarily
    # encode it with an unlikely sentinel and restore it per line below.
    set(_semi_token "@@MESHMC_SEMICOLON@@")
    string(REPLACE "\r\n" "\n" _changelog "${_changelog}")
    string(REPLACE ";" "${_semi_token}" _changelog "${_changelog}")
    string(REPLACE "\n" ";" _lines "${_changelog}")

    set(_out "${_i}<releases>")
    set(_have_release FALSE)
    set(_in_list FALSE)        # currently inside a <ul>
    set(_release_open FALSE)   # a <release> has been opened
    set(_desc_open FALSE)      # the <description> has been opened
    set(_release_has_content FALSE)
    set(_pending_version "")

    foreach(_raw IN LISTS _lines)
        # Restore literal semicolons inside the line.
        string(REPLACE "${_semi_token}" ";" _line "${_raw}")
        string(REGEX REPLACE "[ \t]+$" "" _line "${_line}")

        # Version header: ## X.Y.Z (YYYY-MM-DD)  or  ## X.Y.Z
        # NB: CMake regex does not support {N} repetition, so the date pattern
        # is spelled out digit by digit.
        if(_line MATCHES "^## +v?([0-9]+\\.[0-9]+\\.[0-9]+[0-9A-Za-z.+-]*)( +\\(([0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9])\\))?[ \t]*$")
            set(_version "${CMAKE_MATCH_1}")
            set(_date "${CMAKE_MATCH_3}")

            # Close the previous release, if any.
            if(_release_open)
                if(_in_list)
                    string(APPEND _out "\n${_i}      </ul>")
                    set(_in_list FALSE)
                endif()
                if(NOT _release_has_content)
                    string(APPEND _out "\n${_i}      <p>Release ${_pending_version}.</p>")
                endif()
                string(APPEND _out "\n${_i}    </description>")
                string(APPEND _out "\n${_i}  </release>")
            endif()

            _meshmc_xml_escape(_version _version_esc)
            if(_date)
                _meshmc_xml_escape(_date _date_esc)
                string(APPEND _out "\n${_i}  <release version=\"${_version_esc}\" date=\"${_date_esc}\">")
            else()
                string(APPEND _out "\n${_i}  <release version=\"${_version_esc}\">")
            endif()
            string(APPEND _out "\n${_i}    <description>")

            set(_have_release TRUE)
            set(_release_open TRUE)
            set(_desc_open TRUE)
            set(_release_has_content FALSE)
            set(_in_list FALSE)
            set(_pending_version "${_version_esc}")
            continue()
        endif()

        if(NOT _release_open)
            # Skip the document title / preamble before the first release.
            continue()
        endif()

        # Section header: ### changed (1 change)  or  ### Added
        if(_line MATCHES "^### +(.+)$")
            set(_title "${CMAKE_MATCH_1}")
            # Strip a trailing "(N changes)" / "(N change)" count.
            string(REGEX REPLACE " *\\([0-9]+ +changes?\\)[ \t]*$" "" _title "${_title}")
            _meshmc_normalize_title(_title _title)
            if(_title STREQUAL "")
                continue()
            endif()
            if(_in_list)
                string(APPEND _out "\n${_i}      </ul>")
                set(_in_list FALSE)
            endif()
            _meshmc_xml_escape(_title _title_esc)
            string(APPEND _out "\n${_i}      <p>${_title_esc}</p>")
            string(APPEND _out "\n${_i}      <ul>")
            set(_in_list TRUE)
            set(_release_has_content TRUE)
            continue()
        endif()

        # List item: - [text](url)  or  * text
        if(_line MATCHES "^[-*] +(.+)$")
            set(_item "${CMAKE_MATCH_1}")
            _meshmc_strip_markdown(_item _item)
            if(_item STREQUAL "")
                continue()
            endif()
            if(NOT _in_list)
                # Items without a preceding section header.
                string(APPEND _out "\n${_i}      <p>Changed</p>")
                string(APPEND _out "\n${_i}      <ul>")
                set(_in_list TRUE)
            endif()
            _meshmc_xml_escape(_item _item_esc)
            string(APPEND _out "\n${_i}        <li>${_item_esc}</li>")
            set(_release_has_content TRUE)
            continue()
        endif()
    endforeach()

    # Close the final open release.
    if(_release_open)
        if(_in_list)
            string(APPEND _out "\n${_i}      </ul>")
        endif()
        if(NOT _release_has_content)
            string(APPEND _out "\n${_i}      <p>Release ${_pending_version}.</p>")
        endif()
        string(APPEND _out "\n${_i}    </description>")
        string(APPEND _out "\n${_i}  </release>")
    endif()

    string(APPEND _out "\n${_i}</releases>")

    if(NOT _have_release)
        message(FATAL_ERROR
            "meshmc_generate_metainfo_releases: no releases parsed from "
            "${ARG_CHANGELOG}; is the changelog format correct?")
    endif()

    set(${ARG_OUTPUT_VARIABLE} "${_out}" PARENT_SCOPE)
endfunction()
