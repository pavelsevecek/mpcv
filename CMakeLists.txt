cmake_minimum_required(VERSION 3.5)

project(mpcv LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

find_package(TBB REQUIRED tbb)
add_definitions(-DTBB_SUPPRESS_DEPRECATED_MESSAGES)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

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
    resources.qrc
  )

target_link_libraries(mpcv PRIVATE Qt5::Widgets Qt5::Gui GL GLU tbb "laslib" "openvdb" "Half")
