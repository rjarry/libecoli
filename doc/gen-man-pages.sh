#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Robin Jarry
#
# Generate man pages from doxygen XML using doxygen2man.
# Usage: gen-man-pages.sh <xml-dir> <include-dir> <output-dir> <stamp-file>

set -e

xml_dir="$1"
include_dir="$2"
output_dir="$3"
stamp="$4"

if [ -z "$xml_dir" ] || [ -z "$include_dir" ] || \
   [ -z "$output_dir" ] || [ -z "$stamp" ]; then
	echo "usage: $0 <xml-dir> <include-dir> <output-dir> <stamp-file>" >&2
	exit 1
fi

mkdir -p "$output_dir"

# Extract group XML filenames from the doxygen index.
groups=$(xmllint --xpath '//compound[@kind="group"]/@refid' \
	"$xml_dir/index.xml" | sed 's/ *refid="\([^"]*\)"/\1.xml/g')
if [ -z "$groups" ]; then
	echo "error: no groups found in $xml_dir/index.xml" >&2
	exit 1
fi

for xml in $groups; do
	doxygen2man -m -p libecoli -s 3 \
		-H "Libecoli Programmer's Manual" \
		-C "Olivier Matz" \
		-S "2010" \
		-d "$xml_dir" \
		-o "$output_dir" \
		"$xml"
done

# Fix include paths in generated man pages.
xpath='string(//memberdef[@kind="function"][name="%s"]/location/@file)'
for manpage in "$output_dir"/*.3; do
	[ -f "$manpage" ] || continue
	func_name=$(basename -s .3 "$manpage")
	inc=$(xmllint --xpath "$(printf "$xpath" "$func_name")" \
		"$xml_dir"/group__*.xml 2>/dev/null | xargs echo)
	if [ -n "$inc" ]; then
		sed -i "s|^\.B #include <.*>|.B #include <${inc}>|" "$manpage"
	fi
done

# Strip the systematic ", Inc. All rights reserved." suffix to all copyrights
sed -i 's/, Inc\. All rights reserved\.//' "$output_dir"/*.3

touch "$stamp"
