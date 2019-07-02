#!/bin/bash

eosio_system_accounts=(
eosio.bpay
eosio.msig
eosio.names
eosio.ram
eosio.ramfee
eosio.saving
eosio.stake
eosio.token
eosio.vpay
eosio.rex
)

jenkins_dir=$(dirname $script_dir)
assist_dir=$(dirname $jenkins_dir)

creator=eosio
mis_system_account="mis.exchange"
pri_key=5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
total_tokens="450000000000000.0000"
mis_account_token_count="400000000000000.0000"
ram_token_count="100000000.0000"
delegated_token_count="10000.0000"
balance_token_count="100000.0000"
tokens=($core_symbole_name RMB CNY USDT USD)
all_accounts_num=${#eosio_system_accounts[@]}

function active_PREACTIVATE_FEATURE_for_nodeos-v1.8.0_or_later()
{
    set -x
    PREACTIVATE_FEATURE_NAME="PREACTIVATE_FEATURE"

    nodeosv=$($nodeos --version || true)
    [[ "$nodeosv" > "v1.8" || "$nodeosv" > "1.8" ]] && activated_protocol_features=$(curl -X POST $http_server_address/v1/chain/get_activated_protocol_features) || true

    echo $activated_protocol_features | grep -qw $PREACTIVATE_FEATURE_NAME

    if [[ 0 -lt $? ]]; then
        supported_protocol_features=$(curl -X POST $http_server_address/v1/producer/get_supported_protocol_features)

        truncated_tail=${supported_protocol_features%%${PREACTIVATE_FEATURE_NAME}*}
        truncated_head=${truncated_tail##*\"feature_digest\":\"}
        preactivate_feature_digest=${truncated_head%%\",\"*}

        curl -X POST $http_server_address/v1/producer/schedule_protocol_feature_activations -d '{"protocol_features_to_activate": ['"$preactivate_feature_digest"']}'
    fi
    set +x

    return 0
}

function set_system_contracts()
{
	$cleos set contract eosio.token $assist_dir/build/contracts/eosio.token/
    $cleos set contract eosio $assist_dir/build/contracts/eosio.system/
    for tk in ${tokens[@]}
    do
	    may_create_token $tk
    done
	return 0
}

function may_create_token()
{
	symbol=$($cleos get currency stats eosio.token $1)
	echo $symbol | grep -q $1||
	(
		$cleos push action eosio.token create '["eosio", "'"$total_tokens $1"'"]' -p eosio.token
		$cleos push action eosio.token issue '["eosio", "'"$total_tokens $1"'", "memo"]' -p eosio

        if [[ "$core_symbole_name" == "$1" ]]; then
            $cleos push action eosio init '[0,"4,'"$core_symbole_name"'"]' -p eosio@active
        fi
	)
	return 0
}

function may_transfer_tokens_to_test_accounts()
{
	for account in ${test_accounts[@]}
	do
        for tk in ${tokens[@]}
        do
		    balance=$($cleos get currency balance eosio.token $account $tk)
		    echo $balance | grep -q $tk ||
		    (
			    $cleos push action eosio.token transfer '["eosio", "'"$account"'","'"$balance_token_count $tk"'","memo"]' -p eosio
                if [[ "$mis_system_account" == "$account" ]]; then
                    $cleos push action eosio.token transfer '["eosio", "'"$account"'","'"$mis_account_token_count $tk"'","memo"]' -p eosio
                fi

                if [[ "$core_symbole_name" == "$tk" ]]; then
                    $cleos system buyram eosio $account "$ram_token_count $core_symbole_name"
			        $cleos system delegatebw eosio $account "$delegated_token_count $core_symbole_name" "$delegated_token_count $core_symbole_name" -p eosio
                fi
		    )
        done
	done
	return 0
}

function try_unlock_wallet()
{
	if [[ -f $wallet_pwd_dir/$1.$wallet_pwd_file_ext ]]; then
		wlt_pwd=$(cat $wallet_pwd_dir/$1.$wallet_pwd_file_ext)
		$cleos wallet unlock -n $1 --password $wlt_pwd || (echo "$1 has Unlocked!")
	fi
	return 0
}

function try_lock_wallet()
{
	$cleos wallet lock -n $1 || echo "$1 wallet may not existing or opened!"
	return 0
}

function may_create_wallet()
{
    $cleos wallet open -n $1 ||
	(
		$cleos wallet create -n $1 --file $wallet_pwd_dir/$1.$wallet_pwd_file_ext
        # 系统账号eosio的私钥
		$cleos wallet import -n $1 --private-key $pri_key
	)
	return 0
}

function create_wallet()
{
	may_create_wallet $WALLET_NAME
	try_unlock_wallet $WALLET_NAME
	return 0
}

function may_create_account()
{
    all_accounts_num=$((${all_accounts_num} - 1))
    prefix_len=${#pubkey_prefix}
    pubkey_content="${all_pubkeys_in_wallet[$all_accounts_num]:$prefix_len}"
    pub_key="$pubkey_prefix""$pubkey_content"

    $cleos get account $1 ||
	(
		$cleos create account $creator $1 $pub_key || 
        $cleos system newaccount eosio $1 $pub_key --stake-net "10.0000 $core_symbole_name" --stake-cpu "10.0000 $core_symbole_name" --buy-ram "23.0000 $core_symbole_name"
	)
	return 0
}

function create_system_accounts()
{
	for account in ${eosio_system_accounts[@]}
	do
		may_create_account $account
	done
	return 0
}

function may_create_keys_in_wallet_and_get()
{
    pubkey_count=$(pubkey_count_in_wallet)
    keys_to_be_created_num=$((${all_accounts_num} - ${pubkey_count}))

    create_keys $keys_to_be_created_num

    OLD_IFS=$IFS
    IFS="\"" 

    raw_output=($($cleos wallet keys))
    for key in ${raw_output[@]}
    do
        echo "$key" | grep -q $pubkey_prefix && all_pubkeys_in_wallet[${#all_pubkeys_in_wallet[@]}]=$key
    done

    IFS=$OLD_IFS

    return 0
}

function create_keys()
{
    n=${1:-0}
    for((i=1; i<=$n; i++))
    do
        $cleos wallet create_key -n $WALLET_NAME 
    done

	return 0
}

function pubkey_count_in_wallet()
{
    count=$($cleos wallet keys |grep -c $pubkey_prefix)
    echo $count

	return 0
}

function create_test_accounts()
{

	for account in ${test_accounts[@]}
	do
		may_create_account $account
	done

    return 0
}

function set_test_accounts()
{
	OLD_IFS=$IFS
	IFS=","

    test_accounts=($ACCOUNTS)
    all_accounts_num=$((${all_accounts_num} + ${#test_accounts[@]}))

	IFS=$OLD_IFS

    return 0
}

function main()
{
    active_PREACTIVATE_FEATURE_for_nodeos-v1.8.0_or_later
    create_wallet
    set_test_accounts
    may_create_keys_in_wallet_and_get
    create_system_accounts
    create_test_accounts
    set_system_contracts
    may_transfer_tokens_to_test_accounts

    return 0
}

set +x
main
set -x
