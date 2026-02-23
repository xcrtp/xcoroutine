find_program(GII NAMES git)

function(xc_get_last_git_tag TAG_VAR DEFAULT_VALUE)
    if(NOT GII)
        message(FATAL_ERROR "git not found")
    endif()

    set(${TAG_VAR} ${DEFAULT_VALUE} PARENT_SCOPE)

    execute_process(
        COMMAND ${GII} describe --abbrev=0 --tags
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(GIT_TAG)
        message(STATUS "GIT_TAG: ${GIT_TAG}")
        set(${TAG_VAR} ${GIT_TAG} PARENT_SCOPE)
    endif()
endfunction(xc_get_last_git_tag)

function(xc_get_project_version VERSION_VAR)
    xc_get_last_git_tag(GIT_TAG "v0.0.0")
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" VERSION ${GIT_TAG})
    if(NOT VERSION)
        message(FATAL_ERROR "git tag ${GIT_TAG} not match version format")
    endif()
    set(${VERSION_VAR} ${VERSION} PARENT_SCOPE)
endfunction(xc_get_project_version)