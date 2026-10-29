BUILD_DIR ?= build
CC = clang-20
CXX = clang++-20
ARGS ?=

.PHONY: all build test run reconfigure clean

all: build

$(BUILD_DIR)/build.ninja:
	CC=$(CC) CXX=$(CXX) meson setup $(BUILD_DIR)

build: $(BUILD_DIR)/build.ninja
	meson compile -C $(BUILD_DIR)

test: build
	meson test -C $(BUILD_DIR) --verbose

run: build
	./$(BUILD_DIR)/app/cli $(ARGS)

# Recreate the build directory, e.g. after moving the project or changing compiler.
reconfigure:
	rm -rf $(BUILD_DIR)
	CC=$(CC) CXX=$(CXX) meson setup $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
