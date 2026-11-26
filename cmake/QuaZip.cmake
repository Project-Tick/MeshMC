# Build upstream QuaZip with the CMake 3.10 used by the legacy Linux images.
find_package(ZLIB REQUIRED)
set(quazip_source "${CMAKE_CURRENT_SOURCE_DIR}/libraries/quazip/quazip")
add_library(Launcher_quazip SHARED
    "${quazip_source}/JlCompress.cpp"
    "${quazip_source}/qioapi.cpp"
    "${quazip_source}/quaadler32.cpp"
    "${quazip_source}/quachecksum32.cpp"
    "${quazip_source}/quacrc32.cpp"
    "${quazip_source}/quagzipfile.cpp"
    "${quazip_source}/quaziodevice.cpp"
    "${quazip_source}/quazip.cpp"
    "${quazip_source}/quazipdir.cpp"
    "${quazip_source}/quazipfile.cpp"
    "${quazip_source}/quazipfileinfo.cpp"
    "${quazip_source}/quazipnewinfo.cpp"
    "${quazip_source}/unzip.c"
    "${quazip_source}/zip.c"
)
target_include_directories(Launcher_quazip PUBLIC "${quazip_source}")
target_link_libraries(Launcher_quazip PUBLIC Qt5::Core ZLIB::ZLIB)
target_compile_definitions(Launcher_quazip PRIVATE QUAZIP_BUILD)
set_target_properties(Launcher_quazip PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
install(TARGETS Launcher_quazip
    RUNTIME DESTINATION ${LIBRARY_DEST_DIR}
    LIBRARY DESTINATION ${LIBRARY_DEST_DIR}
)

set(quazip_tests "${CMAKE_CURRENT_SOURCE_DIR}/libraries/quazip/qztest")
file(GLOB quazip_test_sources "${quazip_tests}/*.cpp")
qt5_add_resources(quazip_test_resources "${quazip_tests}/qztest.qrc")
add_executable(Launcher_quazip_test ${quazip_test_sources} ${quazip_test_resources})
target_link_libraries(Launcher_quazip_test PRIVATE Launcher_quazip Qt5::Network Qt5::Test)
add_test(NAME quazip_testsuite COMMAND Launcher_quazip_test)
