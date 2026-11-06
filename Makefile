LISP ?= sbcl

ROOT_DIR   := $(CURDIR)
RAYLIB_DIR := $(ROOT_DIR)/raylib
BUILD_DIR  := $(ROOT_DIR)/build
ASSETS_DIR := $(ROOT_DIR)/assets

.PHONY: all build raylib assets

all: raylib assets build

raylib:
	$(MAKE) -C $(RAYLIB_DIR)
	ln -sf $(BUILD_DIR)/libraylib-modified.so.6.2.0 $(ROOT_DIR)/libraylib-modified.so.6.2.0

assets:
	mkdir -p $(BUILD_DIR)
	cp -r $(ASSETS_DIR) $(BUILD_DIR)/

build: raylib assets
	$(LISP) --load build.lisp
