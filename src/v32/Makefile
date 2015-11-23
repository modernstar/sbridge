# use gcc
CC=gcc

# compilation errors if V32_NO_VITERBI is not defined
#CFLAGS  = -DV32_NO_VITERBI
# we're building a shared object, we need this
CFLAGS = -fPIC

v32objects := $(patsubst %.c,%.o,$(wildcard *.c))

all: libv32.so
	@echo "Built libv32.so"

libv32.so: $(v32objects)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

clean:
	rm -f libv32.so
	rm -f *.o
