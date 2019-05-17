#!/bin/bash

set -e
set -x

script_dir=$(cd $(dirname ${BASH_SOURCE[0]}); pwd )
jenkins_base_dir=$(dirname $JENKINS_HOME)
wallet_pwd_dir=$jenkins_base_dir/eosio-wallet
wallet_pwd_file_ext="pwd"

parent_workspace_dir=$(dirname $WORKSPACE)
pubkey_prefix=$(cat $parent_workspace_dir/eosio/conf/pubkey_prefix)
core_symbole_name=$(cat $parent_workspace_dir/eosio/conf/core_symbol_name)
cleos=$parent_workspace_dir/eosio/bin/cleos

test_accounts=()
. $script_dir/pre_test.sh

wallet_pwd_file=$wallet_pwd_dir/$WALLET_NAME.$wallet_pwd_file_ext
wlt_pwd=$(cat $wallet_pwd_file)
http_server_address="http://""$(cat $parent_workspace_dir/eosio/conf/http_server_address)"

function push_action_test()
{
	res=$(curl -X POST $http_server_address/v1/exchange/push_action -d '{
	"wallet":{
		"wallet_name":"'"$WALLET_NAME"'",
		"wallet_pwd":"'"$wlt_pwd"'"
	},
	"account":"eosio.token",
	"action_name":"transfer",
	"action_args":"'"${test_accounts[0]}"','"${test_accounts[1]}"',0.0001 '"$core_symbole_name"',memo"
	"permissions":["'"${test_accounts[0]}"'@active"]
	}')
	res=${res//\\/}
	res=${res/\"\{/\{}
	res=${res/\}\"/\}}
	
	res=$(curl -X POST $http_server_address/v1/chain/push_transaction -d ''"$res"'')
	echo $res | grep -qiw "executed" || exit 1
}

push_action_test
