cmake_minimum_required(VERSION 3.5)

project(mpcv LANGUAGES CXX)

option(WITH_OPENVDB "Link OpenVDB to enable mesh repair"                 OFF)
option(WITH_OIDN    "Link OpenImageDenoise to enable render denoising"   OFF)
option(WITH_JPEG    "Link libjpeg to enable opening large jpeg textures" OFF)
option(WITH_PNG     "Link libpng to enable opening large png textures"   OFF)
option(WITH_GDAL    "Link GDAL to enable reading DEMs/DSMs"              OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/;${CMAKE_MODULE_PATH}")

find_package(TBB REQUIRED tbb)
add_definitions(-DTBB_SUPPRESS_DEPRECATED_MESSAGES)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
add_definitions(-DQT_NO_DEBUG_OUTPUT)

include_directories(LAStools/LASlib/inc)
include_directories(LAStools/LASzip/src)
add_subdirectory(LAStools)

include_directories(libE57Format/include)
add_subdirectory(libE57Format)
include_directories("${CMAKE_BINARY_DIR}/libE57Format") # libE57 codegen

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

add_executable(mpcv
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    openglwidget.h openglwidget.cpp
    utils.h utils.cpp
    camera.h camera.cpp
    quaternion.h
    parameters.h
    mesh.h mesh.cpp
    texture.h texture.cpp
    las.h las.cpp
    json11.hpp json11.cpp
    e57.h e57.cpp
    dem.h dem.cpp
    coordinates.h
    bvh.h bvh.cpp
    renderer.h renderer.cpp
    sun-sky/SunSky.h sun-sky/SunSky.cpp
    framebuffer.h framebuffer.cpp framebuffer.ui
    sunwidget.h sunwidget.cpp sunwidget.ui
    resources.qrc
  )

set(LIBRARIES
    Qt5::Widgets Qt5::Gui
    LASlib
    E57Format
    GL GLU
    ${Tbb_LIBRARIES})

if (WITH_OPENVDB)
    find_package(OpenVDB REQUIRED)
    list(APPEND LIBRARIES openvdb Half)
    add_definitions(-DHAS_OPENVDB)
endif()

if (WITH_OIDN)
    find_package(OpenImageDenoise)
    list(APPEND LIBRARIES OpenImageDenoise)
    add_definitions(-DHAS_OIDN)
endif()

if (WITH_JPEG)
    list(APPEND LIBRARIES jpeg)
    add_definitions(-DHAS_JPEG)
endif()

if (WITH_PNG)
    list(APPEND LIBRARIES png)
    add_definitions(-DHAS_PNG)
endif()

if (WITH_GDAL)
    find_package(GDAL)
    list(APPEND LIBRARIES ${GDAL_LIBRARIES})
    target_include_directories(mpcv PUBLIC ${GDAL_INCLUDE_DIRS})
    add_definitions(-DHAS_GDAL)
endif()

target_link_libraries(mpcv PRIVATE ${LIBRARIES})
