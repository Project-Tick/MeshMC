# DiscordRPC — MeshMC plugin (.mmco)
#
# Discord Rich Presence integration. Talks to the local Discord client
# over the documented IPC socket (UNIX socket on Linux/macOS, named
# pipe on Windows). No network access required.
#
# Vendor:
#   vendor/discord_ipc/  — Qt-only minimal Discord IPC client (MIT)

set(DISCORDRPC_SOURCES
    DiscordRPCPlugin.cpp
    vendor/discord_ipc/discord_ipc.h
    vendor/discord_ipc/discord_ipc.cpp
)

add_library(DiscordRPC MODULE ${DISCORDRPC_SOURCES})

target_link_libraries(DiscordRPC PRIVATE
    MeshMC_logic
    MeshMC::SDK
    Qt6::Network            # QLocalSocket lives here
)

target_include_directories(DiscordRPC PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor"
)

set_target_properties(DiscordRPC PROPERTIES
    PREFIX ""
    SUFFIX ".mmco"
    LIBRARY_OUTPUT_DIRECTORY "${MESHMC_PLUGIN_STAGING_DIR}"
)

foreach(CFG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${CFG}" CFG_UPPER)
    set_target_properties(DiscordRPC PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} "${MESHMC_PLUGIN_STAGING_DIR}"
    )
endforeach()

install(TARGETS DiscordRPC
    LIBRARY DESTINATION "${MMCO_MODULES_DEST_DIR}"
)

if(MeshMC_PLUGIN_SIGN_ALL)
    mmco_sign_plugin(DiscordRPC)
endif()
