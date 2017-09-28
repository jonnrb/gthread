# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//Makefile
# info: gnu makefile for gthread

include makeutils.mak

SHELL = /bin/bash

AR = ar rcs

CC = gcc
CFLAGS = -xc -Wall -fPIC -I.

LD = gcc
LDFLAGS = -fPIE

SRCDIR = .
OBJDIR = obj
BINDIR = bin
TESTDIR = bin-test

OBJECTS :=
TEST-BINS :=
BINARIES :=
STATIC-LIBS :=



all: objs tests bins libs

# modules
include arch/module.mak
include platform/module.mak
include sched/module.mak
include module.mak
#########

objs: $(OBJECTS)

tests: $(TEST-BINS)

bins: $(BINARIES)

libs: $(STATIC-LIBS)



$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@$(call build_and_check, $(CC) $(CFLAGS) -c $< -o $@)



RUN-TEST-TARGETS := $(addprefix run-test/, $(TEST-BINS))

$(RUN-TEST-TARGETS): run-test/% : %
	@$(call run_and_check, $<)

test: $(RUN-TEST-TARGETS)



clean:
	@rm -rf $(OBJDIR)
	@rm -rf $(TESTDIR)
	@rm -rf $(BINDIR)

.PHONY: clean all test objs tests bins libs
