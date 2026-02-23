add_library(xcoroutine INTERFACE)

target_include_directories(xcoroutine
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(xcoroutine
    INTERFACE
    xcmixin::xcmixin
)
