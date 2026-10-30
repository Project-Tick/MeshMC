target_sources(MeshMC_logic PRIVATE
    # GUI - windows
    GuiUtil.h
    GuiUtil.cpp
    ColorCache.h
    ColorCache.cpp
    MainWindow.h
    MainWindow.cpp
    MainWindow.ui
    MacMenuBar.h
    MacMenuBar.cpp
    InstanceWindow.h
    InstanceWindow.cpp
)

add_subdirectory(setupwizard)
add_subdirectory(themes)
add_subdirectory(pagedialog)
add_subdirectory(pages)
add_subdirectory(dialogs)
add_subdirectory(widgets)
add_subdirectory(instanceview)

