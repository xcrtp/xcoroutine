cmake_minimum_required(VERSION 3.10..4.1)
include(cmake/get_version.cmake)
xc_get_project_version(PROJECT_VERSION)
project(xcoroutine LANGUAGES CXX VERSION ${PROJECT_VERSION})
# Build tests
option(${CMAKE_PROJECT_NAME}_BUILD_TESTS "Build unit tests" OFF)
# Build examples
option(${CMAKE_PROJECT_NAME}_BUILD_EXAMPLES "Build examples" OFF)
# Config file name for find_package()
set(CONFIG_FILE_NAME "${CMAKE_PROJECT_NAME}Config.cmake")
if(${CMAKE_PROJECT_NAME}_BUILD_TESTS)
    enable_testing()
endif()
# Include utility functions
include(cmake/xc_utils.cmake)

# Include dependency installation (optional)
include(cmake/install_deps.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



xc_add_format_dir(xcoroutine)
add_subdirectory(xcoroutine)
# Add test subdirectory if tests are enabled
if(${CMAKE_PROJECT_NAME}_BUILD_TESTS)
    include(cmake/install_test_deps.cmake)
    add_subdirectory(test)
endif()

# Add examples subdirectory if examples are enabled
if(${CMAKE_PROJECT_NAME}_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()


# Include format configuration
include(cmake/formate.cmake)
include(cmake/install.cmake)