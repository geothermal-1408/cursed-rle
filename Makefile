CC ?= clang

# Base compile/link flags
CFLAGS ?= -O2 -Wall -Wextra
CPPFLAGS += -I./include -I./thirdparty

# Apple Silicon tuning
ARCH := $(shell uname -m)
ifeq ($(ARCH), arm64)
	CFLAGS += -march=armv8-a
endif
ifeq ($(ARCH), aarch64)
	CFLAGS += -march=armv8-a
endif

# Paths and output
BIN_DIR := bin
SDL_INCLUDE_DIR ?= /usr/local/include
SDL_LIB_DIR ?= /usr/local/lib
SDL_LIB := $(SDL_LIB_DIR)/libSDL3.a

# Include SDL headers for viewer build
CPPFLAGS += -I$(SDL_INCLUDE_DIR)

# macOS frameworks required by SDL3 static linking
FRAMEWORKS = -framework Cocoa \
	-framework Metal \
	-framework CoreVideo \
	-framework IOKit \
	-framework QuartzCore \
	-framework AudioToolbox \
	-framework ForceFeedback \
	-framework GameController \
	-framework Carbon \
	-framework CoreAudio \
	-framework CoreHaptics \
	-framework AVFoundation \
	-framework CoreMedia \
	-framework UniformTypeIdentifiers

# Source groups
RLE_SRCS := src/main.c src/rle_img.c src/rle_simd.c
GEN_SRCS := src/gen_img.c
VIEWER_SRCS := src/rle_sdl_viewer.c
BENCH_SRCS := src/rle_img.c tests/benchmark.c src/rle_simd.c

.PHONY: all clean dirs rle gen_img rle_viewer bench_neon bench_scalar

all: dirs rle gen_img rle_viewer

dirs:
	mkdir -p $(BIN_DIR)

rle: $(RLE_SRCS) | dirs
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(BIN_DIR)/rle $(RLE_SRCS) -lcurses -lm

gen_img: $(GEN_SRCS) | dirs
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(BIN_DIR)/gen_img $(GEN_SRCS)

rle_viewer: $(VIEWER_SRCS) | dirs
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(BIN_DIR)/rle_viewer $(VIEWER_SRCS) $(SDL_LIB) $(FRAMEWORKS) -liconv

bench_neon: $(BENCH_SRCS) | dirs
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(BIN_DIR)/benchmark_neon $(BENCH_SRCS) -lcurses
	./$(BIN_DIR)/benchmark_neon

bench_scalar: $(BENCH_SRCS) | dirs
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRLE_HAVE_NEON=0 -o $(BIN_DIR)/benchmark_scalar $(BENCH_SRCS) -lcurses
	./$(BIN_DIR)/benchmark_scalar

clean:
	rm -rf $(BIN_DIR)
