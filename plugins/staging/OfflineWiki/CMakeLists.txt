# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-License-Identifier: GPL-3.0-or-later
#
# OfflineWiki — MeshMC plugin (.mmco)
#
# A bundle viewer for offline Minecraft wikis. We don't ship the wiki
# itself (it's hundreds of megabytes); the plugin instead supports two
# bundle formats:
#
#   • Plain-directory bundle: a directory tree of Markdown files with
#     an index.json describing the navigation hierarchy. Simple, easy
#     to author by hand, suitable for small/community wikis.
#
#   • ZIM file (Kiwix format): the standard for offline reference
#     content. Read-only via a minimal in-tree ZIM reader (one source
#     file, no Kiwix runtime needed).
#
# The plugin discovers bundles under <plugin_data>/bundles/ and exposes
# a global settings page where the user can register, swap, or unload
# them.

set(WIKI_SOURCES
    OfflineWikiPlugin.cpp
    WikiBundle.h
    WikiBundle.cpp
    DirectoryBundle.h
    DirectoryBundle.cpp
    ZimBundle.h
    ZimBundle.cpp
    SearchIndex.h
    SearchIndex.cpp
    WikiPage.h
    WikiPage.cpp
)

# We use QTextBrowser for the article viewer (rich HTML/Markdown that
# works without QtWebEngine). This keeps the plugin small and avoids
# pulling Chromium into the launcher's distributable.
add_library(OfflineWiki MODULE ${WIKI_SOURCES})

target_link_libraries(OfflineWiki PRIVATE
    MeshMC_logic
    MeshMC::SDK
)

target_include_directories(OfflineWiki PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties(OfflineWiki PROPERTIES
    PREFIX ""
    SUFFIX ".mmco"
    LIBRARY_OUTPUT_DIRECTORY "${MESHMC_PLUGIN_STAGING_DIR}"
)

foreach(CFG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${CFG}" CFG_UPPER)
    set_target_properties(OfflineWiki PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} "${MESHMC_PLUGIN_STAGING_DIR}"
    )
endforeach()

install(TARGETS OfflineWiki
    LIBRARY DESTINATION "${BINARY_DEST_DIR}/mmcmodules"
)

if(MeshMC_PLUGIN_SIGN_ALL)
    mmco_sign_plugin(OfflineWiki)
endif()
