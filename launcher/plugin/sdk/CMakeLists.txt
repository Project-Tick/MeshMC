# MeshMC Plugin SDK — headers and CMake config for .mmco module development
#
# This target provides:
#   - The mmco_sdk.h single-include header
#   - PluginAPI.h / PluginHooks.h / MMCOFormat.h  (C ABI definitions)
#   - All transitive MeshMC & library headers required by the SDK
#   - A CMake package config so out-of-tree plugins can:
#       find_package(MeshMC_SDK REQUIRED)
#       target_link_libraries(MyPlugin PRIVATE MeshMC::SDK)

# ---------------------------------------------------------------------------
# INTERFACE library — no compiled code, only headers + include paths
# ---------------------------------------------------------------------------
add_library(MeshMC_sdk INTERFACE)
add_library(MeshMC::SDK ALIAS MeshMC_sdk)
set_target_properties(MeshMC_sdk PROPERTIES EXPORT_NAME SDK)

# ---------------------------------------------------------------------------
# C ABI INTERFACE library — Qt-free, header-only.
#
# This target exposes ONLY the language-neutral C ABI (mmco_c_sdk.h plus
# the PluginAPI.h / PluginHooks.h / MMCOFormat.h definitions it builds on).
# It deliberately does NOT link Qt, so plugins written in plain C — or in
# any language that speaks the C ABI (Rust via bindgen, Zig, …) — can build
# a .mmco against it without dragging in the Qt toolchain.
#
# C plugins:    target_link_libraries(MyPlugin PRIVATE MeshMC::SDK_C)
# C++ plugins:  target_link_libraries(MyPlugin PRIVATE MeshMC::SDK)
# ---------------------------------------------------------------------------
add_library(MeshMC_sdk_c INTERFACE)
add_library(MeshMC::SDK_C ALIAS MeshMC_sdk_c)
set_target_properties(MeshMC_sdk_c PROPERTIES EXPORT_NAME SDK_C)

target_include_directories(MeshMC_sdk_c INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/launcher/headers>
    $<INSTALL_INTERFACE:include/meshmc-sdk/headers>
)
# Note: no target_link_libraries here — the C ABI surface is Qt-free.

# The C++ SDK builds on top of the C ABI surface, so it inherits the
# C-ABI include paths and adds Qt on top of them.
target_link_libraries(MeshMC_sdk INTERFACE MeshMC_sdk_c)

# Resolve include directories from imported targets (these are installed externally)
get_target_property(_KATABASIS_INCLUDE_DIRS Katabasis INTERFACE_INCLUDE_DIRECTORIES)

target_link_libraries(MeshMC_sdk INTERFACE
    MeshMC_sdk_c
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
)

# ---------------------------------------------------------------------------
# Install rules
# ---------------------------------------------------------------------------
# Distributions disagree on the library directory ("lib64" on Fedora,
# "lib/<arch>-linux-gnu" under Debian multiarch), so the CMake package files
# below go to ${CMAKE_INSTALL_LIBDIR} rather than a hardcoded "lib". Included
# here as well as in the top-level CMakeLists so this directory still works if
# it is ever configured on its own. The header directory needs no such care:
# "include" is "include" everywhere.
include(GNUInstallDirs)

set(SDK_INSTALL_DIR "include/meshmc-sdk")

install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/launcher/"
    DESTINATION "${SDK_INSTALL_DIR}"
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*.hh"
        PATTERN "*.hxx"
)

install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/launcher/headers/"
    DESTINATION "${SDK_INSTALL_DIR}/headers"
)

# Library headers — resolve from installed package include directories
foreach(_dir ${_KATABASIS_INCLUDE_DIRS})
    if(EXISTS "${_dir}/katabasis/Bits.h")
        install(FILES "${_dir}/katabasis/Bits.h" DESTINATION "${SDK_INSTALL_DIR}/katabasis")
        break()
    endif()
endforeach()

# ---------------------------------------------------------------------------
# CMake package config for out-of-tree plugins (find_package support)
# ---------------------------------------------------------------------------
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/MeshMC_SDKConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MeshMC_SDKConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/MeshMC_SDK"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MeshMC_SDKConfigVersion.cmake"
    VERSION "${MeshMC_RELEASE_VERSION_NAME}"
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MeshMC_SDKConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MeshMC_SDKConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/MeshMC_SDK"
)

install(TARGETS MeshMC_sdk MeshMC_sdk_c
    EXPORT MeshMC_SDKTargets
)

install(EXPORT MeshMC_SDKTargets
    FILE MeshMC_SDKTargets.cmake
    NAMESPACE MeshMC::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/MeshMC_SDK"
)
