cmake_minimum_required(VERSION 3.16)
project(HybridCAD VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets)
find_package(OpenGL REQUIRED)

# Try to find OpenCASCADE (optional)
find_package(OpenCASCADE QUIET)
if(OpenCASCADE_FOUND)
    message(STATUS "OpenCASCADE found")
else()
    message(STATUS "OpenCASCADE not found - advanced CAD features will be disabled")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/CADViewer.cpp
    src/GeometryManager.cpp
    src/MeshManager.cpp
    src/PartManager.cpp
    src/ToolManager.cpp
    src/PropertyPanel.cpp
    src/TreeView.cpp
)

# Header files
set(HEADERS
    include/MainWindow.h
    include/CADViewer.h
    include/GeometryManager.h
    include/MeshManager.h
    include/PartManager.h
    include/ToolManager.h
    include/PropertyPanel.h
    include/TreeView.h
    include/CADTypes.h
)

# Process Qt resources
qt6_add_resources(RESOURCES resources/resources.qrc)

# Process Qt MOC for headers that need it
qt6_wrap_cpp(MOC_SOURCES ${HEADERS})

# Create executable
add_executable(HybridCAD 
    ${SOURCES} 
    ${HEADERS}
    ${MOC_SOURCES}
    ${RESOURCES}
)

# Link required libraries
target_link_libraries(HybridCAD
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    OpenGL::GL
)

# Link OpenCASCADE if found (basic components only)
if(OpenCASCADE_FOUND)
    # Only link core libraries that are definitely available
    target_link_libraries(HybridCAD 
        TKernel 
        TKMath 
        TKG2d 
        TKG3d 
        TKGeomBase 
        TKBRep 
        TKGeomAlgo 
        TKTopAlgo 
        TKBool
        TKOffset
        TKFeat
        TKFillet
        TKMesh
    )
    target_include_directories(HybridCAD PRIVATE ${OpenCASCADE_INCLUDE_DIR})
    target_compile_definitions(HybridCAD PRIVATE HAVE_OPENCASCADE)
    message(STATUS "OpenCASCADE core libraries linked")
endif()

# Set target properties
set_target_properties(HybridCAD PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Installation
install(TARGETS HybridCAD
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
) 