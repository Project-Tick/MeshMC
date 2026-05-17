# NewsViewer — MeshMC plugin (.mmco)
# Displays a news viewer dialog with multi-feed RSS support.
# Content is rendered via cmark (CommonMark) for Markdown fallback.

set(NEWSVIEWER_SOURCES
    NewsViewerPlugin.cpp
    NewsViewerDialog.h
    NewsViewerDialog.cpp
)

add_library(NewsViewer MODULE ${NEWSVIEWER_SOURCES})

target_link_libraries(NewsViewer PRIVATE
    MeshMC_logic        # in-tree: launcher symbols
    MeshMC::SDK         # SDK: Qt + MeshMC includes
    cmark::cmark         # CommonMark parser
)

target_include_directories(NewsViewer PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Output as .mmco into the staging directory
set_target_properties(NewsViewer PROPERTIES
    PREFIX ""
    SUFFIX ".mmco"
    LIBRARY_OUTPUT_DIRECTORY "${MESHMC_PLUGIN_STAGING_DIR}"
)

foreach(CFG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${CFG}" CFG_UPPER)
    set_target_properties(NewsViewer PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} "${MESHMC_PLUGIN_STAGING_DIR}"
    )
endforeach()

install(TARGETS NewsViewer
    LIBRARY DESTINATION "${MMCO_MODULES_DEST_DIR}"
)
