# Support for Minecraft instances and launch
target_sources(MeshMC_logic PRIVATE
    GradleSpecifier.h
    MinecraftInstance.cpp
    MinecraftInstance.h
    LaunchProfile.cpp
    LaunchProfile.h
    Component.cpp
    Component.h
    PackProfile.cpp
    PackProfile.h
    ComponentUpdateTask.cpp
    ComponentUpdateTask.h
    MinecraftLoadAndCheck.h
    MinecraftLoadAndCheck.cpp
    MinecraftUpdate.h
    MinecraftUpdate.cpp
    MojangVersionFormat.cpp
    MojangVersionFormat.h
    Rule.cpp
    Rule.h
    OneSixVersionFormat.cpp
    OneSixVersionFormat.h
    OpSys.cpp
    OpSys.h
    ParseUtils.cpp
    ParseUtils.h
    ProfileUtils.cpp
    ProfileUtils.h
    ShortcutUtils.cpp
    ShortcutUtils.h
    Library.cpp
    Library.h
    MojangDownloadInfo.h
    VersionFile.cpp
    VersionFile.h
    VersionFilterData.h
    VersionFilterData.cpp
    World.h
    World.cpp
    WorldList.h
    WorldList.cpp

    # Assets
    AssetsUtils.h
    AssetsUtils.cpp

    # Local skin library
    skins/SkinEntry.cpp
    skins/SkinEntry.h
    skins/SkinLibrary.cpp
    skins/SkinLibrary.h
    skins/ProfileSkinImport.cpp
    skins/ProfileSkinImport.h
)

add_subdirectory(auth)
add_subdirectory(gameoptions)
add_subdirectory(update)
add_subdirectory(launch)
add_subdirectory(legacy)
add_subdirectory(mod)
add_subdirectory(services)
add_subdirectory(skins)
