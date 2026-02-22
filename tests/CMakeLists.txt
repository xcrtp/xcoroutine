function(add_sample_test name)
    set(target "test_${name}")
    add_executable(${target} ${name}.cc)
    target_link_libraries(${target} PRIVATE ${ARGN} ${TEST_SUPPORT_LIBRARIES})
    add_test(NAME ${name} COMMAND ${target})
endfunction(add_sample_test)

add_sample_test(access xcoroutine)
