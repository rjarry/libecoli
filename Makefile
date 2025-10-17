# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 Robin Jarry

BUILDDIR ?= build
BUILDTYPE ?= debugoptimized
SANITIZE ?= none
COVERAGE ?= false
V ?= 0
ifeq ($V,1)
ninja_opts = --verbose
Q =
else
Q = @
endif
CC ?= gcc

.PHONY: all
all: $(BUILDDIR)/build.ninja
	$Q ninja -C $(BUILDDIR) $(ninja_opts)

.PHONY: debug
debug: BUILDTYPE = debug
debug: SANITIZE = address
debug: COVERAGE = true
debug: all

.PHONY: tests
tests: $(BUILDDIR)/build.ninja
	$Q meson test -C $(BUILDDIR) --print-errorlogs $(if $(filter 1,$V),--verbose)

.PHONY: coverage
coverage: tests
	$Q mkdir -p $(BUILDDIR)/coverage
	gcovr --html-details $(BUILDDIR)/coverage/index.html --txt \
		-e 'test/*' -e 'examples/*' --gcov-ignore-parse-errors \
		--gcov-executable `$(CC) -print-prog-name=gcov` \
		--object-directory $(BUILDDIR) \
		--sort uncovered-percent \
		-r . $(BUILDDIR)
	@echo Coverage data is present in $(BUILDDIR)/coverage/index.html

.PHONY: all
clean:
	$Q ninja -C $(BUILDDIR) clean $(ninja_opts)

.PHONY: install
install: $(BUILDDIR)/build.ninja
	$Q meson install -C $(BUILDDIR)

meson_opts = --buildtype=$(BUILDTYPE) --werror --warnlevel=2
meson_opts += -Db_sanitize=$(SANITIZE) -Db_coverage=$(COVERAGE)
meson_opts += $(MESON_EXTRA_OPTS)

$(BUILDDIR)/build.ninja:
	meson setup $(BUILDDIR) $(meson_opts)

.PHONY: tag-release
tag-release:
	@cur_version=`sed -En "s/^[[:space:]]+version[[:space:]]*:[[:space:]]*'([0-9\.]+)'\\$$/\\1/p" meson.build` && \
	next_version=`echo $$cur_version | awk -F. -v OFS=. '{$$(NF) += 1; print}'` && \
	read -rp "next version ($$next_version)? " n && \
	if [ -n "$$n" ]; then next_version="$$n"; fi && \
	set -xe && \
	sed -i "s/\<$$cur_version\>/$$next_version/" meson.build && \
	git commit -sm "release: version $$next_version" -m "`git shortlog -n v$$cur_version..`" meson.build && \
	git tag -sm "`git shortlog -n v$$cur_version..HEAD^`" "v$$next_version"
