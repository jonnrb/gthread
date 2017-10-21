# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//util/module.mak
# info: gnu module.mak for util module

MODULE := util
LIBS := rb list
TESTS := rb_test list_test
TEST-DEPS := $(OBJDIR)/platform.a

include make_module.mak
