#!/bin/bash

set -x
set -e

curl -X POST http://127.0.0.1:8888/v1/exchange/push_action -d '{
	"wallet":{
		"wallet_name":"ccyw",
		"wallet_pwd":"PW5JrF9vCWjAGFg8LZWtBPxYMbsFWPWv5jbBoPTyattPhSwSuwWp6"
	},
	"account":"ccy",
	"action_name":"approvedata",
	"action_args":"ccy,eosio.token,hexdf,2019/09/23 09%l03%l32,true"
	"permissions":["ccy@active"]
}'
