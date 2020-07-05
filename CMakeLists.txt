cmake_minimum_required(VERSION 3.5)

project(mpcv LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/;${CMAKE_MODULE_PATH}")

find_package(TBB REQUIRED tbb)
add_definitions(-DTBB_SUPPRESS_DEPRECATED_MESSAGES)

find_package(OpenVDB)
if (OpenVDB_FOUND)
    add_definitions(-DENABLE_MESH_REPAIR)
    list(APPEND OpenVDB_LIBRARIES "Half")
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)
add_definitions(-DQT_NO_DEBUG_OUTPUT)

find_package(OpenImageDenoise REQUIRED)

add_executable(mpcv
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    openglwidget.h openglwidget.cpp
    camera.h
    quaternion.h
    mesh.h mesh.cpp
    las.h las.cpp
    coordinates.h
    bvh.h bvh.cpp
    ambient.h ambient.cpp
    renderer.h renderer.cpp
    framebuffer.h framebuffer.cpp framebuffer.ui
    resources.qrc
  )

target_link_libraries(mpcv PRIVATE
    Qt5::Widgets Qt5::Gui
    GL GLU
    ${Tbb_LIBRARIES}
    ${OpenVDB_LIBRARIES}
    laslib
    "/usr/local/lib/libOpenImageDenoise.so"
    )
