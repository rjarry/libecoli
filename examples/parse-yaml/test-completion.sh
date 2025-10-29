# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019, Olivier MATZ <zer0@droids-corp.org>

# complete -F _parser_options dummy_command

_parser_options()
{
	local curr_arg
	local i

	curr_arg=${COMP_WORDS[COMP_CWORD]}

	# remove the command name, only keep args
	words=( "${COMP_WORDS[@]}" )
	unset words[0]

	IFS=$'\n' read -d '' -r -a COMPREPLY <<- EOF
	$(./build/parse-yaml --complete -i examples/yaml/test.yaml -o pipo -- "${words[@]}")
	EOF
}
