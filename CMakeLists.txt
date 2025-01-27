cmake_minimum_required(VERSION 3.23)
project(DumbICOM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# allow fetching deps from github
include(FetchContent)

# Boost for nice program options/flags
find_package(Boost COMPONENTS program_options REQUIRED)

# DICOM-ToolKit for reading data
find_package(DCMTK REQUIRED)

# OpenCV for wrangling data
find_package(OpenCV REQUIRED)

# for rendering
find_package(OpenGL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(
    VTK

    COMPONENTS  # needs to have components specified

    CommonColor
    CommonCore
    CommonDataModel
    InteractionStyle
    InteractionWidgets
    RenderingContextOpenGL2
    RenderingCore
    RenderingFreeType
    RenderingGL2PSOpenGL2
    RenderingOpenGL2
    RenderingVolume
    RenderingVolumeOpenGL2
)

string(TOLOWER ${PROJECT_NAME} BIN_NAME)
add_executable(
    ${BIN_NAME}

    src/main.cpp
    src/options.cpp
    src/options.hpp
    src/dicom.cpp
    src/dicom.hpp
    src/image_stack.cpp
    src/image_stack.hpp
    src/scene.cpp
    src/scene.hpp
    src/convenience.hpp
)

target_link_libraries(
    ${BIN_NAME}

    LINK_PUBLIC

    ${Boost_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${OpenCV_LIBRARIES}
    ${VTK_LIBRARIES}
    ${DCMTK_LIBRARIES}
)

vtk_module_autoinit(
    TARGETS ${BIN_NAME}
    MODULES ${VTK_LIBRARIES}
)
