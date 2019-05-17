#!/bin/bash

# File: modify_eosio_for_mis.sh
# Author: chen chunyun
# Date: 2019/04/25
# Description: 
#		 Make changes in the [modified_list_old_new_file] arrary files,
#		 and could recovery modified by adding 'undo' option to the script when run.

function Usage()
{
	echo -e "Usage: $0 [undo|-h]\n       undo: recovery the modifies"
}

if [[ "X$1" == "X-h" ]];then
	Usage
	exit 1
fi

script_dir=$(cd $(dirname ${BASH_SOURCE[0]}); pwd )
eosio_contract_dir=$(dirname $script_dir)

set -e

# each elem format: '<old_string> <new_string> <file> [<add spaces at specific flag sides within old and new strings>]'
modified_list_old_new_file=(
	'reserve(21) reserve(2) '$eosio_contract_dir'/contracts/eosio.system/src/voting.cpp'
	'size()<21 size()<2 '$eosio_contract_dir'/contracts/eosio.system/src/voting.cpp <'
)

undo="undo"

ARCH=$( uname )
function modify_eosio_files()
{
	echo "${#modified_list_old_new_file[@]} Changed:"
	for ln in "${modified_list_old_new_file[@]}"
	do
		l=($ln)
		old="${l[0]}"
		new="${l[1]}"
		file="${l[2]}"

		
		if [[ "X$undo" == "X$1" ]]; then
			old="${l[1]}"
			new="${l[0]}"
		fi

		if [[ "X" != "X${l[3]}" ]]; then
			old="${old//${l[3]}/ ${l[3]} }"
			new="${new//${l[3]}/ ${l[3]} }"
		fi

		echo $file 
		if [ "$ARCH" == "Linux" ]; then
			sed -i "s/$old/$new/g" $file
		else
			sed -i '' "s/$old/$new/g" $file
		fi
	done
	return 0
}

# 确保是正确的eosio源码根目录
. $script_dir/check_eosio_root.sh $eosio_contract_dir contract

modify_eosio_files $1
