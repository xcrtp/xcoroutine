find_package(GTest CONFIG)

if(NOT GTest_FOUND)
    if(WIN32)
        set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib" FORCE)
    endif()

    set(CMAKE_CXX_FLAGS_TMP "${CMAKE_CXX_FLAGS}")

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-character-conversion")
    endif()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        SYSTEM
    )
    FetchContent_MakeAvailable(googletest)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_TMP}")
endif()

set(TEST_SUPPORT_LIBRARIES
    GTest::gtest GTest::gtest_main
)
enable_testing()
