xc_add_test(access access.cc
    PRIVATE_LINK xcoroutine)

xc_add_test(spin_lock spin_lock.cc
    PRIVATE_LINK xcoroutine)