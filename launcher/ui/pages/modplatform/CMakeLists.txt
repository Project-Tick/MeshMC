target_sources(MeshMC_logic PRIVATE
    # GUI - platform pages
    VanillaPage.cpp
    VanillaPage.h
    VanillaPage.ui

    ContentProviderPage.cpp
    ContentProviderPage.h
    ContentProviderPage.ui

    ImportPage.cpp
    ImportPage.h
    ImportPage.ui
)

add_subdirectory(atlauncher)
add_subdirectory(flame)
add_subdirectory(ftb)
add_subdirectory(legacy_ftb)
add_subdirectory(modrinth)
add_subdirectory(technic)
