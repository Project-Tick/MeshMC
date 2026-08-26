cmake_minimum_required(VERSION 3.25)
project(JavaCheck VERSION 1.0.0)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(Java 1.7 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT JavaCheck)
set(CMAKE_JAVA_COMPILE_FLAGS --release 7 -Xlint:deprecation -Xlint:unchecked -Xlint:-options)

set(SRC
    JavaCheck.java
)

add_jar(JavaCheck ${SRC})

# Built as part of MeshMC, the jar is installed by the top-level CMakeLists.txt
# instead: the launcher looks it up by exact name under JARS_DEST_DIR, which is
# not where install_jar() puts it.
if(NOT PROJECT_IS_TOP_LEVEL)
    return()
endif()

# NB: deliberately no second install(FILES "${CMAKE_CURRENT_BINARY_DIR}/JavaCheck.jar").
# install_jar() already installs the jar, and that hand-rolled duplicate
# hardcoded an output location UseJava never promised: MeshMC's own
# cmake/UseJava.cmake writes to ${CMAKE_BINARY_DIR}/jars, so the path did not
# exist and `cmake --install` failed outright. Use the JAR_FILE target property
# if you ever need the real path.
install_jar(JavaCheck "${CMAKE_INSTALL_DATADIR}/JavaCheck")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/JavaCheckConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/JavaCheckConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JavaCheck)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/JavaCheckConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/JavaCheckConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/JavaCheckConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JavaCheck)
