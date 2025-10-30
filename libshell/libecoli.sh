# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018, Olivier MATZ <zer0@droids-corp.org>

# Source this file from a shell script to add libecoli helpers.

# use a safer version of echo (no option)
ec_echo()
{
	printf "%s\n" "$*"
}

ec_debug()
{
	ec_echo "$@" >&2
}

# $1: node sequence number (ex: ec_node4)
ec_pnode_get_first_child()
{
	local first_child=${1}_first_child
	ec_echo $(eval 'ec_echo ${'$first_child'}')
}

# $1: node sequence number (ex: ec_node4)
ec_pnode_get_next()
{
	local next=${1}_next
	ec_echo $(eval 'ec_echo ${'$next'}')
}

# $1: node sequence number (ex: ec_node4)
ec_pnode_iter_next()
{
	local seq=${1#ec_node}
	seq=$((seq+1))
	local next=ec_node${seq}
	if [ "$(ec_pnode_get_type $next)" != "" ]; then
		ec_echo $next
	fi
}

# $1: node sequence number (ex: ec_node4)
ec_pnode_get_id()
{
	local id=${1}_id
	ec_echo $(eval 'ec_echo ${'$id'}')
}

# $1: node sequence number (ex: ec_node4)
ec_pnode_get_type()
{
	local type=${1}_type
	ec_echo $(eval 'ec_echo ${'$type'}')
}

# $1: node sequence number (ex: ec_node4)
ec_pnode_get_strvec_len()
{
	local strvec_len=${1}_strvec_len
	ec_echo $(eval 'ec_echo ${'$strvec_len'}')
}

# $1: node sequence number (ex: ec_node4)
# $2: index in strvec
ec_pnode_get_str()
{
	if [ $# -ne 2 ]; then
		return
	fi
	local str=${1}_str${2}
	ec_echo $(eval 'ec_echo ${'$str'}')
}

# $1: node sequence number (ex: ec_node4)
# $2: node id (string)
ec_pnode_find_first()
{
	if [ $# -ne 2 ]; then
		return
	fi
	local node_seq=$1
	while [ "$node_seq" != "" ]; do
		local id=$(ec_pnode_get_id $node_seq)
		if [ "$id" = "$2" ]; then
			ec_echo $node_seq
			return 0
		fi
		node_seq=$(ec_pnode_iter_next $node_seq)
	done
}
