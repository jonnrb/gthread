# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//sched/module.mak
# info: gnu module.mak for sched module

MODULE := concur
LIBS := mutex
TESTS := mutex_test
TEST-DEPS := $(OBJDIR)/sched.a $(OBJDIR)/platform.a $(OBJDIR)/arch.a $(OBJDIR)/util.a

include make_module.mak
