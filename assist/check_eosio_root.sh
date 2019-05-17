#!/bin/bash

root_dir=.
if [[ $# -ge 1 ]]; then
	root_dir=$1
fi

CHECK_DIR=$root_dir/libraries/
CHECK_FILE=$root_dir/scripts/eosio_build.sh

CHECK_CONTRACT_DIR=$root_dir/contracts/
CHECK_CONTRACT_FILE=$root_dir/build.sh

function check_eosio_root_dir()
{
	[[ ! -d "${CHECK_DIR}" || ! -f "${CHECK_FILE}" ]] && echo "Please make sure the 'assist/' dir is under eosio repo root directory(Current directory: $root_dir)!" && exit 1
	return 0
}

function check_contract_root_dir()
{
	[[ ! -d "${CHECK_CONTRACT_DIR}" || ! -f "${CHECK_CONTRACT_FILE}" ]] && echo "Please make sure the 'assist/' dir is under eosio contracts repo root directory(Current directory: $root_dir)!" && exit 1
	return 0
}

if [[ "X$2" == "Xcontract" ]];
then
	check_contract_root_dir
else
	check_eosio_root_dir
fi
