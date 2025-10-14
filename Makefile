# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 Robin Jarry

BUILDDIR ?= build
BUILDTYPE ?= debugoptimized
SANITIZE ?= none
V ?= 0
ifeq ($V,1)
ninja_opts = --verbose
Q =
else
Q = @
endif

.PHONY: all
all: $(BUILDDIR)/build.ninja
	$Q ninja -C $(BUILDDIR) $(ninja_opts)

.PHONY: debug
debug: BUILDTYPE = debug
debug: SANITIZE = address
debug: all

.PHONY: all
clean:
	$Q ninja -C $(BUILDDIR) clean $(ninja_opts)

.PHONY: install
install: $(BUILDDIR)/build.ninja
	$Q meson install -C $(BUILDDIR)

meson_opts = --buildtype=$(BUILDTYPE) --werror --warnlevel=2 -Db_sanitize=$(SANITIZE)
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
