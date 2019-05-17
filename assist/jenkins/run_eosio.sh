#!/bin/bash

set -e
set -x

script_dir=$(cd $(dirname ${BASH_SOURCE[0]}); pwd )
jenkins_dir=$(dirname $script_dir)
assist_dir=$(dirname $jenkins_dir)
parent_dir=$(dirname $assist_dir)

data_dir="${parent_dir}/nodeos_data/data"
config_dir="${parent_dir}/nodeos_data/config"
log_dir="${parent_dir}/nodeos_data/log"
log_file="${log_dir}/nodeos.log"

nodeos=$assist_dir/build/bin/nodeos
function run_nodeos()
{
  if  [[ "$START_MODE" = "restart" ]];then
	$nodeos --hard-replay-blockchain --config-dir "${config_dir}" --data-dir "${data_dir}"
  elif  [[ "$START_MODE" = "cleanStart" ]];then
	$nodeos --delete-all-blocks --config-dir "${config_dir}" --data-dir "${data_dir}"
  elif  [[ "$START_MODE" = "genesis" || "$START_MODE" = "removeData" ]];then
	if [[ "$START_MODE" = "removeData" ]];then
		rm -rf $data_dir/*
	fi
	$nodeos --config-dir "${config_dir}"  --data-dir "${data_dir}"
  fi
}

cleos=$assist_dir/build/bin/cleos
function run_keosd()
{
  # will run keosd process automatically at background
  $cleos wallet list
}

run_keosd
run_nodeos
