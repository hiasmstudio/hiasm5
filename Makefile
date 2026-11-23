CMAKE ?= cmake
BUILD_DIR ?= build/qt
CONFIG ?= Debug

.PHONY: all stage test clean
all:
	$(CMAKE) -S . -B "$(BUILD_DIR)" -DCMAKE_BUILD_TYPE=$(CONFIG)
	$(CMAKE) --build "$(BUILD_DIR)" --config $(CONFIG) --parallel

stage: all

test: all
	ctest --test-dir "$(BUILD_DIR)" -C $(CONFIG) --output-on-failure

clean:
	$(CMAKE) --build "$(BUILD_DIR)" --target clean
