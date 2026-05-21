#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Robin Jarry
#
# Install generated man pages. Called by meson.add_install_script().

set -e

man_dir="$1"
mandir="$2"

# mandir may be relative (e.g. "share/man"), resolve against install prefix.
case "$mandir" in
/*)
	dest="${DESTDIR}${mandir}/man3"
	;;
*)
	dest="${MESON_INSTALL_DESTDIR_PREFIX}/${mandir}/man3"
	;;
esac

install -m 644 -Dvt "$dest" "$man_dir"/*.3
