#!/bin/bash

set -e
#set -x

# Default block data directory on Mac X
parent_dir="$HOME/Library/Application Support/eosio/nodeos"

replay=replay

if [ "$1" != "$replay" -a "$1" != "" -a "$1" != "delete" ];then
	parent_dir="$1"
else
	# Default block data directory on Linux-like
	if [ `uname -s` == "Linux" ]; then
		parent_dir="$HOME/.local/share/eosio/nodeos"
	fi
fi

data_dir="${parent_dir}/data"
config_dir="${parent_dir}/config"
log_dir="${parent_dir}/log"

mkdir -p "${log_dir}"

log_file="${parent_dir}/log/nodeos.log"

PID=`ps aux | grep nodeos | grep -v grep | awk '{print $2}'`
if [ -n "$PID" ]
then
	kill -9 $PID
	echo "nodeos has shut down!"
fi

#nodeos=nodeos
nodeos=/Users/ywt/mis/build/programs/nodeos/nodeos
if  [ "$1" = "$replay" ];then
	nohup $nodeos --hard-replay-blockchain --config-dir "${config_dir}" --data-dir "${data_dir}" > "${log_file}" 2>&1 &
elif  [ "$1" = "$delete" ];then
	nohup $nodeos --delete-all-blocks --config-dir "${config_dir}" --data-dir "${data_dir}" > "${log_file}" 2>&1 &
else
	nohup $nodeos --config-dir "${config_dir}"  --data-dir "${data_dir}" > "${log_file}" 2>&1 &
fi

echo "go to log dir (cd \"${log_dir}\")"
