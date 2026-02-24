# CMake Utilities for Library Creation
#
# Configuration Variables:
# DEFAULT_NAMESPACE           - Default namespace for libraries (default: CMAKE_PROJECT_NAME)
# TEST_PREFIX                - Prefix for test executables (default: "test_")
# TEST_SUFFIX                - Suffix for test executables (default: "")
# BUILD_SHARED               - Build shared libraries instead of static (default: OFF)
# ${PROJECT_NAME}_BUILD_SHARED - Override BUILD_SHARED per project
# ${PROJECT_NAME}_MODIFY_OUTPUT_PREFIX - Add namespace prefix to output name (default: ON)
#
# Internal Functions (prefixed with __xc_):
# __xc_link_include    - Add link libraries and include directories
# __xc_get_namespace   - Get namespace
# __xc_set_output      - Set output name with prefix
#
# Public API:
# xc_install           - Install target and headers
# xc_add_alias         - Add namespace alias
# xc_target_source     - Add source files
# xc_add_header_library  - Create header-only library
# xc_add_static_library   - Create static library
# xc_add_shared_library  - Create shared library
# xc_add_library         - Create static or shared library
# xc_add_test            - Create unit test

if(NOT DEFINED DEFAULT_NAMESPACE)
    set(DEFAULT_NAMESPACE ${CMAKE_PROJECT_NAME})
endif()

if(NOT DEFINED TEST_PREFIX)
    set(TEST_PREFIX "test_")
endif(NOT DEFINED TEST_PREFIX)

if(NOT DEFINED TEST_SUFFIX)
    set(TEST_SUFFIX "")
endif(NOT DEFINED TEST_SUFFIX)

set(BUILD_SHARED OFF)

if(DEFINED ${CMAKE_PROJECT_NAME}_BUILD_SHARED)
    set(BUILD_SHARED ${${CMAKE_PROJECT_NAME}_BUILD_SHARED})
endif()

if(NOT DEFINED ${CMAKE_PROJECT_NAME}_MODIFY_OUTPUT_PREFIX)
    set(${CMAKE_PROJECT_NAME}_MODIFY_OUTPUT_PREFIX ON)
endif()

# __xc_link_include(TARGET)
# Add link libraries and include directories to target
#
# Options:
# PUBLIC_LINK lib        - Public link library
# PRIVATE_LINK lib      - Private link library
# INTERFACE_LINK lib     - Interface link library
# PUBLIC_INCLUDE path   - Public include directory
# PRIVATE_INCLUDE path   - Private include directory
# INTERFACE_INCLUDE path - Interface include directory
#
# Examples:
# __xc_link_include(my_target PUBLIC_LINK lib1 PRIVATE_LINK lib2)
# __xc_link_include(my_target PUBLIC_INCLUDE /path/to/include)
# __xc_link_include(my_target INTERFACE_LINK some_target INTERFACE_INCLUDE /path)
function(__xc_link_include TARGET)
    cmake_parse_arguments(
        ARG
        ""
        ""
        "PUBLIC_LINK;PRIVATE_LINK;INTERFACE_LINK;PUBLIC_INCLUDE;PRIVATE_INCLUDE;INTERFACE_INCLUDE"
        "${ARGN}"
    )

    if(DEFINED ARG_PUBLIC_LINK)
        target_link_libraries(${TARGET} PUBLIC ${ARG_PUBLIC_LINK})
    endif()

    if(DEFINED ARG_PRIVATE_LINK)
        target_link_libraries(${TARGET} PRIVATE ${ARG_PRIVATE_LINK})
    endif()

    if(DEFINED ARG_INTERFACE_LINK)
        target_link_libraries(${TARGET} INTERFACE ${ARG_INTERFACE_LINK})
    endif()

    if(DEFINED ARG_PUBLIC_INCLUDE)
        target_include_directories(${TARGET} PUBLIC ${ARG_PUBLIC_INCLUDE})
    endif()

    if(DEFINED ARG_PRIVATE_INCLUDE)
        target_include_directories(${TARGET} PRIVATE ${ARG_PRIVATE_INCLUDE})
    endif()

    if(DEFINED ARG_INTERFACE_INCLUDE)
        target_include_directories(${TARGET} INTERFACE ${ARG_INTERFACE_INCLUDE})
    endif()
endfunction(__xc_link_include)

# __xc_get_namespace(OUT_VAR)
# Get namespace, defaults to DEFAULT_NAMESPACE
#
# Options:
# NAMESPACE name - Override namespace
#
# Examples:
# __xc_get_namespace(NS)
# __xc_get_namespace(NS NAMESPACE custom)
function(__xc_get_namespace OUT_VAR)
    cmake_parse_arguments(
        ARG
        ""
        "NAMESPACE"
        ""
        "${ARGN}"
    )
    set(${OUT_VAR} ${DEFAULT_NAMESPACE} PARENT_SCOPE)

    if(DEFINED ARG_NAMESPACE)
        set(${OUT_VAR} ${ARG_NAMESPACE} PARENT_SCOPE)
    endif()
endfunction(__xc_get_namespace)

# xc_install(TARGET DIR NAMESPACE)
# Install target library and header files
#
# Examples:
# xc_install(myhello hello myproject)
function(xc_install TARGET DIR NAMESPACE)
    install(
        TARGETS ${TARGET}
        EXPORT ${NAMESPACE}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(
        DIRECTORY ${DIR}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/
        FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.h"
        PATTERN "*.hh"
    )
endfunction(xc_install)

# xc_add_alias(TARGET NAMESPACE)
# Add namespace alias for target (e.g., myproject::hello)
#
# Examples:
# xc_add_alias(hello myproject)
function(xc_add_alias TARGET NAMESPACE)
    set(ALIAS_NAME "${NAMESPACE}::${TARGET}")

    if("${TARGET}" MATCHES "^${NAMESPACE}_.*")
        string(REPLACE "${NAMESPACE}_" "${NAMESPACE}::" ALIAS_NAME ${TARGET})
    endif()

    add_library("${ALIAS_NAME}" ALIAS ${TARGET})
    message(STATUS "add alias ${ALIAS_NAME} to ${TARGET}")
endfunction(xc_add_alias)

# __xc_set_output(TARGET NAMESPACE)
# Set target output name (e.g., myproject_hello)
#
# Examples:
# __xc_set_output(hello myproject)
function(__xc_set_output TARGET NAMESPACE)
    if("${TARGET}" MATCHES "^${NAMESPACE}_.*")
        set_target_properties(
            ${TARGET}
            PROPERTIES OUTPUT_NAME "${TARGET}"
        )
    else()
        set_target_properties(
            ${TARGET}
            PROPERTIES OUTPUT_NAME "${NAMESPACE}_${TARGET}"
        )
    endif()
endfunction(__xc_set_output TARGET NAMESPACE)

# xc_target_source(TARGET DIR)
# Add source files to target with recursive/non-recursive support
#
# Options:
# NORECURSIVE         - Non-recursive search (default: recursive)
# EXCLUDE_PATH path   - Exclude files matching path
# EXTSRC file         - Add extra source files
#
# Examples:
# xc_target_source(my_target src)
# xc_target_source(my_target src NORECURSIVE)
# xc_target_source(my_target src EXCLUDE_PATH "test" EXCLUDE_PATH "mock")
# xc_target_source(my_target src EXTSRC extra.cpp)
function(xc_target_source TARGET DIR)
    cmake_parse_arguments(
        ARG
        "NORECURSIVE"
        ""
        "EXCLUDE_PATH;EXTSRC"
        ${ARGN}
    )
    set(SOURCE_PATTERNS "${DIR}/*.cc" "${DIR}/*.cpp" "${DIR}/*.c")

    if(ARG_NORECURSIVE)
        file(GLOB SRC_FILES ${SOURCE_PATTERNS})
    else()
        file(GLOB_RECURSE SRC_FILES ${SOURCE_PATTERNS})
    endif()

    if(DEFINED ARG_EXCLUDE_PATH)
        foreach(EXCLUDE ${ARG_EXCLUDE_PATH})
            list(FILTER SRC_FILES EXCLUDE REGEX ".*${EXCLUDE}.*")
        endforeach()
    endif()

    if(DEFINED ARG_EXTSRC)
        list(APPEND SRC_FILES ${ARG_EXTSRC})
    endif()

    target_sources(${TARGET} PRIVATE ${SRC_FILES})
endfunction(xc_target_source)

# Internal macro: common library setup
macro(__xc_setup_lib TARGET SRCDIR)
    __xc_get_namespace(NAMESPACE ${ARGN})
    xc_add_alias(${TARGET} ${NAMESPACE})

    if(${CMAKE_PROJECT_NAME}_MODIFY_OUTPUT_PREFIX)
        __xc_set_output(${TARGET} ${NAMESPACE})
    endif()

    target_include_directories(${TARGET} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}>
    )
    xc_target_source(${TARGET} ${SRCDIR} ${ARGN})
    __xc_link_include(${TARGET} ${ARGN})
    xc_install(${TARGET} ${SRCDIR} ${NAMESPACE})
endmacro(__xc_setup_lib)

# xc_add_header_library(TARGET INCDIR)
# Create a header-only library
#
# Options:
# NAMESPACE name         - Specify namespace (default: CMAKE_PROJECT_NAME)
# PUBLIC_LINK lib        - Public link library
# PRIVATE_LINK lib       - Private link library
# INTERFACE_LINK lib     - Interface link library
# PUBLIC_INCLUDE path    - Public include directory
# PRIVATE_INCLUDE path   - Private include directory
# INTERFACE_INCLUDE path - Interface include directory
#
# Examples:
# xc_add_header_library(mylib include)
# xc_add_header_library(mylib include NAMESPACE myproject)
# xc_add_header_library(mylib include PUBLIC_INCLUDE /path/to/include PRIVATE_LINK some_lib)
function(xc_add_header_library TARGET INCDIR)
    add_library(${TARGET} INTERFACE)
    __xc_get_namespace(NAMESPACE ${ARGN})
    xc_add_alias(${TARGET} ${NAMESPACE})
    target_include_directories(${TARGET} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}>
    )
    __xc_link_include(${TARGET} ${ARGN})
    xc_install(${TARGET} ${INCDIR} ${NAMESPACE})
endfunction(xc_add_header_library)

# xc_add_static_library(TARGET SRCDIR)
# Create a static library
#
# Options:
# NAMESPACE name         - Specify namespace (default: CMAKE_PROJECT_NAME)
# PUBLIC_LINK lib        - Public link library
# PRIVATE_LINK lib       - Private link library
# INTERFACE_LINK lib     - Interface link library
# PUBLIC_INCLUDE path    - Public include directory
# PRIVATE_INCLUDE path   - Private include directory
# INTERFACE_INCLUDE path - Interface include directory
#
# Examples:
# xc_add_static_library(hello hello)
# xc_add_static_library(hello hello NAMESPACE myproject)
# xc_add_static_library(hello hello PUBLIC_LINK otherlib PRIVATE_INCLUDE /path/to/include)
function(xc_add_static_library TARGET SRCDIR)
    add_library(${TARGET} STATIC)
    __xc_setup_lib(${TARGET} ${SRCDIR} ${ARGN})
endfunction(xc_add_static_library)

# xc_add_shared_library(TARGET SRCDIR)
# Create a shared library
#
# Options:
# NAMESPACE name         - Specify namespace (default: CMAKE_PROJECT_NAME)
# PUBLIC_LINK lib        - Public link library
# PRIVATE_LINK lib       - Private link library
# INTERFACE_LINK lib     - Interface link library
# PUBLIC_INCLUDE path    - Public include directory
# PRIVATE_INCLUDE path   - Private include directory
# INTERFACE_INCLUDE path - Interface include directory
#
# Examples:
# xc_add_shared_library(hello hello)
# xc_add_shared_library(hello hello NAMESPACE myproject)
# xc_add_shared_library(hello hello PUBLIC_LINK otherlib PRIVATE_INCLUDE /path/to/include)
function(xc_add_shared_library TARGET SRCDIR)
    add_library(${TARGET} SHARED)
    __xc_setup_lib(${TARGET} ${SRCDIR} ${ARGN})
    target_compile_definitions(${TARGET} PRIVATE ${CMAKE_PROJECT_NAME}_BUILD_SHARED)
endfunction(xc_add_shared_library)

# xc_add_library(TARGET SRCDIR)
# Create static or shared library based on BUILD_SHARED variable
#
# Options:
# NAMESPACE name         - Specify namespace (default: CMAKE_PROJECT_NAME)
# PUBLIC_LINK lib        - Public link library
# PRIVATE_LINK lib       - Private link library
# INTERFACE_LINK lib     - Interface link library
# PUBLIC_INCLUDE path    - Public include directory
# PRIVATE_INCLUDE path   - Private include directory
# INTERFACE_INCLUDE path - Interface include directory
#
# Examples:
# xc_add_library(hello hello)
# xc_add_library(hello hello NAMESPACE myproject)
# xc_add_library(hello hello PUBLIC_LINK otherlib PRIVATE_INCLUDE /path/to/include)
function(xc_add_library TARGET SRCDIR)
    if(${BUILD_SHARED})
        xc_add_shared_library(${TARGET} ${SRCDIR} ${ARGN})
    else()
        xc_add_static_library(${TARGET} ${SRCDIR} ${ARGN})
    endif()
endfunction(xc_add_library)

# xc_add_test(NAME SOURCES)
# Create a unit test
#
# Options:
# TARGET_OUTPUT outvar   - Output variable to receive test executable name
# PUBLIC_LINK lib        - Public link library
# PRIVATE_LINK lib       - Private link library
# INTERFACE_LINK lib     - Interface link library
# PUBLIC_INCLUDE path    - Public include directory
# PRIVATE_INCLUDE path   - Private include directory
# INTERFACE_INCLUDE path - Interface include directory
#
# Examples:
# xc_add_test(mytest test.cc)
# xc_add_test(mytest test.cc PUBLIC_INCLUDE /path/to/include)
# xc_add_test(mytest test.cc PRIVATE_LINK mylib PUBLIC_INCLUDE /path/to/include)
# xc_add_test(mytest test.cc TARGET_OUTPUT test_exe)
function(xc_add_test NAME SOURCES)
    cmake_parse_arguments(
        ARG
        ""
        "TARGET_OUTPUT"
        ""
        ${ARGN}
    )
    set(TEST_NAME "${TEST_PREFIX}${NAME}${TEST_SUFFIX}")
    add_executable(${TEST_NAME} ${SOURCES})
    target_link_libraries(${TEST_NAME} PRIVATE ${TEST_SUPPORT_LIBRARIES})
    add_test(NAME ${NAME} COMMAND ${TEST_NAME})
    __xc_link_include(${TEST_NAME} ${ARGN})

    if(NOT ARG_TARGET_OUTPUT)
        set(${ARG_TARGET_OUTPUT} ${TEST_NAME} PARENT_SCOPE)
    endif()
endfunction(xc_add_test SOURCES)

# xc_add_format_dir(DIR)
# Add directory to the list of directories to be formatted
#
# The format target will search recursively for source files
# (.hpp, .h, .hh, .c, .cc, .cpp) in all added directories
#
# Args:
#   DIR - Directory to add for formatting
#
# Examples:
#   xc_add_format_dir(${CMAKE_SOURCE_DIR}/src)
#   xc_add_format_dir(${CMAKE_SOURCE_DIR}/include)
macro(xc_add_format_dir DIR)
    get_filename_component(ABSOLUTE_DIR ${DIR} ABSOLUTE)
    list(APPEND ${CMAKE_PROJECT_NAME}_FORMAT_DIRS ${ABSOLUTE_DIR})
    message(STATUS "Add format directory: ${ABSOLUTE_DIR}")
endmacro(xc_add_format_dir)
