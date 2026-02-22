find_program(FORMAT_EXE NAMES clang-format)

if(NOT FORMAT_EXE)
    message(FATAL_ERROR "clang-format not found")
endif()

set(FORMAT_DIRS ${CMAKE_SOURCE_DIR}/xc/)
foreach(DIR ${FORMAT_DIRS})
    file(GLOB_RECURSE SRC "${DIR}/*.hpp" "${DIR}/*.cc")
    list(APPEND FORMAT_FILES ${SRC})
endforeach()

add_custom_target(
    format
    COMMAND ${FORMAT_EXE} -i ${FORMAT_FILES}
    COMMENT "Formatting all sources"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)