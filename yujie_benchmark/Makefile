# run `make bench` to run benchmarks
# run `make USE_LIBC_PTHREAD=1` to build benchmark against libc -pthread

CFLAGS  := $(CFLAGS) -Wall -I../
LDFLAGS := $(LDFLAGS) -L../
LDLIBS  := -lmy_pthread

ifeq ($(USE_LIBC_PTHREAD), 1)
	CFLAGS += -DUSE_MY_PTHREAD=0 -pthread
	LDLIBS += -lpthread
endif

# these are the benchmark tests
BENCH-TESTS := parallelCal vectorMultiply externalCal

.PHONY: all bench clean run-*
all: $(BENCH-TESTS)

# create deps files to detect if ../my_pthread_t.h changes
%.d: %.c
	$(CC) -MF $@ -MM -MT $@ -MT $(basename $@).o $(CFLAGS) $<

-include $(addsuffix .d,$(BENCH-TESTS))

$(addprefix run-,$(BENCH-TESTS)): run-%: % record
	bash -c "time ./$<"

bench: $(addprefix run-,$(BENCH-TESTS))

record: genRecord.sh
	./genRecord.sh

clean:
	$(RM) $(BENCH-TESTS) *.o *.d ./record/*
	$(RM) -r record
