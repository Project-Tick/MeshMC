target_sources(MeshMC_logic PRIVATE
    # GUI - appearance: colour schemes, widget style and icons, from KDE Frameworks
    AppearanceManager.cpp
    AppearanceManager.h
    ColorSchemes.cpp
    ColorSchemes.h
    ThemedIcons.cpp
    ThemedIcons.h
)

if (APPLE)
    target_sources(MeshMC_logic PRIVATE
        AppearanceManager.mm
    )
endif()
