CC = clang
CFLAGS = -O2 -Wall -Wextra -I./include -I./thirdparty

ARCH := $(shell uname -m)
ifeq ($(ARCH), arm64)
	CFLAGS += -march=armv8-a
endif
ifeq ($(ARCH), aarch64)
	CFLAGS += -march=armv8-a
endif

SRCS_RLE = src/main.c src/rle_img.c src/rle_simd.c

SRCS_GEN = src/gen_img.c

.PHONY: all clean

all:rle gen_img

gen_img: $(SRCS_GEN)
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/gen_img $(SRCS_GEN)

rle: $(SRCS_RLE)
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/rle $(SRCS_RLE) -lcurses -lm

test: tests/test_simd.c src/rle_simd.c
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/test_simd src/rle_img.c tests/test_simd.c src/rle_simd.c -lcurses
	./bin/test_simd

bench_neon: tests/benchmark.c src/rle_simd.c
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/benchmark_neon src/rle_img.c tests/benchmark.c src/rle_simd.c -lcurses
	./bin/benchmark_neon

bench_scalar: tests/benchmark.c src/rle_simd.c
	mkdir -p bin
	$(CC) $(CFLAGS) -DRLE_HAVE_NEON=0 -o bin/benchmark_scalar src/rle_img.c tests/benchmark.c src/rle_simd.c -lcurses
	./bin/benchmark_scalar

clean:
	rm -rf bin
