cmake_minimum_required(VERSION 3.10)
project(LocalPeer)


set(SINGLE_SOURCES
src/LocalPeer.cpp
src/LockedFile.cpp
src/LockedFile.h
include/LocalPeer.h
)

if(UNIX)
    list(APPEND SINGLE_SOURCES
        src/LockedFile_unix.cpp
    )
endif()

if(WIN32)
    list(APPEND SINGLE_SOURCES
        src/LockedFile_win.cpp
    )
endif()

add_library(LocalPeer STATIC ${SINGLE_SOURCES})
target_include_directories(LocalPeer PUBLIC include)

target_link_libraries(LocalPeer Qt${Launcher_QT_MAJOR}::Core Qt${Launcher_QT_MAJOR}::Network)

include (UnitTest)
add_unit_test(LocalPeer
    SOURCES src/LocalPeer_test.cpp
    LIBS LocalPeer
)
