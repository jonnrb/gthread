# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//sched/module.mak
# info: gnu module.mak for sched module

MODULE := sched
LIBS := sched task
TESTS := sched_test
TEST-DEPS := $(OBJDIR)/my_malloc.a $(OBJDIR)/util.a $(OBJDIR)/platform.a $(OBJDIR)/arch.a

include make_module.mak
