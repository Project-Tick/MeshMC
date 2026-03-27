cmake_minimum_required(VERSION 3.25)
project(iconfix)

find_package(Qt6Core REQUIRED QUIET)
find_package(Qt6Widgets REQUIRED QUIET)

set(ICONFIX_SOURCES
xdgicon.h
xdgicon.cpp
internal/qhexstring_p.h
internal/qiconloader.cpp
internal/qiconloader_p.h
)

add_library(MeshMC_iconfix SHARED ${ICONFIX_SOURCES})
target_include_directories(MeshMC_iconfix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_BINARY_DIR}" )

target_link_libraries(MeshMC_iconfix Qt6::Core Qt6::Widgets)

set_target_properties(MeshMC_iconfix PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN 1)
generate_export_header(MeshMC_iconfix)

# Install it
install(
    TARGETS MeshMC_iconfix
    RUNTIME DESTINATION ${LIBRARY_DEST_DIR}
    LIBRARY DESTINATION ${LIBRARY_DEST_DIR}
)