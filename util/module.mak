# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//util/module.mak
# info: gnu module.mak for util module

MODULE := util
LIBS := rb
TESTS := rb_test
TEST-DEPS := $(OBJDIR)/platform.a

include make_module.mak
