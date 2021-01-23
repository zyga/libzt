.PHONY: all clean

CFLAGS ?= -Wall -Werror -O2
LDLIBS += -lzt

all: demo test-root-user bench-sqrt
bench-sqrt: LDLIBS += -lm
clean:
	rm -f *.o demo test-root-user bench-sqrt
