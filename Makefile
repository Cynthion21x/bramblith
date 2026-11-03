LISP ?= sbcl

.PHONY: build

build:
	$(LISP) --load build.lisp
