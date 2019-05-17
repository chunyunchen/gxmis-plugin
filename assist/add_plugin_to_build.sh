#!/bin/bash

#set -o errexit
#set -x

script_dir=$(cd $(dirname ${BASH_SOURCE[0]}); pwd )
eosio_root=$(dirname $script_dir)


MAKELIST="CMakeLists.txt"
path_nodeos_cmakelist_file="${eosio_root}/programs/nodeos/${MAKELIST}"
path_plugins_cmakelist_file="${eosio_root}/plugins/${MAKELIST}"
path_contracts_cmakelist_file="${eosio_root}/contracts/${MAKELIST}"

PLUGINS=("data_exchange_api_plugin" "data_exchange_plugin")
CONTRACTS=()

ARCH=$( uname )

function add_plugin_to_cmakelist()
{
	for plugin in ${PLUGINS[@]}
	do
		grep -q "${plugin}" ${path_nodeos_cmakelist_file}
		if [ $? -ne 0 ]; then
			if [ "$ARCH" == "Linux" ]; then
				sed -i -e '/PRIVATE appbase/a\'$'\n''        PRIVATE -Wl,${whole_archive_flag} '${plugin}'        -Wl,${no_whole_archive_flag}' ${path_nodeos_cmakelist_file}
			else
				sed -i "" -e '/PRIVATE appbase/a\'$'\n''        PRIVATE -Wl,${whole_archive_flag} '${plugin}'        -Wl,${no_whole_archive_flag}' ${path_nodeos_cmakelist_file}
			fi
		fi
		
		grep -q "${plugin}" ${path_plugins_cmakelist_file}
		if [ $? -ne 0 ]; then
			if [ "$ARCH" == "Linux" ]; then
				sed -i -e '/add_subdirectory(chain_plugin)/a\'$'\n''add_subdirectory('${plugin}')' ${path_plugins_cmakelist_file}
			else
				sed -i "" -e '/add_subdirectory(chain_plugin)/a\'$'\n''add_subdirectory('${plugin}')' ${path_plugins_cmakelist_file}
			fi
		fi
	done
	return 0
}
		
function add_contract_to_cmakelist()
{
	for ct in ${CONTRACTS[@]}
	do
		grep -q "${ct}" ${path_contracts_cmakelist_file}
		if [ $? -ne 0 ]; then
			if [ "$ARCH" == "Linux" ]; then
				sed -i -e '/add_subdirectory(eosiolib)/a\'$'\n''add_subdirectory('${ct}')' ${path_contracts_cmakelist_file}
			else
				sed -i "" -e '/add_subdirectory(eosiolib)/a\'$'\n''add_subdirectory('${ct}')' ${path_contracts_cmakelist_file}
			fi
		fi
	done
	return 0
}

# 确保脚本是操作eosio源码根目录下的文件
. $script_dir/check_eosio_root.sh $eosio_root

add_plugin_to_cmakelist
add_contract_to_cmakelist
