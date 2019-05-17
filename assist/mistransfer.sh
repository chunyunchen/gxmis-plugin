#!/bin/bash

set -o errexit

arry_account=("wangdada1234" "aabbyeaabbye" "abbeabbeabbe" "wangzanqun12" "meiyiqingaaa" "liuchajun" "dengjunying" "wangtianhui" "zhongying" "baiyifengbbb" "tuconglan" "shenying" "liubaozhi" "chentufeng" "shiqing" "weijian" "zengweihua" "wuyinqiao" "guoqiaozhi" "shenaiyin")

SYMBOL="MIS"

echo "[$(date)] test accounts transfter started"
# 转账算法
# 1. 随机选择转出和转入账号
# 2. 当最小账号余额小于水位值（200.0000 MIS），由余额最多的账号转出600.0000 MIS到最小余额的账号
while true
do
    min_balance="999999999999.9999 $SYMBOL"
    max_balance="0 $SYMBOL"
    min_balance_account="wangdada1234"
    max_balance_account="wangdada1234"
    for ac in ${arry_account[@]}
    do  
        blc=$(cleos get currency balance eosio.token $ac $SYMBOL)
        blc_len=$(echo "${#blc}")

        min_balance_len=$(echo "${#min_balance}")
        if [[ $blc_len -lt $min_balance_len || $blc_len -eq $min_balance_len && "$blc" < "$min_balance" ]];then
            min_balance="$blc"
            min_balance_account="$ac"
        fi  

		max_balance_len=$(echo "${#max_balance}")
        if [[ $blc_len -gt $max_balance_len || $blc_len -eq $max_balance_len && "$blc" > "$max_balance" ]];then
            max_balance="$blc"
            max_balance_account="$ac"
        fi  
    done

    watermark_balance="200.0000 $SYMBOL"
    watermark_balance_len=$(echo "${#watermark_balance}")
    min_balance_len=$(echo "${#min_balance}")
    if [[ $watermark_balance_len -gt $min_balance_len || $watermark_balance_len -eq $min_balance_len && "$watermark_balance" > "$min_balance" ]];then
        cleos transfer ${max_balance_account} ${min_balance_account} "600.0000 $SYMBOL"
        echo "[$(date)][max to min account] ${max_balance_account} -> ${min_balance_account} 600.0000 $SYMBOL"
    fi  

    amount=$(awk 'BEGIN{printf "%.4f",('$RANDOM'/'3000')}')
    idf=$(($RANDOM%20))
    idt=$((($idf+1)%20))
	time=$(($RANDOM%3 + 1))
	wait_ms=$(awk 'BEGIN{printf "%.3f",('$time'/'1000')}')
    
    if [[ "${arry_account[$idf]}" == "$min_balance_account" ]];then
        continue
    fi  

    cleos transfer ${arry_account[$idf]} ${arry_account[$idt]} "$amount $SYMBOL"

    echo "[$(date)] ${arry_account[$idf]} -> ${arry_account[$idt]} $amount $SYMBOL, next wait $wait_ms seconds"

    sleep $wait_ms
done
