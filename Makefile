.PHONY: all clean configure build run test

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

test: build
	@./tests/run.sh

install:
	@if [ -z "$$AKX_HOME" ]; then \
		export AKX_HOME=~/.akx; \
	fi; \
	echo "Installing to $$AKX_HOME"; \
	mkdir -p $$AKX_HOME; \
	cd $(BUILD_DIR) && cmake -DAKX_HOME=$$AKX_HOME .. && cmake --install .

