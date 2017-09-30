# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//platform/module.mak
# info: gnu module.mak for platform module

MODULE := platform
LIBS := clock memory timer
TESTS := allocate_stack_test clock_test free_stack_test timer_test

include make_module.mak
