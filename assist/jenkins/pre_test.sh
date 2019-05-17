#!/bin/bash

set -e
set -x

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
)

jenkins_dir=$(dirname $script_dir)
assist_dir=$(dirname $jenkins_dir)

creator=eosio
pub_key="$pubkey_prefix""6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
pri_key=5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
total_tokens="1000000000.0000"
each_account_tokens="100000.0000"

function set_system_contracts()
{
	$cleos set contract eosio.token $assist_dir/build/contracts/eosio.token/
	may_create_token
	return 0
}

function may_create_token()
{
	symbol=$($cleos get currency stats eosio.token $core_symbole_name)
	echo $symbol | grep -q $core_symbole_name ||
	(
		$cleos push action eosio.token create '["eosio", "'"$total_tokens $core_symbole_name"'"]' -p eosio.token
		$cleos push action eosio.token issue '["eosio", "'"$total_tokens $core_symbole_name"'", "memo"]' -p eosio
	)
	return 0
}

function may_transfer_tokens_to_test_accounts()
{
	for account in ${test_accounts[@]}
	do
		balance=$($cleos get currency balance eosio.token $account)
		echo $balance | grep -q $core_symbole_name ||
		(
			$cleos push action eosio.token transfer '["eosio", "'"$account"'","'"$each_account_tokens $core_symbole_name"'","memo"]' -p eosio
		)
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

function may_create_wallet()
{
    $cleos wallet open -n $1 ||
	(
		$cleos wallet create -n $1 --file $wallet_pwd_dir/$1.$wallet_pwd_file_ext
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
    $cleos get account $1 ||
	(
		$cleos create account $creator $1 $pub_key $pub_key
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

function create_test_accounts()
{
	OLD_IFS=$IFS
	IFS=","
	taccs=("$ACCOUNTS")
	for account in ${taccs[@]}
	do
		test_accounts[${#test_accounts[@]}]=$account
		may_create_account $account
	done
	IFS=$OLD_IFS
	return 0
}

create_wallet
create_system_accounts
create_test_accounts
set_system_contracts
may_transfer_tokens_to_test_accounts
