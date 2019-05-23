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
# load tool functions
. $script_dir/utils.sh

wallet_pwd_file=$wallet_pwd_dir/$WALLET_NAME.$wallet_pwd_file_ext
wlt_pwd=$(cat $wallet_pwd_file)
http_server_address="http://""$(cat $parent_workspace_dir/eosio/conf/http_server_address)"

function transfer_test()
{
    quantity=0.0001
    sender_balance_before=$($cleos get currency balance eosio.token ${test_accounts[0]} $core_symbole_name | awk '{print $1}')
	res=$(curl -X POST $http_server_address/v1/exchange/push_action -d '{
	"wltpwd":"'"$WALLET_NAME"' : '"$wlt_pwd"'",
	"account":"eosio.token",
	"action_name":"transfer",
	"action_args":"'"${test_accounts[0]}"','"${test_accounts[1]}"','"$quantity"' '"$core_symbole_name"',memo"
	"permissions":["'"${test_accounts[0]}"'@active"]
	}')
	res=${res//\\/}
	res=${res/\"\{/\{}
	res=${res/\}\"/\}}
	
	res=$(curl -X POST $http_server_address/v1/chain/push_transaction -d ''"$res"'')
	echo $res | grep -qiw "executed" || exit 1

    sender_balance_after=$($cleos get currency balance eosio.token ${test_accounts[0]} $core_symbole_name | awk '{print $1}')
    balance_add=$(awk -v x=$sender_balance_after -v y=$quantity 'BEGIN{printf "%.4f\n",x+y}')
    [[ "$sender_balance_before" == "$balance_add" ]] || exit 1
}

function limit_order_test()
{
	loop_num=$1
	for((i=1; i<=$loop_num; i++))
	do
		idx=$((2 - $i % 2))
		oper=$(($i % 2))
		quantity=$(($RANDOM % 37 + 1))
		price="$(($RANDOM % 27))"".0103 RMB"

		res=$(curl -X POST $http_server_address/v1/exchange/push_action -d '{
		"wltpwd":"'"$WALLET_NAME"' : '"$wlt_pwd"'",
		"account":"mis.exchange",
		"action_name":"lmto",
		"action_args":"'"${test_accounts[$idx]}"',token,'"$core_symbole_name"','"$quantity"','"$price"','"$oper"'"
		"permissions":["'"${test_accounts[$idx]}"'@active"]
		}')

		res=${res//\\/}
		res=${res/\"\{/\{}
		res=${res/\}\"/\}}
		
		res=$(curl -X POST $http_server_address/v1/chain/push_transaction -d ''"$res"'')
		echo $res | grep -qiw "executed" || exit 1
	done
}

#transfer_test
limit_order_test 2