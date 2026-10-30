target_sources(MeshMC_logic PRIVATE
    # MMCO Plugin System
    MMCOFormat.h
    PluginHooks.h
    PluginAPI.h
    PluginMetadata.h
    CoreSupersededPlugins.h
    PluginLoader.h
    PluginLoader.cpp
    PluginManager.h
    PluginManager.cpp
    PluginHookTask.h
    PluginHookTask.cpp
    PluginSignature.h
    PluginSignature.cpp
    PluginDependencyResolver.h
    PluginDependencyResolver.cpp
)
