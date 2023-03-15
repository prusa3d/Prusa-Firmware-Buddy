CC = gcc
CFLAGS = -Wall -g -fsanitize=undefined -fsanitize=address -O3

all: genrq v2test v3test

genrq: genrq.c FORCE
	$(CC) $(CFLAGS) genrq.c -I../../include -o $@

v2test: llm-v2.0/build/liblightmodbus.a FORCE
	$(CC) $(CFLAGS) v2test.c llm-v2.0/build/liblightmodbus.a -Illm-v2.0/include -o $@

v3test: FORCE
	$(CC) $(CFLAGS) v3test.c -I../../include -o $@

llm-v2.0/build/liblightmodbus.a: llm-v2.0
	cd llm-v2.0 && mkdir build && cd build && cmake .. && make

llm-v2.0:
	git clone https://github.com/jacajack/liblightmodbus $@
	cd $@ && git checkout v2.0

FORCE: