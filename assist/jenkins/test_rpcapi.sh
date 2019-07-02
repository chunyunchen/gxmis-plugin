#!/bin/bash

set -e

script_dir=$(cd $(dirname ${BASH_SOURCE[0]}); pwd )
jenkins_base_dir=$(dirname $JENKINS_HOME)
wallet_pwd_dir=$jenkins_base_dir/eosio-wallet
wallet_pwd_file_ext="pwd"

parent_workspace_dir=$(dirname $WORKSPACE)
pubkey_prefix=$(cat $parent_workspace_dir/eosio/conf/pubkey_prefix)
core_symbole_name=$(cat $parent_workspace_dir/eosio/conf/core_symbol_name)
cleos=$parent_workspace_dir/eosio/bin/cleos
nodeos=$parent_workspace_dir/eosio/bin/nodeos

http_server_address="http://""$(cat $parent_workspace_dir/eosio/conf/http_server_address)"

test_accounts=()
all_pubkeys_in_wallet=()
. $script_dir/pre_test.sh

# load tool functions
. $script_dir/utils.sh

wallet_pwd_file=$wallet_pwd_dir/$WALLET_NAME.$wallet_pwd_file_ext
wlt_pwd=$(cat $wallet_pwd_file)

function transfer_test()
{
    quantity=0.0001
    sender_balance_before=$($cleos get currency balance eosio.token ${test_accounts[0]} $core_symbole_name | awk '{print $1}')
    quantity_integer_decimal=(${quantity%%.*} ${quantity##*.})
    sender_balance_before_integer_decimal=(${sender_balance_before%%.*} ${sender_balance_before##*.})
	res=$(curl -X POST $http_server_address/v1/exchange/push_action -d '{
	"wltpwd":"'"$WALLET_NAME#$wlt_pwd"'",
	"account":"eosio.token",
	"action_name":"transfer",
	"action_args":"'"${test_accounts[0]}"','"${test_accounts[1]}"','"$quantity"' '"$core_symbole_name"',memo"
	"permissions":["'"${test_accounts[0]}"'@active"]
	}')

	echo $res | grep -qiw "executed" || exit 1

    sender_balance_after=$($cleos get currency balance eosio.token ${test_accounts[0]} $core_symbole_name | awk '{print $1}')
    sender_balance_after_integer_decimal=(${sender_balance_after%%.*} ${sender_balance_after##*.})
    balance_add[0]=$((${sender_balance_after_integer_decimal[0]}+${quantity_integer_decimal[0]}))
    balance_add[1]=$((${sender_balance_after_integer_decimal[1]}+${quantity_integer_decimal[1]}))
    # 进位
    if [[ ${balance_add[1]} -ge 10000 ]]; then
        balance_add[0]=$((${balance_add[0]}+1))
        balance_add[1]=$((${balance_add[1]}-10000))
    fi

    [[ ${sender_balance_before_integer_decimal[0]} -eq ${balance_add[0]} && ${sender_balance_before_integer_decimal[1]} -eq ${balance_add[1]} ]] || exit 1
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
		"wltpwd":"'"$WALLET_NAME#$wlt_pwd"'",
		"account":"mis.exchange",
		"action_name":"lmto",
		"action_args":"'"${test_accounts[$idx]}"',token,'"$core_symbole_name"','"$quantity"','"$price"','"$oper"'"
		"permissions":["'"${test_accounts[$idx]}"'@active"]
		}')

		echo $res | grep -qiw "executed" || exit 1
	done
}

function swlt_test()
{
	res=$(curl -X POST $http_server_address/v1/exchange/push_action -d '{
	"wltpwd":"'"$WALLET_NAME"'#'"$wlt_pwd"'",
	"account":"mis.exchange",
	"action_name":"swlt",
	"action_args":"'"${test_accounts[0]}"','"${WALLET_NAME}#${wlt_pwd}"'"
	"permissions":["'"${test_accounts[0]}"'@active"]
	}')

    set +x
	echo $res | grep -qiw "executed" || exit 1
    set -x

}

function random_acct()
{
    set +x

    letters="12345abcdefghijklmnopqrstuvwxyz"
    acct_fixed_len=${1:-12}

    acct=""
    for(( i=1; i<=$acct_fixed_len; i++))
    do
        pos=$(($RANDOM % ${#letters}))
        acct="$acct""${letters:$pos:1}"
    done

    echo $acct

    set -x
}

function account_create()
{
    acct=$1
    opk=$2

	action_args="${acct},${opk}"
    
    if [[ "X" != "X$3" ]]; then
	    action_args="${acct},${opk},${3}"
    fi

	res=$(curl -X POST $http_server_address/v1/exchange/push_action -d '{
	"wltpwd":"'"$WALLET_NAME#$wlt_pwd"'",
	"account":"mis.exchange",
	"action_name":"newaccount",
	"action_args":"'"${action_args}"'"
	"permissions":["mis.exchange@active"]
	}')

    set +x
	echo $res | grep -qiw "executed" || exit 1
    set -x

}

function newaccount_test()
{
    set +x
    accts=(
        $(random_acct 12)
        $(random_acct 11)
        $(random_acct 10)
        $(random_acct 9)
        $(random_acct 8)
        $(random_acct 7)
        $(random_acct 6)
        $(random_acct 5)
        $(random_acct 4)
        $(random_acct 3)
        $(random_acct 2)
        $(random_acct 1)
        "$(random_acct 9)"".cn"
        "$(random_acct 7)"".com"
        )
    set -x

    n=${1:-2}
    pk_n=${#all_pubkeys_in_wallet[@]}
    a_n=${#accts[@]}
    for((i=1; i<=2; i++))
    do
        # 指定owner权限的key
        ki=$(($RANDOM % $pk_n))
        keys[0]=${all_pubkeys_in_wallet[$ki]}
        
        # 指定active权限的key
        one_more_key=$(($i % 2))
        ki=$((($ki+$one_more_key) % $pk_n))
        if [[ 1 -eq $one_more_key ]]; then
            keys[1]=${all_pubkeys_in_wallet[$ki]}
        else
            # 清空当前的值，否则前面执行结果会保存下来影响当前
            keys[1]=
        fi
        
        ai=$(($RANDOM % $a_n))
        account_create ${accts[$ai]} ${keys[@]} 
    done
}

function show_blockchain_info()
{
    curl -X POST $http_server_address/v1/chain/get_info
}

set -x
transfer_test
#limit_order_test 2
#swlt_test
newaccount_test
show_blockchain_info
set +x
