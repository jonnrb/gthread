# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//module.mak
# info: gnu makefile for root module

TESTS := testy
TEST-DEPS := $(OBJDIR)/arch.a $(OBJDIR)/platform.a $(OBJDIR)/sched.a

include make_module.mak
