# author: Khalid Akash, JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//concur/module.mak
# info: gnu module.mak for concurrency module

MODULE := my_malloc
LIBS := mymalloc
TESTS := test
TEST-DEPS := $(OBJDIR)/sched.a $(OBJDIR)/platform.a $(OBJDIR)/arch.a $(OBJDIR)/util.a $(OBJDIR)/concur.a

include make_module.mak
