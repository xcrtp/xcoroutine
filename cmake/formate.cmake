if(${CMAKE_PROJECT_NAME}_FORMAT_DIRS)
    find_program(FORMAT_EXE NAMES clang-format)

    if(NOT FORMAT_EXE)
        message(FATAL_ERROR "clang-format not found")
    endif()

    foreach(DIR ${${CMAKE_PROJECT_NAME}_FORMAT_DIRS})
        file(GLOB_RECURSE SRC
            "${DIR}/*.hpp"
            "${DIR}/*.h"
            "${DIR}/*.hh"
            "${DIR}/*.c"
            "${DIR}/*.cc"
            "${DIR}/*.cpp")
        list(APPEND FORMAT_FILES ${SRC})
    endforeach()

    if(FORMAT_FILES)
        message(STATUS "add format target")
        add_custom_target(
            format
            COMMAND ${FORMAT_EXE} -i ${FORMAT_FILES}
            COMMENT "Formatting all sources"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endif()
endif()