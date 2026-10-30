target_sources(MeshMC_logic PRIVATE
    # Flame
    FlamePackIndex.cpp
    FlamePackIndex.h
    PackManifest.h
    PackManifest.cpp
    FileResolvingTask.h
    FileResolvingTask.cpp
    FlameContentModel.h
    FlameContentModel.cpp
    FlameApi.h
    FlameApi.cpp
    # The file identifier CurseForge looks files up by, and the export
    # that has to compute it to name a mod instead of shipping it.
    FlameFingerprint.h
    FlameFingerprint.cpp
    FlamePackExportTask.h
    FlamePackExportTask.cpp
)
