# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//Makefile
# info: gnu makefile for gthread

include makeutils.mak



local_CFLAGS := -xc -std=c11 -Wall -fPIC -I.
local_LDFLAGS := -lm



SRCDIR = .
OBJDIR = obj
BINDIR = bin
TESTDIR = bin-test

OBJECTS :=
TEST-BINS :=
BINARIES :=
LIB-OBJECTS :=
STATIC-LIBS :=



all: libmy_pthread.a objs tests bins libs

# modules
include arch/module.mak
include concur/module.mak
include platform/module.mak
include sched/module.mak
include util/module.mak
#########

objs: $(OBJECTS)

tests: $(TEST-BINS)

bins: $(BINARIES)

libs: $(STATIC-LIBS)



libmy_pthread.a: $(LIB-OBJECTS)
	@$(call build_and_check, $(AR) rcs $@ $^)

bench: libmy_pthread.a
	@cd yujie_benchmark             && \
		make clean && make            && \
		./genRecord.sh                && \
		echo "Running externalCal"    && \
		time ./externalCal            && \
		echo "Running parallelCal"    && \
		time ./parallelCal            && \
		echo "Running vectorMultiply" && \
		time ./vectorMultiply



DEPFILES = $(OBJECTS:.o=.d)

$(DEPFILES): $(OBJDIR)/%.d : $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) -MF $@ -MM -MT $@ -MT $(basename $@).o -I$(SRCDIR) $<

-include $(DEPFILES)



$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c $(OBJDIR)/%.d
	@mkdir -p $(dir $@)
	@$(call build_and_check, $(CC) $(local_CFLAGS) $(CFLAGS) -c $< -o $@)



RUN-TEST-TARGETS := $(addprefix run-test/, $(TEST-BINS))

$(RUN-TEST-TARGETS): run-test/% : %
	@$(call run_and_check, $<)

test: $(RUN-TEST-TARGETS)



clean:
	@rm -rf $(OBJDIR)
	@rm -rf $(TESTDIR)
	@rm -rf $(BINDIR)

.PHONY: clean all test objs tests bins libs
