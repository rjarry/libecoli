#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019, Olivier MATZ <zer0@droids-corp.org>

set -e

parse_yaml=ecoli-parse-yaml
libecoli=/usr/share/libecoli/ecoli.sh

usage()
{
        cat <<- END_OF_HELP
        This example demonstrates how to build an interactive command
        line in a shell script, using a description in yaml format.

        Usage: $0 [options...] [tests...]
            -h
               show this help
            -p <path>
               path to ecoli-parse-yaml binary (default is to search
               in PATH).
            -l <path>
               path to libecoli.sh library (default is $libecoli)
END_OF_HELP
}

ARG=0
while getopts "hp:l:" opt; do
        case "$opt" in
        "h")
                usage
                exit 0
                ;;
        "p")
                parse_yaml="$OPTARG"
                ;;
        "l")
                libecoli="$OPTARG"
                ;;
	esac
done

if ! command -v $parse_yaml > /dev/null 2> /dev/null && \
		! readlink -e $parse_yaml > /dev/null 2> /dev/null; then
	echo "Cannot find ecoli-parse-yaml ($parse_yaml)"
	exit 1
fi
if ! readlink -e $libecoli > /dev/null 2> /dev/null; then
	echo "Cannot find libecoli.sh ($libecoli)"
	exit 1
fi

. $libecoli

yaml=$(mktemp)
cat << EOF > $yaml
type: or
children:
- type: seq
  id: hello
  help: Say hello to someone
  children:
  - type: str
    string: hello
  - type: or
    id: name
    help: Name of the person to greet
    children:
    - type: str
      string: john
    - type: str
      string: mike
- type: seq
  id: goodbye
  help: Say good bye to someone
  children:
  - type: str
    string: good
  - type: str
    string: bye
  - type: or
    id: name
    help: Name of the person to greet
    children:
    - type: str
      string: mary
    - type: str
      string: jessica
EOF

output=$(mktemp)
match=1
$parse_yaml -i $yaml -o $output || match=0
if [ "$match" = "1" ]; then
	cat $output
	. $output
	name=$(ec_pnode_get_str $(ec_pnode_find_first ec_node1 name) 0)
	hello=$(ec_pnode_get_str $(ec_pnode_find_first ec_node1 hello) 0)

	if [ "$hello" != "" ]; then
		echo "$name says hello to you!"
	else
		echo "$name says good bye to you!"
	fi
else
	echo "no match"
fi
rm $output
rm $yaml
