#!/usr/bin/make -f
# run `make bench` to run benchmarks
# run `make USE_LIBC_PTHREAD=1` to build benchmark against libc -pthread

CFLAGS  := $(CFLAGS) -Wall -I../
LDFLAGS := $(LDFLAGS) -L../ -lmy_pthread

ifeq ($(USE_LIBC_PTHREAD), 1)
	CFLAGS += -DUSE_MY_PTHREAD=0 -pthread
endif

BENCH-TESTS := parallelCal vectorMultiply externalCal

.PHONY: all bench clean
all: $(BENCH-TESTS)

%.d: %.c
	$(CC) -MF $@ -MM -MT $@ -MT $(basename $@).o $(CFLAGS) $<

-include $(addsuffix .d,$(BENCH-TESTS))

$(addprefix run-,$(BENCH-TESTS)): run-%: % record
	time ./$<

bench: $(addprefix run-,$(BENCH-TESTS))

record: genRecord.sh
	./genRecord.sh

clean:
	$(RM) $(BENCH-TESTS) *.o *.d ./record/*
	rmdir record
