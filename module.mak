# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//module.mak
# info: gnu makefile for root module

MODULE :=
BINS := testy
BIN-DEPS := $(OBJDIR)/arch.a $(OBJDIR)/platform.a $(OBJDIR)/util.a $(OBJDIR)/sched.a

include make_module.mak
