target_sources(MeshMC_logic PRIVATE
    # GUI - paged dialog base
    BasePage.h
    BasePageContainer.h
    BasePageProvider.h
)

add_subdirectory(global)
add_subdirectory(instance)
add_subdirectory(modplatform)
