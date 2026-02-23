include(FetchContent)
set(XCMIXIN_BUILD_EXAMPLES OFF)
FetchContent_Declare(
    xcoroutine
    GIT_REPOSITORY https://github.com/xcrtp/xcmixin.git
    GIT_TAG v1.0.1
    SYSTEM
)
FetchContent_MakeAvailable(xcoroutine)
