# author: Khalid Akash, JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//concur/module.mak
# info: gnu module.mak for concurrency module

MODULE := concur
LIBS := mutex
TESTS := mutex_test
TEST-DEPS := $(OBJDIR)/sched.a $(OBJDIR)/my_malloc.a $(OBJDIR)/platform.a $(OBJDIR)/arch.a $(OBJDIR)/util.a

include make_module.mak
