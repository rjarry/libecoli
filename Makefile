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

.PHONY: build
build: $(BUILDDIR)/build.ninja
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

.PHONY: clean
clean:
	$Q ninja -C $(BUILDDIR) clean $(ninja_opts)

meson_opts = --buildtype=$(BUILDTYPE) --werror --warnlevel=2
meson_opts += -Db_sanitize=$(SANITIZE) -Db_coverage=$(COVERAGE)
meson_opts += $(MESON_EXTRA_OPTS)

$(BUILDDIR)/build.ninja:
	meson setup $(BUILDDIR) $(meson_opts)

CLANG_FORMAT ?= clang-format
c_src = git ls-files '*.[ch]'
all_files = git ls-files ':!:subprojects'
licensed_files = git ls-files ':!:*.svg' ':!:LICENSE' ':!:*.md' ':!:todo.txt' ':!:.*'

.PHONY: lint
lint:
	@echo '[clang-format]'
	$Q $(c_src) | $(CLANG_FORMAT) --files=/proc/self/fd/0 --dry-run --Werror
	@echo '[license-check]'
	$Q ! $(licensed_files) | while read -r f; do \
		if ! grep -qF 'SPDX-License-Identifier: BSD-3-Clause' $$f; then \
			echo $$f; \
		fi; \
		if ! grep -q 'Copyright .*\<[0-9]\{4\}\>' $$f; then \
			echo $$f; \
		fi; \
	done | LC_ALL=C sort -u | grep --color . || { \
		echo 'error: files are missing license and/or copyright notice'; \
		exit 1; \
	}
	@echo '[white-space]'
	$Q $(all_files) | xargs devtools/check-whitespace
	@echo '[comments]'
	$Q $(c_src) | xargs devtools/check-comments
	@echo '[codespell]'
	$Q codespell *

.PHONY: format
format:
	@echo '[clang-format]'
	$Q $(c_src) | $(CLANG_FORMAT) --files=/proc/self/fd/0 -i --verbose

REVISION_RANGE ?= @{u}..

.PHONY: check-commits
check-commits:
	$Q devtools/check-commits $(REVISION_RANGE)

.PHONY: git-config
git-config:
	@mkdir -p .git/hooks
	@rm -f .git/hooks/commit-msg*
	ln -s ../../devtools/commit-msg .git/hooks/commit-msg

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

.PHONY: help
help:
	$Q echo 'Available targets:'
	$Q echo
	$Q echo '  build         Configure and build with default options (default target)'
	$Q echo '  debug         Configure and build with -O0 and -fsantize=address'
	$Q echo '  clean         Clean build directory'
	$Q echo '  tag-release   Create a release commit and signed tag'
	$Q echo '  tests         Run unit tests'
	$Q echo '  coverage      Run unit tests and generate test coverage report'
	$Q echo
	$Q echo 'Environment variables:'
	$Q echo
	$Q echo '  BUILDDIR            Build directory (default: $(BUILDDIR))'
	$Q echo '  BUILDTYPE           Optimization level (default: $(BUILDTYPE))'
	$Q echo '  COVERAGE            Enable coverage data in binaries (default: $(COVERAGE))'
	$Q echo '  SANITIZE            Value of -fsanitize (default: $(SANITIZE))'
	$Q echo '  MESON_EXTRA_OPTS    Additional options for meson setup'
