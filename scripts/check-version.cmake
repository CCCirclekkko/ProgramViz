if(NOT DEFINED TAG_VERSION OR TAG_VERSION STREQUAL "")
    message(FATAL_ERROR "TAG_VERSION is required, for example -DTAG_VERSION=0.1.0")
endif()

file(READ "${CMAKE_CURRENT_LIST_DIR}/../CMakeLists.txt" cmake_lists)
string(REGEX MATCH "project\\([ \t]*ProgramViz[ \t]+VERSION[ \t]+([0-9]+\\.[0-9]+\\.[0-9]+)" match "${cmake_lists}")
if(NOT match)
    message(FATAL_ERROR "Could not find ProgramViz project version in CMakeLists.txt")
endif()

set(cmake_version "${CMAKE_MATCH_1}")
if(NOT cmake_version STREQUAL TAG_VERSION)
    message(FATAL_ERROR "Git tag version ${TAG_VERSION} does not match CMake version ${cmake_version}")
endif()

message(STATUS "Version check passed: ${TAG_VERSION}")
