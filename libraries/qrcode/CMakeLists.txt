cmake_minimum_required(VERSION 3.10)

project(qrcode)
enable_testing()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_C_STANDARD_REQUIRED true)
set(CMAKE_C_STANDARD 11)


set( qrcode_PRIVATE
    src/qrcodegen.cpp
    src/qrcodegen.h
    src/QrCodeGenerator.cpp
)

set( qrcode_PUBLIC
    include/qrcode/QrCodeGenerator.h
)

add_library( qrcode STATIC ${qrcode_PRIVATE} ${qrcode_PUBLIC} )
target_link_libraries(qrcode Qt${Launcher_QT_MAJOR}::Core Qt${Launcher_QT_MAJOR}::Gui Qt${Launcher_QT_MAJOR}::Svg)

# needed for statically linked qrcode in shared libs on x86_64
set_target_properties(qrcode
    PROPERTIES POSITION_INDEPENDENT_CODE TRUE
)

target_include_directories(qrcode PUBLIC include PRIVATE src include/qrcode)
