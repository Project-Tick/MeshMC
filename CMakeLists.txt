# 3.24 is a hard floor, not a preference: FetchContent_Declare(FIND_PACKAGE_ARGS)
# below is a 3.24 feature, and CMake errors on the unknown keyword rather than
# ignoring it. PROJECT_IS_TOP_LEVEL (3.21) is relied on in libraries/ as well.
cmake_minimum_required(VERSION 3.24)
# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-FileContributor: Project Tick
# SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0

# Required for CMAKE_MSVC_DEBUG_INFORMATION_FORMAT to work (avoids /Zi PDB conflicts with sccache)
cmake_policy(SET CMP0141 NEW)

# Required so CMAKE_MSVC_RUNTIME_LIBRARY controls the CRT (/MD vs /MDd) instead
# of it being baked into CMAKE_CXX_FLAGS. We pin the CRT to /MD when the Rust
# subsystem is enabled (see the MeshMC_RUST block), because the Rust MSVC
# toolchain only links the release CRT. Must be set before project().
cmake_policy(SET CMP0091 NEW)

# Never search our own install prefix when resolving find_package().
#
# The presets install into <source>/install, and project() would otherwise add
# CMAKE_INSTALL_PREFIX to CMAKE_SYSTEM_PREFIX_PATH — so after one
# `cmake --install`, the next configure starts finding our own installed
# artifacts as if they were system packages. That is how a half-installed ECM
# (only ECMConfig.cmake landed under install/share/ECM, none of the modules)
# made find_package(ECM) succeed against a broken prefix and took the whole
# configure down with "could not find ECMUseFindModules.cmake".
#
# Nothing we legitimately depend on lives in our own install tree, so refusing
# to look there removes that entire class of self-poisoning. Must be set before
# project(), which is when CMAKE_SYSTEM_PREFIX_PATH is built.
set(CMAKE_FIND_NO_INSTALL_PREFIX TRUE)

project(MeshMC)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


find_package(ECM NO_MODULE QUIET)
if(NOT ECM_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        ECM
        GIT_REPOSITORY https://invent.kde.org/frameworks/extra-cmake-modules.git
        GIT_TAG        v6.29.0
        GIT_SHALLOW    TRUE
        # ECM is nothing but a pile of .cmake modules — there is no library to
        # build. SOURCE_SUBDIR points at a directory with no CMakeLists.txt,
        # which is the documented way to tell FetchContent_MakeAvailable() to
        # download the sources but skip add_subdirectory().
        #
        # That matters: adding ECM as a subproject pulled its own install()
        # rules and its whole test suite into our build. The install rules
        # dropped a partial ECM into our install prefix, which the *next*
        # configure then found via find_package() and choked on.
        SOURCE_SUBDIR  cmake-modules-only-no-build
    )

    # Belt and braces on top of SOURCE_SUBDIR: keep ECM's own test suite out
    # of our ctest run. It gates on nothing but BUILD_TESTING
    #   option(BUILD_TESTING "Build automated tests." ON)
    #   if(BUILD_TESTING) enable_testing() add_subdirectory(tests) endif()
    # and it is not small — it used to register ~530 tests into our suite,
    # drowning MeshMC's 14. SOURCE_SUBDIR already prevents ECM's CMakeLists
    # from being processed at all, so this changes nothing today; it is here so
    # that building ECM properly can never silently drag its tests back in.
    #
    # The restore is deliberately asymmetric: when BUILD_TESTING was not
    # defined beforehand it is unset again rather than set to "". include(CTest)
    # below does option(BUILD_TESTING "..." ON), and under CMP0077 an
    # already-defined-but-empty normal variable wins over the option default —
    # which would silently switch off MeshMC's own tests. Do not "simplify"
    # this into a plain save/restore.
    set(_meshmc_had_build_testing FALSE)
    if(DEFINED BUILD_TESTING)
        set(_meshmc_had_build_testing TRUE)
        set(_meshmc_saved_build_testing "${BUILD_TESTING}")
    endif()
    set(BUILD_TESTING OFF)

    FetchContent_MakeAvailable(ECM)

    if(_meshmc_had_build_testing)
        set(BUILD_TESTING "${_meshmc_saved_build_testing}")
    else()
        unset(BUILD_TESTING)
    endif()
    unset(_meshmc_had_build_testing)
    unset(_meshmc_saved_build_testing)

    # Mirrors what ECMConfig.cmake would set up (ECM_MODULE_PATH plus the
    # find-modules directory).
    list(APPEND CMAKE_MODULE_PATH
         "${ecm_SOURCE_DIR}/modules"
         "${ecm_SOURCE_DIR}/find-modules")
endif()
set(CMAKE_MODULE_PATH "${ECM_MODULE_PATH};${CMAKE_MODULE_PATH}")

include(CTest)
include(ECMAddTests)
if(BUILD_TESTING)
    enable_testing()
endif()

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BUILD_DIR}" IS_IN_SOURCE_BUILD)
if(IS_IN_SOURCE_BUILD)
    message(FATAL_ERROR "You are building MeshMC in-source. Please separate the build tree from the source tree.")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_HOST_SYSTEM_VERSION MATCHES ".*[Mm]icrosoft.*" OR
        CMAKE_HOST_SYSTEM_VERSION MATCHES ".*WSL.*"
    )
        message(FATAL_ERROR "Building MeshMC is not supported in Linux-on-Windows distributions.")
    endif()
endif()

set(MeshMC_COMPILER_NAME ${CMAKE_CXX_COMPILER_ID})
set(MeshMC_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
set(MeshMC_COMPILER_TARGET_SYSTEM ${CMAKE_SYSTEM_NAME})
set(MeshMC_COMPILER_TARGET_SYSTEM_VERSION ${CMAKE_SYSTEM_VERSION})
set(MeshMC_COMPILER_TARGET_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})

##################################### Set CMake options #####################################
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/;${CMAKE_MODULE_PATH}")

# Output all executables and shared libs in the main build folder, not in subfolders.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
if(UNIX)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()
set(CMAKE_JAVA_TARGET_OUTPUT_DIR ${PROJECT_BINARY_DIR}/jars)

######## Set compiler flags ########
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_C_STANDARD_REQUIRED true)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)

include(GenerateExportHeader)
if(MSVC)
    set(CMAKE_CXX_FLAGS " /W4 ${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DQT_NO_DEPRECATED_WARNINGS=Y")
else()
    # FIXME: readd -Werror flag
    set(CMAKE_CXX_FLAGS " -Wall -pedantic -Wno-deprecated-declarations -fstack-protector-strong --param=ssp-buffer-size=4 -O3 -D_FORTIFY_SOURCE=2 ${CMAKE_CXX_FLAGS}")
    if(UNIX AND APPLE)
        set(CMAKE_CXX_FLAGS " -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
    endif()
    # FIXME: readd this: set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror=return-type")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_DEPRECATED_WARNINGS=Y")
endif()

include(ECMQtDeclareLoggingCategory)

option(ENABLE_LTO "Enable Link Time Optimization" off)
option(MeshMC_DISABLE_JAVA_DOWNLOADER "Disable the built-in Java downloader feature" OFF)
option(MeshMC_ENABLE_CLANG_TIDY "Run clang-tidy during C/C++ compilation" OFF)
option(MeshMC_PLUGINS "Build plugins" OFF)
option(MeshMC_STAGING_PLUGINS "Build Staging plugins" OFF)

find_program(MeshMC_CLANG_TIDY_EXECUTABLE NAMES clang-tidy clang-tidy-19 OPTIONAL)
if(MeshMC_ENABLE_CLANG_TIDY)
    if(MeshMC_CLANG_TIDY_EXECUTABLE)
        set(CMAKE_C_CLANG_TIDY
            "${MeshMC_CLANG_TIDY_EXECUTABLE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
        set(CMAKE_CXX_CLANG_TIDY
            "${MeshMC_CLANG_TIDY_EXECUTABLE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
        message(STATUS "clang-tidy enabled: ${MeshMC_CLANG_TIDY_EXECUTABLE}")
    else()
        message(WARNING "MeshMC_ENABLE_CLANG_TIDY is ON but clang-tidy was not found")
    endif()
endif()

if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
        if(CMAKE_BUILD_TYPE)
            if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
                message(STATUS "IPO / LTO enabled")
            else()
                message(STATUS "Not enabling IPO / LTO on debug builds")
            endif()
        else()
            message(STATUS "IPO / LTO will only be enabled for release builds")
        endif()
    else()
        message(STATUS "IPO / LTO not supported: <${ipo_error}>")
    endif()
endif()

################################ 3rd Party Libs ################################

# Find the required Qt parts
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Concurrent
    Network
    NetworkAuth
    Test
    Xml
    OpenGL
    OpenGLWidgets
)
if(DEFINED Qt6Core_VERSION)
    set(_projt_qt_version "${Qt6Core_VERSION}")
elseif(DEFINED Qt6_VERSION)
    set(_projt_qt_version "${Qt6_VERSION}")
endif()
if(DEFINED _projt_qt_version AND _projt_qt_version VERSION_LESS "6.8.0")
    message(FATAL_ERROR "Qt 6.8.0 or newer is required. Detected: ${_projt_qt_version}")
endif()
unset(_projt_qt_version)

if(UNIX)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_search_module(SCDOC scdoc)
        if(SCDOC_FOUND)
            pkg_get_variable(SCDOC_SCDOC scdoc scdoc)
            message(STATUS "SCDOC_FOUND=${SCDOC_FOUND}")
            message(STATUS "SCDOC_SCDOC=${SCDOC_SCDOC}")
        endif()
    endif()
endif()

include(QMakeQuery)
QUERY_QMAKE(QT_INSTALL_PLUGINS QT_PLUGINS_DIR)
QUERY_QMAKE(QT_INSTALL_LIBS QT_LIBS_DIR)
QUERY_QMAKE(QT_INSTALL_LIBEXECS QT_LIBEXECS_DIR)

# Map missing multi-config configurations to available ones.
# Monorepo libraries are built with single-config (Ninja), but MeshMC uses
# Ninja Multi-Config which validates all configurations at configure time.
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO RelWithDebInfo Release Debug "")
set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL MinSizeRel Release Debug "")

# cmark and toml++ are plain, self-contained upstream libraries with no
# monorepo ties, so they no longer need a pre-built dependency prefix.
#
# FIND_PACKAGE_ARGS makes FetchContent_MakeAvailable() try find_package()
# first and only clone when nothing is installed. That ordering matters:
# distro packaging, Nix and CI build in sandboxes with no network, and they do
# have cmark/toml++ available as system packages. Fetching unconditionally
# would break all three.
#
# Tags are pinned exactly — a floating branch makes builds non-reproducible.
include(FetchContent)

# Deliberately NOT setting CMARK_SHARED / CMARK_STATIC / CMARK_TESTS: cmark
# 0.31.1 removed all three. The library type follows BUILD_SHARED_LIBS (which
# defaults to static there, exactly what we want) and the tests follow
# BUILD_TESTING. Defining the retired options only earns deprecation warnings.
set(CMARK_LIB_FUZZER OFF CACHE BOOL "" FORCE)
FetchContent_Declare(cmark
    GIT_REPOSITORY https://github.com/commonmark/cmark.git
    GIT_TAG        0.31.1
    GIT_SHALLOW    TRUE
    FIND_PACKAGE_ARGS NAMES cmark
)

FetchContent_Declare(tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
    GIT_SHALLOW    TRUE
    FIND_PACKAGE_ARGS NAMES tomlplusplus
)

# Three MeshMC-wide settings are held back while these are configured:
#
#   BUILD_TESTING   cmark's api_test/test dirs need Python and would register
#                   their tests in our CTest suite. It is read with a bare
#                   if(), so a plain variable is enough and MeshMC's own tests
#                   (which need BUILD_TESTING ON) are unaffected.
#   CMAKE_C_STANDARD  cmark explicitly does `if(NOT MSVC) set(CMAKE_C_STANDARD 99)`
#                   — on MSVC it wants the compiler default. Forcing C23 on a
#                   vendored C library overrides upstream's deliberate choice
#                   and invalidates the configuration they test against. See
#                   the longer note on this in the zlib-ng/libarchive block.
#   CMAKE_AUTOMOC   moc has no business running over C sources.
set(_meshmc_saved_build_testing "${BUILD_TESTING}")
set(_meshmc_saved_c_standard "${CMAKE_C_STANDARD}")
set(_meshmc_saved_c_standard_required "${CMAKE_C_STANDARD_REQUIRED}")
set(_meshmc_saved_automoc "${CMAKE_AUTOMOC}")
set(BUILD_TESTING OFF)
unset(CMAKE_C_STANDARD)
unset(CMAKE_C_STANDARD_REQUIRED)
set(CMAKE_AUTOMOC OFF)

FetchContent_MakeAvailable(cmark tomlplusplus)

set(BUILD_TESTING "${_meshmc_saved_build_testing}")
set(CMAKE_C_STANDARD "${_meshmc_saved_c_standard}")
set(CMAKE_C_STANDARD_REQUIRED "${_meshmc_saved_c_standard_required}")
set(CMAKE_AUTOMOC "${_meshmc_saved_automoc}")
unset(_meshmc_saved_build_testing)
unset(_meshmc_saved_c_standard)
unset(_meshmc_saved_c_standard_required)
unset(_meshmc_saved_automoc)

# Normalise the cmark target name so the rest of the build can just link
# cmark::cmark, whether it came from find_package() or from the clone above.
#
# The in-tree library target is plain `cmark`; the namespaced name only exists
# in cmark's install(EXPORT). Do NOT reach for `cmark_static`: in 0.31.1 that
# name is an add_custom_target() shim kept for OSS-Fuzz, so aliasing it fails
# with "target cmark_static is not a library". Hence checking the target TYPE
# instead of merely whether the name exists.
if(NOT TARGET cmark::cmark)
    if(NOT TARGET cmark)
        message(FATAL_ERROR "cmark is available but exposes no 'cmark' target")
    endif()
    get_target_property(_meshmc_cmark_type cmark TYPE)
    if(NOT _meshmc_cmark_type MATCHES "_LIBRARY$")
        message(FATAL_ERROR
            "cmark target 'cmark' is a ${_meshmc_cmark_type}, not a library")
    endif()
    add_library(cmark::cmark ALIAS cmark)
    unset(_meshmc_cmark_type)
endif()

# GpgME (C++ bindings) — used by the MMCO plugin signature verifier.
# KDE's gpgmepp distribution exports the bare target name "Gpgmepp".
#
# Upstream gpgme does not support MSVC (its build system is autotools
# and the C++ bindings are not ABI-compatible with MSVC), so the plugin
# signature verifier compiles into a stub on MSVC builds: the launcher
# still loads OSS plugins and rejects non-OSS plugins on signature
# grounds, it just cannot actually verify trailers. Linux and native
# (single-arch) macOS / MinGW Windows builds get the full GpgME-backed
# verifier.
#
# macOS universal builds (CMAKE_OSX_ARCHITECTURES contains both
# "x86_64" and "arm64") get the stub too. The reason is purely a
# distribution-side limitation: Homebrew ships gpgme/gpgmepp dylibs
# for the host architecture only — on an Apple Silicon CI runner
# /opt/homebrew/lib/libgpgmepp.dylib is arm64-only, so a universal
# binary fails to link the x86_64 slice. Notarized macOS releases
# get plugin trust through Apple codesigning + Sparkle anyway, so
# the GPG verifier is not a useful second layer there.
if(MSVC)
    set(_meshmc_default_plugin_signatures OFF)
elseif(APPLE)
    # Detect a universal build: CMAKE_OSX_ARCHITECTURES is a list, and
    # a "universal" build is any list with more than one architecture.
    list(LENGTH CMAKE_OSX_ARCHITECTURES _meshmc_osx_arch_count)
    if(_meshmc_osx_arch_count GREATER 1)
        set(_meshmc_default_plugin_signatures OFF)
        message(STATUS
            "macOS universal build detected (CMAKE_OSX_ARCHITECTURES="
            "${CMAKE_OSX_ARCHITECTURES}). "
            "Defaulting MeshMC_PLUGIN_SIGNATURES=OFF because Homebrew "
            "gpgmepp only ships single-arch dylibs. Override with "
            "-DMeshMC_PLUGIN_SIGNATURES=ON if you have a fat gpgmepp.")
    else()
        set(_meshmc_default_plugin_signatures ON)
    endif()
    unset(_meshmc_osx_arch_count)
else()
    set(_meshmc_default_plugin_signatures ON)
endif()
option(MeshMC_PLUGIN_SIGNATURES
    "Enable GPG-backed plugin signature verification (requires Gpgmepp)"
    ${_meshmc_default_plugin_signatures})

if(MeshMC_PLUGIN_SIGNATURES)
    find_package(Gpgmepp REQUIRED)
    if(TARGET Gpgmepp)
        # Upstream only ships an unconfigured ("NOCONFIG") import, which
        # trips the Ninja Multi-Config generator with errors like:
        #   IMPORTED_LOCATION not set for imported target "Gpgmepp"
        #   configuration "RelWithDebInfo"
        # Replicate the NOCONFIG IMPORTED_LOCATION / IMPORTED_SONAME on
        # every build configuration we use.
        get_target_property(_gpgmepp_loc Gpgmepp IMPORTED_LOCATION_NOCONFIG)
        get_target_property(_gpgmepp_son Gpgmepp IMPORTED_SONAME_NOCONFIG)
        if(_gpgmepp_loc)
            foreach(_cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
                set_property(TARGET Gpgmepp APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS ${_cfg})
                set_target_properties(Gpgmepp PROPERTIES
                    IMPORTED_LOCATION_${_cfg} "${_gpgmepp_loc}")
                if(_gpgmepp_son)
                    set_target_properties(Gpgmepp PROPERTIES
                        IMPORTED_SONAME_${_cfg} "${_gpgmepp_son}")
                endif()
            endforeach()
        endif()
    endif()
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

################################ Rust subsystem ################################
option(MeshMC_RUST "Build and embed the Rust subsystem (always ON for release builds)" ON)

if(MeshMC_RUST AND MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING
        "Per-config dynamic CRT (/MDd in Debug, /MD otherwise) to match meshmc-deps." FORCE)
endif()

if(MeshMC_RUST)
    find_program(MeshMC_CARGO_EXECUTABLE NAMES cargo)
    find_program(MeshMC_RUSTC_EXECUTABLE NAMES rustc)
    if(NOT MeshMC_CARGO_EXECUTABLE OR NOT MeshMC_RUSTC_EXECUTABLE)
        message(FATAL_ERROR
            "MeshMC_RUST is ON but a Rust toolchain was not found.\n"
            "  cargo: ${MeshMC_CARGO_EXECUTABLE}\n"
            "  rustc: ${MeshMC_RUSTC_EXECUTABLE}\n"
            "The Rust subsystem (feature flags, launch) is a hard build "
            "requirement.\n"
            "Install Rust via https://rustup.rs and make sure cargo/rustc are "
            "on PATH (e.g. add %USERPROFILE%\\.cargo\\bin), then reconfigure.\n"
            "If you really must build without it, pass -DMeshMC_RUST=OFF "
            "(unsupported).")
    endif()
    message(STATUS "Rust toolchain found: ${MeshMC_CARGO_EXECUTABLE}")

    find_package(Corrosion QUIET)
    if(NOT Corrosion_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            Corrosion
            GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
            GIT_TAG v0.5.1
        )
        FetchContent_MakeAvailable(Corrosion)
    endif()

    corrosion_import_crate(MANIFEST_PATH "${CMAKE_SOURCE_DIR}/Cargo.toml")

    if(NOT TARGET meshmc_featureflags)
        message(FATAL_ERROR
            "Corrosion did not create the expected 'meshmc_featureflags' "
            "target. Check that crates/meshmc-featureflags is a member of the "
            "workspace Cargo.toml and that its package name matches.")
    endif()

    if(MSVC)
        corrosion_set_env_vars(meshmc_featureflags
            "CFLAGS=$<IF:$<CONFIG:Debug>,/MDd,/MD>"
            "CXXFLAGS=$<IF:$<CONFIG:Debug>,/MDd,/MD>")
        set(_ff_target_triple "${Rust_CARGO_TARGET_CACHED}")
        if(NOT _ff_target_triple)
            set(_ff_target_triple "${Rust_CARGO_TARGET}")
        endif()
        set(_ff_host_triple "${Rust_CARGO_HOST_TARGET_CACHED}")
        if(NOT _ff_host_triple)
            set(_ff_host_triple "${Rust_CARGO_HOST_TARGET}")
        endif()

        if(CMAKE_CXX_COMPILER AND _ff_target_triple)
            get_filename_component(_ff_msvc_bindir "${CMAKE_CXX_COMPILER}" DIRECTORY)
            set(_ff_msvc_link "${_ff_msvc_bindir}/link.exe")
            if(EXISTS "${_ff_msvc_link}")
                # TOML wants forward slashes; normalize.
                file(TO_CMAKE_PATH "${_ff_msvc_link}" _ff_msvc_link_toml)
                set(_ff_cargo_cfg
                    "# Generated by MeshMC CMake — do not edit.\n"
                    "# Pins the MSVC linker by absolute path so a GNU `link`\n"
                    "# shadowing it on PATH (Git Bash) cannot be picked up by\n"
                    "# rustc for host build scripts, proc-macros, or the lib.\n")
                string(APPEND _ff_cargo_cfg
                    "[target.${_ff_target_triple}]\nlinker = \"${_ff_msvc_link_toml}\"\n")
                if(_ff_host_triple AND NOT (_ff_host_triple STREQUAL _ff_target_triple))
                    string(APPEND _ff_cargo_cfg
                        "[target.${_ff_host_triple}]\nlinker = \"${_ff_msvc_link_toml}\"\n")
                endif()
                file(WRITE
                    "${CMAKE_SOURCE_DIR}/launcher/featureflags/.cargo/config.toml"
                    ${_ff_cargo_cfg})
            endif()
        endif()
    endif()

    # The cxx C++ glue is generated and compiled *inside* the Rust staticlib by
    # the crate's build.rs (cxx-build), not by Corrosion's cxxbridge helper. The
    # build script copies the generated headers to a stable location next to the
    # sources: launcher/featureflags/generated/{meshmc_featureflags/lib.rs.h,
    # rust/cxx.h}. Expose that directory to the C++ consumers so
    # `#include "meshmc_featureflags/lib.rs.h"` resolves.
    set(MeshMC_FEATUREFLAGS_GENERATED_INCLUDE
        "${CMAKE_SOURCE_DIR}/launcher/featureflags/generated"
        CACHE INTERNAL "Include dir for cxx-generated feature-flag bridge headers")
    # Create the directory now so the include path exists at configure time;
    # build.rs populates it with the real headers during the cargo build.
    file(MAKE_DIRECTORY "${MeshMC_FEATUREFLAGS_GENERATED_INCLUDE}/meshmc_featureflags")
    file(MAKE_DIRECTORY "${MeshMC_FEATUREFLAGS_GENERATED_INCLUDE}/rust")

    if(MSVC AND TARGET meshmc_featureflags)
        # Rust links the release CRT (msvcrt) unconditionally; in Debug the rest
        # of the binary uses msvcrtd. Drop Rust's release-CRT default in Debug so
        # the (superset) debug CRT satisfies its symbols. No-op elsewhere.
        target_link_options(meshmc_featureflags INTERFACE
            "$<$<CONFIG:Debug>:LINKER:/NODEFAULTLIB:msvcrt>")
    endif()

endif()

####################################### Branding #######################################

######## Firstly, dont touch here, this section calculate Git commit, branch; Timestamp
######## and initialize targets for print version sections.

#### Check the current Git commit and branch
include(GetGitRevisionDescription)
get_git_head_revision(MeshMC_GIT_REFSPEC MeshMC_GIT_COMMIT ALLOW_LOOKING_ABOVE_CMAKE_SOURCE_DIR)
git_get_exact_tag(MeshMC_GIT_TAG)

message(STATUS "Git commit: ${MeshMC_GIT_COMMIT}")
message(STATUS "Git refspec: ${MeshMC_GIT_REFSPEC}")
message(STATUS "Git tag: ${MeshMC_GIT_TAG}")

string(TIMESTAMP TODAY "%Y-%m-%d")
set(MeshMC_BUILD_TIMESTAMP "${TODAY}")

#### Custom target to just print the version.
add_custom_target(version echo "Version: ${MeshMC_RELEASE_VERSION_NAME}")
add_custom_target(tcversion echo "\\#\\#teamcity[setParameter name=\\'env.MESHMC_VERSION\\' value=\\'${MeshMC_RELEASE_VERSION_NAME}\\']")

######## Secondly, please set version numbers ########
set(MeshMC_VERSION_MAJOR    8)
set(MeshMC_VERSION_MINOR    2)
set(MeshMC_VERSION_HOTFIX   0)

set(MeshMC_RELEASE_VERSION_NAME "${MeshMC_VERSION_MAJOR}.${MeshMC_VERSION_MINOR}.${MeshMC_VERSION_HOTFIX}")
set(MeshMC_RELEASE_TWO_NAME "${MeshMC_VERSION_MAJOR}.${MeshMC_VERSION_MINOR}")
set(MeshMC_VERSION_NAME4 "${MeshMC_VERSION_MAJOR}.${MeshMC_VERSION_MINOR}.${MeshMC_VERSION_HOTFIX}.0")
set(MeshMC_VERSION_NAME4_COMMA "${MeshMC_VERSION_MAJOR},${MeshMC_VERSION_MINOR},${MeshMC_VERSION_HOTFIX},0")

######## Thirdly, please define URLs and API keys ########

set(MeshMC_NEWS_RSS_URL "https://projecttick.org/product/meshmc/feed.xml" CACHE STRING "URL to fetch MeshMC's news RSS feed from.")
set(MeshMC_NEWS_EXTRA_FEEDS "https://projecttick.org/feed.xml;" CACHE STRING "Semicolon-separated list of extra RSS feed URLs for the NewsViewer plugin.")

# Build number
set(MeshMC_VERSION_BUILD -1 CACHE STRING "Build number. -1 for no build number.")

# Legacy substring artifact name (e.g. "MeshMC-Linux-Portable"). Used as a
# last-resort fallback when the product feed's assets do not carry the new
# structured attributes. Prefer setting the four structured cache vars below
# instead.
set(MeshMC_BUILD_ARTIFACT "" CACHE STRING "Legacy substring used to match a feed asset when structured attributes are unavailable.")

# Structured build identity. Matched directly against the feed asset's
# `platform`, `arch`, `portable` and `kind` attributes.
set(MeshMC_BUILD_PLATFORM_ID "" CACHE STRING "Updater asset platform id: linux | windows | macos.")
set(MeshMC_BUILD_ARCH        "" CACHE STRING "Updater asset arch: x86_64 | aarch64.")
set(MeshMC_BUILD_PORTABLE    "" CACHE STRING "Updater asset portable flag: true | false.")
set(MeshMC_BUILD_KIND        "" CACHE STRING "Updater asset kind: archive | appimage | installer.")

# Build platform.
set(MeshMC_BUILD_PLATFORM "" CACHE STRING "A short string identifying the platform that this build was built for. Only used by the notification system and to display in the about dialog.")

# Channel list URL (legacy - kept for compatibility)
set(MeshMC_UPDATER_BASE "https://projecttick.org/product/meshmc/feed.xml" CACHE STRING "Base URL for the legacy GoUpdate system (unused).")

# Updater: authoritative product feed URL (projt: namespace).
set(MeshMC_UPDATER_FEED_URL "https://projecttick.org/product/meshmc/feed.xml" CACHE STRING "RSS product feed URL for the updater (projt: namespace).")

# Updater: latest.json mirror URL (used as a cross-check; empty disables it).
set(MeshMC_UPDATER_LATEST_JSON_URL "https://ftp.projecttick.org/Project-Tick/latest.json" CACHE STRING "Project Tick latest.json mirror used as a sanity check against the feed.")

# Notification URL
set(MeshMC_NOTIFICATION_URL "https://projecttick.org/" CACHE STRING "URL for checking for notifications.")

# The metadata server
set(MeshMC_META_URL "https://meta.projecttick.org/" CACHE STRING "URL to fetch MeshMC's meta files from.")

# Microsoft Client ID
set(MeshMC_MICROSOFT_CLIENT_ID "3035382c-8f73-493a-b579-d182905c2864")

# paste.ee API key
set(MeshMC_PASTE_EE_API_KEY "ay2miY3lWxUHEEzYucj5e4YShIuo1aaY43MfwsUAe" CACHE STRING "API key you can get from paste.ee when you register an account")

# Imgur API Client ID
set(MeshMC_IMGUR_CLIENT_ID "5b97b0713fba4a3" CACHE STRING "Client ID you can get from Imgur when you register an application")

# CurseForge API key
set(MeshMC_CURSEFORGE_API_KEY "$2a$10$S7KcKijbCj8mCHUQcn0tgOmtHg0kA8q9FI0niNJJ7knPq0INomzrG" CACHE STRING "API key for the CurseForge API (get one at https://console.curseforge.com/)")

# Google analytics ID
set(MeshMC_ANALYTICS_ID "G-91XWT2Q4LY" CACHE STRING "ID you can get from Google analytics")
set(MeshMC_ANALYTICS_SECRET "_EG4lDmVT-S1oGiOIwvzPA" CACHE STRING "API secret for Google Analytics 4 Measurement Protocol")

# Bug tracker URL
set(MeshMC_BUG_TRACKER_URL "https://github.com/Project-Tick/MeshMC/issues" CACHE STRING "URL for the bug tracker.")

# Discord URL
set(MeshMC_DISCORD_URL "" CACHE STRING "URL for the Discord guild.")

# Subreddit URL
set(MeshMC_SUBREDDIT_URL "" CACHE STRING "URL for the subreddit.")

# Feature flags
set(MeshMC_UNLEASH_URL "" CACHE STRING "GitLab Unleash-compatible feature flags endpoint.")
set(MeshMC_UNLEASH_INSTANCE_ID "" CACHE STRING "Unleash instance id. Set this to enable runtime feature flags.")
set(MeshMC_UNLEASH_APP_NAME "" CACHE STRING "Unleash application name / environment (UNLEASH-APPNAME). Must match the flag's environment spec, e.g. 'production'.")

######## Fourthly, please define MeshMC attributes ########

set(MeshMC_CommonName "MeshMC")
set(MeshMC_AppBinaryName "meshmc")
set(MeshMC_AppID "org.projecttick.MeshMC")
set(MeshMC_Copyright "Project Tick")
set(MeshMC_Copyright_Mac "© 2026 Project Tick")
set(MeshMC_Domain "projecttick.org")
set(MeshMC_Name "${MeshMC_CommonName}")
set(MeshMC_DisplayName "${MeshMC_CommonName}")
set(MeshMC_UserAgent "${MeshMC_CommonName}/${MeshMC_RELEASE_TWO_NAME}")
set(MeshMC_ConfigFile "meshmc.cfg")
set(MeshMC_Git "https://github.com/Project-Tick/MeshMC")
set(MeshMC_Authors "Copyright (C) 2026 Project Tick <https://projecttick.org>, All rights reserved. This Project Licensed under GNU GENERAL PUBLIC LICENSE, either version 3 of the License, or any later version. ")

######## Lastly, configure placeholders

set(MeshMC_SVGFileName "${MeshMC_AppID}.svg")
set(MeshMC_Branding_ICNS "branding/${MeshMC_AppID}.icns")
set(MeshMC_Branding_ICO "${MeshMC_AppID}.ico")
set(MeshMC_Branding_WindowsRC "branding/${MeshMC_AppBinaryName}.rc")
set(MeshMC_Branding_LogoQRC "branding/${MeshMC_AppBinaryName}.qrc")

configure_file(branding/${MeshMC_AppID}.desktop.in ${MeshMC_AppID}.desktop)

# The AppStream <releases> block in the metainfo file is generated from
# CHANGELOG.md (the single source of truth) at configure time, so the release
# history is never maintained by hand in two places. This is done in pure CMake
# (no Python or shell dependency) so it behaves identically across GitLab CI,
# the Flatpak/KDE SDK sandbox, the Nix sandbox, vcpkg/Windows and macOS.
# Re-run configure whenever the changelog changes.
set(MeshMC_Changelog "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MeshMC_Changelog}")

include(GenerateMetainfoReleases)
meshmc_generate_metainfo_releases(
    CHANGELOG "${MeshMC_Changelog}"
    OUTPUT_VARIABLE MeshMC_METAINFO_RELEASES)

configure_file(branding/${MeshMC_AppID}.metainfo.xml.in ${MeshMC_AppID}.metainfo.xml @ONLY)
configure_file(branding/${MeshMC_AppBinaryName}.rc.in ${MeshMC_AppBinaryName}.rc @ONLY)
configure_file(branding/${MeshMC_AppBinaryName}.qrc.in ${MeshMC_AppBinaryName}.qrc @ONLY)
configure_file(branding/${MeshMC_CommonName}.manifest.in ${MeshMC_AppBinaryName}.manifest @ONLY)
configure_file(branding/${MeshMC_AppID}.mime.xml ${MeshMC_AppID}.xml COPYONLY)
configure_file(branding/${MeshMC_SVGFileName} ${MeshMC_SVGFileName} COPYONLY)
configure_file(branding/${MeshMC_AppID}.ico ${MeshMC_AppID}.ico COPYONLY)
configure_file(branding/${MeshMC_AppBinaryName}.6.scd.in ${MeshMC_AppBinaryName}.6.scd @ONLY)
set(MeshMC_Portable_File "branding/portable.txt")
set(MeshMC_Branding_MAC_ICON "branding/${MeshMC_CommonName}.icon")

if(SCDOC_FOUND)
    set(in_scd "${CMAKE_CURRENT_BINARY_DIR}/${MeshMC_AppBinaryName}.6.scd")
    set(out_man "${CMAKE_CURRENT_BINARY_DIR}/${MeshMC_AppBinaryName}.6")
    add_custom_command(
        DEPENDS "${in_scd}"
        OUTPUT "${out_man}"
        COMMAND ${SCDOC_SCDOC} < "${in_scd}" > "${out_man}"
    )
    add_custom_target(man ALL DEPENDS ${out_man})
    set(MeshMC_ManPage "${CMAKE_CURRENT_BINARY_DIR}/${MeshMC_AppBinaryName}.6")
endif()

if(MSVC)
    set(MeshMC_MSVC_Redist_NSIS_Section [=[
!ifdef haveNScurl
Section "Visual Studio Runtime"
    Var /GLOBAL vc_redist_exe
    ${If} ${IsNativeARM64}
        StrCpy $vc_redist_exe "vc_redist.arm64.exe"
    ${Else}
        StrCpy $vc_redist_exe "vc_redist.x64.exe"
    ${EndIf}
    DetailPrint 'Downloading Microsoft Visual C++ Redistributable...'
    NScurl::http GET "https://aka.ms/vs/17/release/$vc_redist_exe" "$INSTDIR\vc_redist\$vc_redist_exe" /INSIST /CANCEL /Zone.Identifier /END
    Pop $0
    ${If} $0 == "OK"
        DetailPrint "Download successful"
        ExecWait "$INSTDIR\vc_redist\$vc_redist_exe /install /passive /norestart"
    ${Else}
        DetailPrint "Download failed with error $0"
    ${EndIf}
SectionEnd
!endif
]=])
endif()

configure_file(branding/win_install.nsi.in win_install.nsi @ONLY)

####################################### Install layout #######################################

if(NOT (UNIX AND APPLE))
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${MeshMC_Portable_File}" DESTINATION "." COMPONENT portable EXCLUDE_FROM_ALL)
endif()

if(UNIX AND APPLE)
    set(BINARY_DEST_DIR "${MeshMC_Name}.app/Contents/MacOS")
    set(FRAMEWORK_DEST_DIR "${MeshMC_Name}.app/Contents/Frameworks")
    set(LIBRARY_DEST_DIR "${FRAMEWORK_DEST_DIR}")
    set(PLUGIN_DEST_DIR "${MeshMC_Name}.app/Contents/MacOS")
    set(RESOURCES_DEST_DIR "${MeshMC_Name}.app/Contents/Resources")
    # Java archives (JavaCheck.jar / NewLaunch.jar) MUST NOT live
    # under Contents/MacOS. Apple's signing tooling reserves that
    # directory exclusively for the main executable and its Mach-O
    # helpers; every other file it finds there is treated as a
    # subcomponent of the main executable and gets seal-hashed in
    # the strict-validation pass. The result, observed in CI:
    #     "code object is not signed at all
    #      In subcomponent: .../Contents/MacOS/jars/JavaCheck.jar"
    # The bundle anatomy guide explicitly puts non-code data under
    # Contents/Resources/, which is what we do. Application::
    # getJarsPath() has a matching macOS branch.
    set(JARS_DEST_DIR "${MeshMC_Name}.app/Contents/Resources/jars")
    set(BUNDLE_DEST_DIR ".")
    # .mmco loadable bundles on macOS live under
    #   MeshMC.app/Contents/Resources/mmcmodules
    #
    # Why not Contents/MacOS/:
    #   Apple's codesign reserves Contents/MacOS/ for the main
    #   executable + Mach-O helpers; every other file there is
    #   treated as an unsigned subcomponent and aborts strict
    #   validation.
    #
    # Why not Contents/PlugIns/ (Apple's "official" plug-in dir):
    #   codesign --strict walks Contents/PlugIns/ recursively and
    #   demands every Mach-O carry a valid Apple LC_CODE_SIGNATURE
    #   covering the whole file. Our .mmco files have a custom RSA
    #   trailer (scripts/mmco_sign.py) appended past __LINKEDIT,
    #   which Apple's strict mode rejects as
    #     "main executable failed strict validation
    #      In subcomponent: .../mmcmodules/Foo.mmco"
    #   even when the Mach-O image was correctly signed before the
    #   trailer was reattached. The two signing schemes are
    #   physically incompatible inside the same file.
    #
    # Why Contents/Resources/ works:
    #   codesign treats Resources/ as opaque data — every file is
    #   hashed verbatim into _CodeSignature/CodeResources but its
    #   bytes are never parsed as code. Trailer-signed Mach-Os
    #   live there without tripping strict mode. dlopen() against
    #   them still works (Apple's loader doesn't care which bundle
    #   directory a library is loaded from at runtime, only that
    #   codesign was able to seal it).
    #
    # PluginLoader::defaultSearchPaths has the matching macOS
    # branch that points at Contents/Resources/mmcmodules.
    set(MMCO_MODULES_DEST_DIR
        "${MeshMC_Name}.app/Contents/Resources/mmcmodules")

    # Plugin-shipped DATA files (rule packs, templates, ...) MUST
    # NOT live next to the .mmco under Contents/PlugIns/. Apple's
    # strict-signing pass walks Contents/PlugIns/ recursively and
    # treats every non-code file it finds as a subcomponent of the
    # main executable, demanding it be signed. We observed
    #     "code object is not signed at all
    #      In subcomponent: .../PlugIns/mmcmodules/ErrorOracle/rules/jvm.json"
    # in CI before this fix. The cure is to push plugin data into
    # Contents/Resources/ — codesign hashes those files via
    # CodeResources without trying to interpret them as nested
    # code. Each plugin gets its own subdirectory under
    # MMCOPluginData/ so namespaces don't collide.
    # Runtime path resolution lives in the individual plugins
    # (e.g. ErrorOraclePlugin::builtInRulesDir).
    set(MMCO_PLUGIN_DATA_DEST_DIR
        "${MeshMC_Name}.app/Contents/Resources/MMCOPluginData")

    # Mac bundle settings
    set(MACOSX_BUNDLE_BUNDLE_NAME "${MeshMC_DisplayName}")
    set(MACOSX_BUNDLE_INFO_STRING "${MeshMC_DisplayName}: A custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once.")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "${MeshMC_AppID}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${MeshMC_RELEASE_VERSION_NAME}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${MeshMC_RELEASE_VERSION_NAME}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${MeshMC_RELEASE_VERSION_NAME}")
    set(MACOSX_BUNDLE_ICON_FILE ${MeshMC_AppID}.icns)
    set(MACOSX_BUNDLE_COPYRIGHT "${MeshMC_Copyright_Mac}")
    set(MACOSX_SPARKLE_UPDATE_PUBLIC_KEY "C0eBoyDSoZbzgCMxQH9wH6kmjU2mPRmvhZZd9mHgqZQ=" CACHE STRING "Public key for Sparkle update feed")
    set(MACOSX_SPARKLE_UPDATE_FEED_URL "https://projecttick.org/product/meshmc/appcast.xml" CACHE STRING "URL for Sparkle update feed")

    set(MACOSX_SPARKLE_DOWNLOAD_URL "https://github.com/sparkle-project/Sparkle/releases/download/2.8.0/Sparkle-2.8.0.tar.xz" CACHE STRING "URL to Sparkle release archive")
    set(MACOSX_SPARKLE_SHA256 "fd5681ee92bf238aaac2d08214ceaf0cc8976e452d7f882d80bac1e61581f3b1" CACHE STRING "SHA256 checksum for Sparkle release archive")
    set(MACOSX_SPARKLE_DIR "${CMAKE_BINARY_DIR}/frameworks/Sparkle")

    # Add the icon
    install(FILES ${MeshMC_Branding_ICNS} DESTINATION ${RESOURCES_DEST_DIR} RENAME ${MeshMC_AppID}.icns)

    find_program(ACTOOL_EXE actool DOC "Path to the apple asset catalog compiler")
    if(ACTOOL_EXE)
        execute_process(
            COMMAND xcodebuild -version
            OUTPUT_VARIABLE XCODE_VERSION_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        string(REGEX MATCH "Xcode ([0-9]+\.[0-9]+)" XCODE_VERSION_MATCH "${XCODE_VERSION_OUTPUT}")
        if(XCODE_VERSION_MATCH)
            set(XCODE_VERSION ${CMAKE_MATCH_1})
        else()
            set(XCODE_VERSION 0.0)
        endif()

        if(XCODE_VERSION VERSION_GREATER_EQUAL 26.0)
            set(ASSETS_OUT_DIR "${CMAKE_BINARY_DIR}/branding")
            set(GENERATED_ASSETS_CAR "${ASSETS_OUT_DIR}/Assets.car")
            set(ICON_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${MeshMC_Branding_MAC_ICON}")

            add_custom_command(
                OUTPUT "${GENERATED_ASSETS_CAR}"
                COMMAND ${ACTOOL_EXE} "${ICON_SOURCE}"
                        --compile "${ASSETS_OUT_DIR}"
                        --output-partial-info-plist /dev/null
                        --app-icon ${MeshMC_Name}
                        --enable-on-demand-resources NO
                        --target-device mac
                        --minimum-deployment-target ${CMAKE_OSX_DEPLOYMENT_TARGET}
                        --platform macosx
                DEPENDS "${ICON_SOURCE}"
                COMMENT "Compiling asset catalog (${ICON_SOURCE})"
                VERBATIM
            )
            add_custom_target(compile_assets ALL DEPENDS "${GENERATED_ASSETS_CAR}")
            install(FILES "${GENERATED_ASSETS_CAR}" DESTINATION "${RESOURCES_DEST_DIR}")
        else()
            message(WARNING "Xcode ${XCODE_VERSION} is too old. Minimum required version is 26.0. Not compiling liquid glass icons.")
        endif()

    else()
        message(WARNING "actool not found. Cannot compile macOS app icons.\n"
                "Install Xcode command line tools: 'xcode-select --install'")
    endif()


elseif(UNIX)
    set(BINARY_DEST_DIR "bin")
    set(LIBRARY_DEST_DIR "lib${LIB_SUFFIX}")
    set(JARS_DEST_DIR "share/${MeshMC_Name}")
    # On Linux .mmco loadable bundles continue to live next to the
    # launcher binary, matching the pre-existing layout (and the
    # search paths in PluginLoader::defaultSearchPaths). Exposed as
    # a named variable so plugin CMakeLists files can reference one
    # symbol regardless of platform.
    set(MMCO_MODULES_DEST_DIR "${BINARY_DEST_DIR}/mmcmodules")
    # On Linux there is no codesign machinery walking PlugIns/, so
    # plugin data can keep living next to the .mmco. We still expose
    # the variable so plugin CMakeLists files can write
    #   install(... DESTINATION ${MMCO_PLUGIN_DATA_DEST_DIR}/<Name> ...)
    # uniformly across platforms.
    set(MMCO_PLUGIN_DATA_DEST_DIR "${MMCO_MODULES_DEST_DIR}")

    # Set RPATH
    SET(MeshMC_BINARY_RPATH "$ORIGIN/../lib${LIB_SUFFIX}")

    # Standard FreeDesktop install paths (replaces KDEInstallDirs to avoid
    # conflicts with GNUInstallDirs already loaded by find_package(Qt6)).
    include(GNUInstallDirs)
    set(KDE_INSTALL_APPDIR      "${CMAKE_INSTALL_DATAROOTDIR}/applications")
    set(KDE_INSTALL_METAINFODIR "${CMAKE_INSTALL_DATAROOTDIR}/metainfo")
    set(KDE_INSTALL_ICONDIR     "${CMAKE_INSTALL_DATAROOTDIR}/icons")
    set(KDE_INSTALL_MIMEDIR     "${CMAKE_INSTALL_DATAROOTDIR}/mime/packages")
    set(KDE_INSTALL_MANDIR      "${CMAKE_INSTALL_MANDIR}")

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MeshMC_AppID}.desktop DESTINATION ${KDE_INSTALL_APPDIR})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MeshMC_AppID}.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/branding/${MeshMC_AppID}.svg DESTINATION "${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps")
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/branding/${MeshMC_AppID}_256.png DESTINATION "${KDE_INSTALL_ICONDIR}/hicolor/256x256/apps" RENAME "${MeshMC_AppID}.png")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MeshMC_AppID}.xml DESTINATION ${KDE_INSTALL_MIMEDIR})

    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/launcher/qtlogging.ini" DESTINATION "share/${MeshMC_Name}")

    set(PLUGIN_DEST_DIR "plugins")
    set(BUNDLE_DEST_DIR ".")
    set(RESOURCES_DEST_DIR ".")

    if(MeshMC_ManPage)
        install(FILES ${MeshMC_ManPage} DESTINATION "${KDE_INSTALL_MANDIR}/man6")
    endif()

    # Install basic runner script if component "portable" is selected
    configure_file(launcher/MeshMC.in "${CMAKE_CURRENT_BINARY_DIR}/MeshMCScript" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/MeshMCScript" DESTINATION "." RENAME ${MeshMC_Name} COMPONENT portable EXCLUDE_FROM_ALL)

elseif(WIN32)
    set(BINARY_DEST_DIR ".")
    set(LIBRARY_DEST_DIR ".")
    set(PLUGIN_DEST_DIR ".")
    set(FRAMEWORK_DEST_DIR ".")
    set(RESOURCES_DEST_DIR ".")
    set(JARS_DEST_DIR "jars")
    # Windows keeps the flat layout it has always used: .mmco files
    # next to meshmc.exe under <install>/mmcmodules. Variable exists
    # so plugin CMakeLists files can refer to it uniformly.
    set(MMCO_MODULES_DEST_DIR "${BINARY_DEST_DIR}/mmcmodules")
    # Windows has no codesign-style strict mode that distinguishes
    # code from data inside the plugin directory, so plugin data
    # can stay alongside the .mmco. Variable mirrors the macOS one
    # for cross-platform plugin CMakeLists.
    set(MMCO_PLUGIN_DATA_DEST_DIR "${MMCO_MODULES_DEST_DIR}")
else()
    message(FATAL_ERROR "Platform not supported")
endif()

################################ Included Libs ################################

include(ExternalProject)
set_directory_properties(PROPERTIES EP_BASE External)

# Everything under libraries/ is built as part of this build tree, statically,
# unless the library hardcodes SHARED (iconfix, rainbow) or opts out itself
# (nbt++, via NBT_BUILD_SHARED). This matches what the old pre-built
# dependency prefix shipped, so linkage does not change — only where the
# objects come from.
#
# This is a cache entry rather than a plain variable on purpose: libarchive
# declares cmake_minimum_required(VERSION 2.8.12), which puts CMP0077 in OLD
# mode for its scope, and there option() ignores normal variables.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build monorepo libraries as shared libraries" FORCE)

# libarchive's cmake_minimum_required is older than CMake 4 accepts.
set(CMAKE_POLICY_VERSION_MINIMUM "3.5")

# ----------------------------------------------------------------------------
# Vendored C libraries are built the way upstream tests them, not the way
# MeshMC builds its own code. Two settings are held back:
#
# CMAKE_C_STANDARD 23 — neither library asks for C23, and forcing it overrides
#   a deliberate upstream decision: zlib-ng does
#   `if(NOT DEFINED CMAKE_C_STANDARD) set(CMAKE_C_STANDARD 11)` and cmark does
#   `if(NOT MSVC) set(CMAKE_C_STANDARD 99)` — i.e. on MSVC it specifically
#   wants the compiler default. Feature detection results depend on the C
#   dialect, so building vendored code in a dialect upstream never tested it
#   in is asking for subtly wrong probe results.
#
# CMAKE_AUTOMOC — Qt's moc has no business running over C sources. Left on, it
#   generates a mocs_compilation.cpp for every vendored target and drags a C++
#   compile into otherwise C-only libraries.
#
# NOTE: feature-detection results are cached, so when changing anything here,
# delete CMakeCache.txt or the probes will not re-run.
set(_meshmc_saved_c_standard "${CMAKE_C_STANDARD}")
set(_meshmc_saved_c_standard_required "${CMAKE_C_STANDARD_REQUIRED}")
set(_meshmc_saved_automoc "${CMAKE_AUTOMOC}")
set(_meshmc_saved_autouic "${CMAKE_AUTOUIC}")
set(_meshmc_saved_autorcc "${CMAKE_AUTORCC}")
unset(CMAKE_C_STANDARD)
unset(CMAKE_C_STANDARD_REQUIRED)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

############################ zlib-ng (must be first) ##########################
# ORDER MATTERS. zlib-ng must be configured before libarchive: the cache
# entries seeded below only work once zlib-ng has generated its zlib.h, and
# libarchive's find_package(ZLIB) happens while it is being added. Configure
# libarchive first and it silently builds without DEFLATE support — precisely
# the bug that made every .jar unreadable in 8.2.0. Do not reorder these two.
#
# ZLIB_COMPAT=ON is required, not cosmetic: libarchive consumes the standard
# zlib API (zlib.h, inflate(), z_stream). In zlib-ng's native mode the symbols
# are zng_-prefixed and libarchive cannot use them at all.
set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)
set(WITH_GTEST OFF CACHE BOOL "" FORCE)
set(WITH_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(WITH_FUZZERS OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)

# zlib-ng's tests (gtest, pigz, fuzzers) are not ours to run. BUILD_TESTING is
# shadowed with a plain variable rather than set in the cache, because the same
# entry drives MeshMC's own unit tests, which we very much do want. zlib-ng
# declares cmake_minimum_required(3.14...3.31), so CMP0077 is NEW there and its
# option() honours this.
set(_meshmc_saved_build_testing "${BUILD_TESTING}")
set(BUILD_TESTING OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/libraries/zlib-ng zlib-ng)
set(BUILD_TESTING "${_meshmc_saved_build_testing}")
unset(_meshmc_saved_build_testing)

# ----------------------------------------------------------------------------
# Hand the in-tree zlib-ng to libarchive's find_package(ZLIB 1.2.1).
#
# libarchive does `SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/cmake")`
# — a plain overwrite, not an append — so dropping a FindZLIB.cmake into our
# own cmake/ directory does NOT work: libarchive cannot see it and falls
# through to CMake's stock FindZLIB. Since we do not patch submodules, we make
# the stock module succeed instead.
#
# find_path()/find_library() skip searching when their cache variable is
# already set, and stock FindZLIB only creates ZLIB::ZLIB `if(NOT TARGET
# ZLIB::ZLIB)`. So: pre-seed both cache entries and pre-create the alias.
# FindZLIB then reports found, reads the version out of the generated zlib.h,
# and leaves our alias alone.
#
# ZLIB_LIBRARY_RELEASE is deliberately the *target name*, not a file path: it
# ends up in libarchive's ADDITIONAL_LIBS and therefore in
# target_link_libraries(), which keeps real build ordering and works across
# every configuration of a multi-config generator.
#
# ZLIB_INCLUDE_DIR must stay a single directory — FindZLIB does
# file(STRINGS "${ZLIB_INCLUDE_DIR}/zlib.h" ...) to parse the version, which
# breaks on a list. The binary dir is the right one: that is where zlib-ng
# generates zlib.h and zconf.h.
get_target_property(_meshmc_zlibng_bindir zlib-ng BINARY_DIR)
set(ZLIB_INCLUDE_DIR "${_meshmc_zlibng_bindir}" CACHE PATH "" FORCE)
set(ZLIB_LIBRARY_RELEASE "zlib-ng" CACHE STRING "" FORCE)
unset(_meshmc_zlibng_bindir)

if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlib-ng)
endif()
# ----------------------------------------------------------------------------

################################## libarchive #################################
# Only the library itself: no bsdtar/bsdcpio/bsdcat, no test suite, and no
# install rules (we link archive_static into the launcher).
#
# Compression/format backend detection is left at upstream defaults so the
# resulting feature set matches what previous releases shipped. ENABLE_ZLIB is
# forced ON and asserted below, because that one is load-bearing.
set(ENABLE_ZLIB ON CACHE BOOL "" FORCE)
set(ENABLE_OPENSSL OFF CACHE BOOL "" FORCE)
set(ENABLE_TEST OFF CACHE BOOL "" FORCE)
set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_TAR OFF CACHE BOOL "" FORCE)
set(ENABLE_CPIO OFF CACHE BOOL "" FORCE)
set(ENABLE_CAT OFF CACHE BOOL "" FORCE)
set(ENABLE_UNZIP OFF CACHE BOOL "" FORCE)

# Tell libarchive that the MSVC CRT has the intrinsic-named functions, because
# it cannot work that out for itself.
#
# libarchive appends /Oi ("enable intrinsic functions") to CMAKE_C_FLAGS_DEBUG
# and CMAKE_C_FLAGS_RELEASE inside its own IF(MSVC) block, and its probes then
# run with that flag. CMake's check_function_exists() declares the candidate
# as `char wcscpy(void);` and calls it with no arguments — which recent MSVC
# rejects outright once /Oi makes wcscpy an intrinsic:
#
#   warning C4392: 'char wcscpy(void)': incorrect number of arguments for
#                  intrinsic; '2' expected
#   error   C2168: 'wcscpy': too few actual parameters for intrinsic
#
# The probe therefore "fails", libarchive concludes the CRT has no wcscpy, and
# archive_entry.c compiles its own fallback — which MSVC refuses with
# "error C2169: 'wcscpy': intrinsic function, cannot be defined".
#
# Exactly the intrinsic-named probes are affected: everything below came out
# as "missing" while e.g. strftime (same probe mechanism, not an intrinsic)
# was found. All seven are unconditionally present in the MSVC CRT — that is
# *why* they are intrinsics — so stating so is a statement of fact, not a
# workaround for a real absence.
#
# check_function_exists() and libarchive's CHECK_FUNCTION_EXISTS_GLIBC both
# begin with `if(NOT DEFINED <var>)`, so a pre-seeded cache entry skips the
# broken probe entirely. CACHE INTERNAL implies FORCE, which also cleans up
# empty entries left by an earlier failed configure.
#
# Note the deliberate choice NOT to fix this by steering
# CMAKE_TRY_COMPILE_CONFIGURATION at a config without /Oi: /O2 already implies
# /Oi, so RelWithDebInfo and Release are both out, and that would make the fix
# depend on MSVC's optimisation-flag implications rather than on a fact.
if(MSVC)
    foreach(_meshmc_crt_fn IN ITEMS
            HAVE_MEMMOVE HAVE_WCSCMP HAVE_WCSCPY HAVE_WCSLEN
            HAVE_WMEMCMP HAVE_WMEMCPY HAVE_WMEMMOVE)
        set(${_meshmc_crt_fn} 1 CACHE INTERNAL
            "Present in the MSVC CRT; libarchive's /Oi probe cannot detect it")
    endforeach()
    unset(_meshmc_crt_fn)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/libraries/libarchive libarchive)

# Fail the build instead of shipping an archive library that cannot inflate.
#
# This asserts the actual outcome, not our intent: HAVE_ZLIB_H in the config.h
# libarchive just generated is the exact switch that compiles its DEFLATE
# support in or out. Getting it wrong is not a loud failure — you get a
# library that opens .jar files happily and then refuses every entry inside
# them — so it is worth re-checking on every configure.
#
# (ZLIB_FOUND is set inside libarchive's own directory scope and is not
# visible from here, hence reading config.h instead.)
set(_meshmc_libarchive_config "${CMAKE_CURRENT_BINARY_DIR}/libarchive/config.h")
set(_meshmc_libarchive_has_zlib "")
if(EXISTS "${_meshmc_libarchive_config}")
    file(STRINGS "${_meshmc_libarchive_config}" _meshmc_libarchive_has_zlib
         REGEX "^#define[ \t]+HAVE_ZLIB_H[ \t]+1")
endif()
if(NOT _meshmc_libarchive_has_zlib)
    message(FATAL_ERROR
        "libarchive was configured WITHOUT zlib support.\n"
        "It would be unable to read any ordinary .jar/.zip file, failing with "
        "\"Unsupported ZIP compression method (8: deflation)\" on every "
        "DEFLATE entry.\n"
        "The bundled zlib-ng is most likely not checked out. Run:\n"
        "    git submodule update --init --recursive")
endif()

# Verify the pre-seeding above actually reached config.h, so this fails at
# configure time instead of 60 objects into the build with an opaque C2169.
# Guards against the HAVE_* names drifting upstream, and against libarchive
# growing a new intrinsic-named probe we have not seeded yet.
if(MSVC)
    set(_meshmc_libarchive_crt_missing "")
    foreach(_meshmc_fn IN ITEMS
            HAVE_MEMMOVE HAVE_WCSCMP HAVE_WCSCPY HAVE_WCSLEN
            HAVE_WMEMCMP HAVE_WMEMCPY HAVE_WMEMMOVE)
        set(_meshmc_fn_hit "")
        if(EXISTS "${_meshmc_libarchive_config}")
            file(STRINGS "${_meshmc_libarchive_config}" _meshmc_fn_hit
                 REGEX "^#define[ \t]+${_meshmc_fn}[ \t]+1")
        endif()
        if(NOT _meshmc_fn_hit)
            list(APPEND _meshmc_libarchive_crt_missing "${_meshmc_fn}")
        endif()
    endforeach()
    if(_meshmc_libarchive_crt_missing)
        message(FATAL_ERROR
            "libarchive thinks the MSVC CRT is missing: "
            "${_meshmc_libarchive_crt_missing}\n"
            "It is not. Its /Oi-enabled check_function_exists() probes cannot "
            "detect intrinsic-named functions (see the note above), so these "
            "are pre-seeded into the cache — and that seeding did not reach "
            "config.h. Either a HAVE_* name changed upstream, or there is a "
            "newly probed intrinsic to add to the list.\n"
            "Left alone, libarchive compiles its own fallback definitions and "
            "the build dies with \"C2169: intrinsic function, cannot be "
            "defined\".\n"
            "The failing probe is logged in "
            "build/CMakeFiles/CMakeConfigureLog.yaml.")
    endif()
    unset(_meshmc_libarchive_crt_missing)
    unset(_meshmc_fn)
    unset(_meshmc_fn_hit)
endif()

unset(_meshmc_libarchive_config)
unset(_meshmc_libarchive_has_zlib)

# libarchive sets LIBARCHIVE_STATIC on archive_static as a *private* compile
# definition, so consumers would still see __declspec(dllimport) in archive.h
# and fail to link on Windows. Propagate it through the interface.
# set_property() rather than target_compile_definitions(): the target is
# created in a nested directory scope.
set_property(TARGET archive_static APPEND
             PROPERTY INTERFACE_COMPILE_DEFINITIONS LIBARCHIVE_STATIC)

# Sibling libraries and the updater look for LibArchive::LibArchive (the name
# CMake's FindLibArchive exports). Provide it so they need no special casing.
if(NOT TARGET LibArchive::LibArchive)
    add_library(LibArchive::LibArchive ALIAS archive_static)
endif()

# Vendored C libraries are done; MeshMC's own settings apply again from here.
set(CMAKE_C_STANDARD "${_meshmc_saved_c_standard}")
set(CMAKE_C_STANDARD_REQUIRED "${_meshmc_saved_c_standard_required}")
set(CMAKE_AUTOMOC "${_meshmc_saved_automoc}")
set(CMAKE_AUTOUIC "${_meshmc_saved_autouic}")
set(CMAKE_AUTORCC "${_meshmc_saved_autorcc}")
unset(_meshmc_saved_c_standard)
unset(_meshmc_saved_c_standard_required)
unset(_meshmc_saved_automoc)
unset(_meshmc_saved_autouic)
unset(_meshmc_saved_autorcc)

############################### monorepo libraries ############################
option(NBT_BUILD_SHARED "Build NBT shared library" ON)
option(NBT_USE_ZLIB "Build NBT library with zlib support" OFF)
option(NBT_BUILD_TESTS "Build NBT library tests" OFF) #FIXME: fix unit tests.
set(NBT_DEST_DIR ${LIBRARY_DEST_DIR})

add_subdirectory(${CMAKE_SOURCE_DIR}/libraries/libnbtplusplus libnbtplusplus)

add_subdirectory(libraries/systeminfo) # system information library
add_subdirectory(libraries/ganalytics) # google analytics library
add_subdirectory(libraries/launcher) # java based launcher part for Minecraft
add_subdirectory(libraries/javacheck) # java compatibility checker
add_subdirectory(libraries/xz-embedded) # xz compression
add_subdirectory(libraries/rainbow) # Qt extension for colors
add_subdirectory(libraries/iconfix) # fork of Qt's QIcon loader
add_subdirectory(libraries/localpeer) # fork of a library from Qt solutions
add_subdirectory(libraries/classparser) # Java class file parser
add_subdirectory(libraries/optional-bare)
add_subdirectory(libraries/katabasis) # An OAuth2 library that tried to do too much

# Three libraries under libraries/ build as shared objects (iconfix and rainbow
# hardcode SHARED, nbt++ opts in via NBT_BUILD_SHARED), so they have to be
# deployed where the launcher can load them: alongside the binary on Windows,
# LIBRARY_DEST_DIR elsewhere (covered by the RPATH, $ORIGIN/../lib).
#
# Installed explicitly rather than left to install(RUNTIME_DEPENDENCY_SET),
# which scans the built binary and searches a directory list — it did not pick
# these three up. Their own subprojects do install them, but to
# ${CMAKE_INSTALL_BINDIR} (install/bin/) while meshmc.exe sits at the install
# root, so the launcher died on startup with "nbt++.dll not found". These are
# our own targets and we know exactly where they belong; no point rediscovering
# it by scanning a binary.
install(TARGETS nbt++ iconfix rainbow
    RUNTIME DESTINATION "${BINARY_DEST_DIR}"
    LIBRARY DESTINATION "${LIBRARY_DEST_DIR}")

# The Java helpers are built in-tree now, so their jars come out of the build
# directory. The launcher looks for them by exact name under JARS_DEST_DIR
# (see Application::getJarsPath()), which is not where install_jar() puts them.
get_target_property(JAVACHECK_JAR JavaCheck JAR_FILE)
get_target_property(NEWLAUNCH_JAR NewLaunch JAR_FILE)
install(FILES "${JAVACHECK_JAR}" DESTINATION "${JARS_DEST_DIR}" RENAME "JavaCheck.jar")
install(FILES "${NEWLAUNCH_JAR}" DESTINATION "${JARS_DEST_DIR}" RENAME "NewLaunch.jar")

# cmark, zlib-ng and libarchive each do
#   if(NOT CMAKE_BUILD_TYPE) set(CMAKE_BUILD_TYPE Release CACHE ... FORCE)
# which, under a multi-config generator where CMAKE_BUILD_TYPE is empty by
# design, plants a bogus "Release" in our cache. It is ignored at build time,
# but it makes a re-configure of the same build tree behave differently from
# the first one (see the ENABLE_LTO block near the top) and is thoroughly
# confusing to read. Drop it again — but only for multi-config, where the
# variable is meaningless anyway, never when the user set it deliberately.
if(CMAKE_CONFIGURATION_TYPES AND DEFINED CACHE{CMAKE_BUILD_TYPE})
    unset(CMAKE_BUILD_TYPE CACHE)
endif()

############################### Built Artifacts ###############################

add_subdirectory(buildconfig)

# The standalone meshmc-updater binary is built whenever the updater feed
# URL is configured AND the build supplies either a structured asset
# identity (platform/arch/kind) or the legacy substring identifier.
if(NOT MeshMC_UPDATER_FEED_URL STREQUAL "" AND
   ((NOT MeshMC_BUILD_PLATFORM_ID STREQUAL "" AND
     NOT MeshMC_BUILD_ARCH STREQUAL "" AND
     NOT MeshMC_BUILD_KIND STREQUAL "") OR
    NOT MeshMC_BUILD_ARTIFACT STREQUAL ""))
    add_subdirectory(updater)
endif()

add_subdirectory(crashreporter)

# NOTE: this must always be last to appease the CMake deity of quirky install command evaluation order.
add_subdirectory(launcher)

if(MeshMC_PLUGINS)
    # Plugin SDK (INTERFACE library — headers only, must come before plugins)
    add_subdirectory(launcher/plugin/sdk)

    # In-tree .mmco plugins (after launcher so MeshMC_logic target is available)
    add_subdirectory(plugins)
endif()
