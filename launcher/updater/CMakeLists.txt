target_sources(MeshMC_logic PRIVATE
    ExternalUpdater.h
    UpdateLockFile.h
    UpdateLockFile.cpp
)

if(APPLE)
    if(MeshMC_SPARKLE_ENABLED)
        target_sources(MeshMC_logic PRIVATE
            MacSparkleUpdater.h
            MacSparkleUpdater.mm
        )
    endif()
else()
    target_sources(MeshMC_logic PRIVATE
        MeshMCExternalUpdater.h
        MeshMCExternalUpdater.cpp
    )
endif()
