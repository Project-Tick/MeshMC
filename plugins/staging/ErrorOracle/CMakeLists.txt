# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-License-Identifier: GPL-3.0-or-later
#
# ErrorOracle — MeshMC plugin (.mmco)
#
# LLM-free crash / log analyser. Two layers:
#
#   1. A built-in rule pack (rules/*.json) shipped with the plugin
#      mapping regex patterns to remediation advice. Loaded once at
#      init time.
#
#   2. A *learning* layer that tracks (rule, instance) -> outcome
#      pairs over time, persists them under <plugin_data>/learn.json,
#      and re-ranks rule suggestions for an instance based on what
#      worked last time. Pure local statistics, no network.
#
# Users can drop additional rule packs into
# <plugin_data>/userrules/*.json without recompiling — the rule
# engine watches that directory at init time and reloads when the
# user clicks the "Reload rules" button.

set(ERR_SOURCES
    ErrorOraclePlugin.cpp
    RuleEngine.h
    RuleEngine.cpp
    LogIngester.h
    LogIngester.cpp
    LearningStore.h
    LearningStore.cpp
    AnalysisPage.h
    AnalysisPage.cpp
)

add_library(ErrorOracle MODULE ${ERR_SOURCES})

target_link_libraries(ErrorOracle PRIVATE
    MeshMC_logic
    MeshMC::SDK
)

target_include_directories(ErrorOracle PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties(ErrorOracle PROPERTIES
    PREFIX ""
    SUFFIX ".mmco"
    LIBRARY_OUTPUT_DIRECTORY "${MESHMC_PLUGIN_STAGING_DIR}"
)

foreach(CFG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${CFG}" CFG_UPPER)
    set_target_properties(ErrorOracle PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} "${MESHMC_PLUGIN_STAGING_DIR}"
    )
endforeach()

# Install the bundled rule pack as PLUGIN DATA, not next to the
# .mmco. On macOS the .mmco lives under Contents/PlugIns/mmcmodules,
# which Apple's codesign treats as nested-code territory; dropping
# JSON files there causes
#   "code object is not signed at all
#    In subcomponent: .../PlugIns/mmcmodules/ErrorOracle/rules/jvm.json"
# at signing time. MMCO_PLUGIN_DATA_DEST_DIR resolves to:
#   Linux/Windows -> <install>/mmcmodules        (legacy, next to .mmco)
#   macOS         -> MeshMC.app/Contents/Resources/MMCOPluginData
# The runtime lookup chain in ErrorOraclePlugin::builtInRulesDir
# probes the platform-specific location first, then falls back to
# the legacy "next to the .mmco" path so older installs and ad-hoc
# developer builds keep working.
install(DIRECTORY rules
    DESTINATION "${MMCO_PLUGIN_DATA_DEST_DIR}/ErrorOracle"
    FILES_MATCHING PATTERN "*.json"
)

# Stage rule files next to the .mmco in the build tree.
file(GLOB _eo_rule_files "${CMAKE_CURRENT_SOURCE_DIR}/rules/*.json")
foreach(_rule ${_eo_rule_files})
    file(COPY "${_rule}"
         DESTINATION "${MESHMC_PLUGIN_STAGING_DIR}/ErrorOracle/rules")
endforeach()

install(TARGETS ErrorOracle
    LIBRARY DESTINATION "${MMCO_MODULES_DEST_DIR}"
)

if(MeshMC_PLUGIN_SIGN_ALL)
    mmco_sign_plugin(ErrorOracle)
endif()
