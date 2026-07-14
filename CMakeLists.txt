cmake_minimum_required(VERSION 3.21)

project(ProgramViz VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Test)
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(ProgramViz MACOSX_BUNDLE
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/hsipalettedialog.cpp
    src/hsipalettedialog.h
    src/projectmodel.cpp
    src/projectmodel.h
    src/treemapview.cpp
    src/treemapview.h
    src/visualsettings.cpp
    src/visualsettings.h
)

target_link_libraries(ProgramViz PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

enable_testing()
qt_add_executable(ProgramVizTests
    tests/projectmodel_test.cpp
    src/projectmodel.cpp
    src/projectmodel.h
    src/visualsettings.cpp
    src/visualsettings.h
)
target_link_libraries(ProgramVizTests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)
add_test(NAME ProgramVizTests COMMAND ProgramVizTests)

set_target_properties(ProgramViz PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "ProgramViz"
    MACOSX_BUNDLE_GUI_IDENTIFIER "dev.programviz.app"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ProgramViz
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
