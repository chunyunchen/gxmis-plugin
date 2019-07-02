#!/bin/bash

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

contract_user=${test_accounts[0]}
sha256_buyer=""
sha256_seller=""
cleos_option="-l 1000 --show-payer"
cleos_multi_perms="-p ${test_accounts[1]} -p ${test_accounts[2]} -p $contract_user"
table_scope="dataex"

set -x
buy_quantity=$(($RANDOM % 37 + 1))
sell_quantity=$(($RANDOM % 37 + 1))
buy_price="$(($RANDOM % 37 + 27))"".5326 RMB"
sell_price="$(($RANDOM % 27))"".7723 RMB"
set +x

function set_contract_to_account()
{
	$cleos set contract $contract_user $WORKSPACE/contracts/exchange/build/exchange
    return 0
}

function add_wltpwd_into_chain()
{
    for idx in $(seq ${#test_accounts[@]})
    do
		res=$(curl -X POST $http_server_address/v1/exchange/push_action -d '{
		"wltpwd":"'"$WALLET_NAME"'#'"$wlt_pwd"'",
		"account":"mis.exchange",
		"action_name":"swlt",
		"action_args":"'"${test_accounts[$idx-1]}"','"${WALLET_NAME}#${wlt_pwd}"'"
		"permissions":["'"${test_accounts[$idx-1]}"'@active"]
		}')

		echo $res | grep -qiw "executed" || exit 1
    done
}

function query_table_by_sha256_index_test()
{
    # EOSIO中的sha256字符串存储的大小端顺序与平时产生的sha256字节序相反，所以需要翻转
    reversed_sha256_str=$(reverse_fixed_char $sha256_buyer)
    reversed_sha256_str=$(reverse_fixed_char $reversed_sha256_str 32)

    res=$($cleos get table $contract_user $table_scope ctoc $cleos_option  --index 2 --key-type sha256 -L $reversed_sha256_str -U $reversed_sha256_str)
    echo $res | grep -qw "$sha256_buyer" || exit 1

    return 0
}

function swlt_action_test()
{
    try_unlock_wallet $WALLET_NAME

    # add wallet
    wltpwd="wlt#pwd"
	$cleos push action $contract_user swlt '["'"${test_accounts[2]}"'","'"${wltpwd}"'"]' -p ${test_accounts[2]}
    res=$($cleos get table $contract_user $table_scope wltpwd $cleos_option)
    echo $res | grep -qw "${wltpwd}" || exit 1

    # remove wallet
	$cleos push action $contract_user swlt '["'"${test_accounts[2]}"'",""]' -p ${test_accounts[2]}
    res=$($cleos get table $contract_user $table_scope wltpwd $cleos_option)
    echo $res | grep -qw "${test_accounts[2]}" && exit 1
}

function clmto_action_test()
{
    try_unlock_wallet $WALLET_NAME

    sq=$(($RANDOM % 77 + 1))
    sp="$(($RANDOM % 99))"".3223 RMB"
	$cleos push action $contract_user lmto '["'"${test_accounts[1]}"'","token","'"$core_symbole_name"'",'"$sq"',"'"$sp"'",0]' -p ${test_accounts[1]}

    oid=$($cleos get table $contract_user $table_scope seller $cleos_option --index 3 --key-type i64 -L ${test_accounts[1]} -U ${test_accounts[1]} | grep oid | cut -d'"' -f 4 )
    $cleos push action $contract_user clmto '["'"${test_accounts[1]}"'","'"$oid"'"]' -p ${test_accounts[1]}

    res=$($cleos get table $contract_user $table_scope cancelorder $cleos_option)
    echo $res | grep -qw $oid || exit 1

    return 0
}

function lmto_action_test()
{
    try_unlock_wallet $WALLET_NAME

    cur_seconds="$(date +%s)""000000"
	$cleos push action $contract_user lmto '["'"${test_accounts[1]}"'","token","'"$core_symbole_name"'",'"$sell_quantity"',"'"$sell_price"'",0]' -p ${test_accounts[1]}
	$cleos push action $contract_user lmto '["'"${test_accounts[2]}"'","token","'"$core_symbole_name"'",'"$buy_quantity"',"'"$buy_price"'",1]' -p ${test_accounts[2]}

    res=$($cleos get table $contract_user $table_scope commoditype $cleos_option)
    echo $res | grep -qw "token" || exit 1

    res=$($cleos get table $contract_user $table_scope buyer $cleos_option --index 3 --key-type i64 -L ${test_accounts[2]} -U ${test_accounts[2]}  | tail -20 |grep -w consign_time | cut -d'"' -f 4 || true)
    res2=$($cleos get table $contract_user $table_scope buyerorder $cleos_option --index 3 --key-type i64 -L ${test_accounts[2]} -U ${test_accounts[2]}  | tail -20 |grep -w consign_time | cut -d'"' -f 4 || true)
    [[ "$cur_seconds" < "$res" || "$cur_seconds" < "$res2" ]] || exit 1

    sha256_buyer=$($cleos get table $contract_user $table_scope buyer $cleos_option --index 3 --key-type i64 -L ${test_accounts[2]} -U ${test_accounts[2]} | tail -20 | grep oid | cut -d'"' -f 4 )
    sha256_seller=$($cleos get table $contract_user $table_scope seller $cleos_option --index 3 --key-type i64 -L ${test_accounts[1]} -U ${test_accounts[1]} | tail -20 | grep oid |cut -d'"' -f 4)

    return 0
}

function mlmto_action_test()
{
    diff_quantity=6
    set +x
    minV=$(min $buy_quantity $sell_quantity)
    part_quantity=$(($minV - $diff_quantity))

    sha256_sym=$(checksum256 $core_symbole_name)
    # EOSIO中的sha256字符串存储的大小端顺序与平时产生的sha256字节序相反，所以需要翻转
    sha256_sym=$(reverse_fixed_char $sha256_sym)
    sha256_sym=$(reverse_fixed_char $sha256_sym 32)
    set -x

    matched_price="$($cleos get table $contract_user $table_scope matchedprice $cleos_option --index 2 --key-type sha256 -L $sha256_sym -U $sha256_sym | grep price | cut -d'"' -f 4)"

    precision=10000
    matchedV=${matched_price%% *}
    matchedV=$(awk -v x=$matchedV -v y=$precision 'BEGIN{printf "%ld", x*y}')
    buyV=${buy_price%% *}
    buyV=$(awk -v x=$buyV -v y=$precision 'BEGIN{printf "%ld", x*y}')
    sellV=${sell_price%% *}
    sellV=$(awk -v x=$sellV -v y=$precision 'BEGIN{printf "%ld", x*y}')

    if [[ $matchedV -gt $buyV ]]; then
        matched_price="$buy_price"
    elif [[ $matchedV -lt $sellV ]]; then
        matched_price="$sell_price"
    fi

    $cleos push action $contract_user mlmto '["'"$sha256_buyer"'","'"$sha256_seller"'","'"$core_symbole_name"'"]' ${cleos_multi_perms}

    res=$($cleos get table $contract_user $table_scope ctoc $cleos_option | tail -17)
    echo $res | grep -qw "\"buyer\": \"${sha256_buyer}\"" || exit 1

    res="$($cleos get table $contract_user $table_scope matchedprice $cleos_option --index 2 --key-type sha256 -L $sha256_sym -U $sha256_sym | grep price | cut -d'"' -f 4)"
    [[ "$matched_price" != "$res" ]] && exit 1

#	$cleos push action $contract_user mlmto '[["'"$sha256_seller"'","'"$sha256_buyer"'"],""]' ${cleos_multi_perms}
    # make sure limited orders have updated
    if [[ $buy_quantity -lt $sell_quantity ]]; then
        res=$($cleos get table $contract_user $table_scope buyerorder $cleos_option | tail -15)
    else
        res=$($cleos get table $contract_user $table_scope sellerorder $cleos_option | tail -15)
    fi
    # 全部成交
    echo $res | grep -qw "\"stat\": 3" || exit 1

    # failed testing
    res=$($cleos push action $contract_user mlmto '["'"$sha256_buyer"'","'"$sha256_seller"'","'"$core_symbole_name"'"]' -p ${test_accounts[1]} 2>&1) || true  # the true: make jenkins build continue 
    set +x
    echo $res | grep -qw "Missing required authority" || exit 1
    set -x

    return 0
}

function show_blockchain_info()
{
    $cleos get info
    return 0
}

set -x
set_contract_to_account
add_wltpwd_into_chain
#swlt_action_test
clmto_action_test
lmto_action_test
#mlmto_action_test
set +x
#query_table_by_sha256_index_test
# get the lastest block num for query transactions by trx_id
show_blockchain_info
# lock wallet after testing
try_lock_wallet $WALLET_NAME
