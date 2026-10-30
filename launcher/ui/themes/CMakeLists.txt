target_sources(MeshMC_logic PRIVATE
    # GUI - themes
    FusionTheme.cpp
    FusionTheme.h
    BrightTheme.cpp
    BrightTheme.h
    GreenDarkTheme.cpp
    GreenDarkTheme.h
    GreenLightTheme.cpp
    GreenLightTheme.h
    CustomTheme.cpp
    CustomTheme.h
    DarkTheme.cpp
    DarkTheme.h
    ITheme.cpp
    ITheme.h
    SystemTheme.cpp
    SystemTheme.h
    ThemeManager.cpp
    ThemeManager.h
    CatPack.cpp
    CatPack.h
)

if (APPLE)
    target_sources(MeshMC_logic PRIVATE
        ThemeManager.mm
    )
endif()
