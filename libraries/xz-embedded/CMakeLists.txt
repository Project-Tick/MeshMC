cmake_minimum_required(VERSION 3.25)
project(xz-embedded VERSION 1.0.0 LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(XZ_BUILD_BCJ "Build xz-embedded with BCJ support (native binary optimization)" OFF)
option(XZ_BUILD_CRC64 "Build xz-embedded with CRC64 checksum support" ON)
option(XZ_BUILD_MINIDEC "Build a tiny utility that decompresses xz streams" OFF)

set(XZ_SOURCES
    src/xz_crc32.c
    src/xz_crc64.c
    src/xz_dec_lzma2.c
    src/xz_dec_stream.c
)
add_library(xz-embedded ${XZ_SOURCES})
add_library(xz-embedded::xz-embedded ALIAS xz-embedded)
target_include_directories(xz-embedded
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/xz-embedded>)
set_property(TARGET xz-embedded PROPERTY C_STANDARD 99)

if(${XZ_BUILD_MINIDEC})
    add_executable(xzminidec xzminidec.c)
    target_link_libraries(xzminidec xz-embedded)
    set_property(TARGET xzminidec PROPERTY C_STANDARD 99)
endif()
