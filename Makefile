.PHONY: all clean configure build run

BUILD_DIR := build
BUILD_TYPE ?= Release
JOBS := $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

all: build

configure:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ..

build: configure
	@cmake --build $(BUILD_DIR) -j$(JOBS)

clean:
	@rm -rf $(BUILD_DIR)

run: build
	@$(BUILD_DIR)/bin/akx

