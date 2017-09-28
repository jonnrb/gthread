# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//arch/module.mak
# info: gnu module.mak for arch module

MODULE := arch
LIBS := switch_to
TESTS := atomic_test bit_twiddle_test switch_to_test
TEST-DEPS := $(OBJDIR)/platform/memory.o

include make_module.mak
