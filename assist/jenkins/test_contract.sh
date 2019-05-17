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

contract_user=${test_accounts[0]}
function set_contract_to_account()
{
	$cleos set contract $contract_user $WORKSPACE/contracts/exchange/build/exchange
}

function lmto_action_test()
{
	$cleos push action $contract_user lmto '["'"${test_accounts[1]}"'","token","'"$core_symbole_name"'",23.8903,"32.0099 RMB",9999999,0]' -p ${test_accounts[1]}
}

function mlmto_action_test()
{
	$cleos push action $contract_user mlmto '["'"${test_accounts[1]}"'","'"${test_accounts[2]}"'","token","'"$core_symbole_name"'",23.8903,"32.0099 RMB",9999999]' -p ${test_accounts[1]} -p ${test_accounts[2]}
}

set_contract_to_account
lmto_action_test
mlmto_action_test
