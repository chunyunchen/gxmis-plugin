#!/bin/bash

# Date: 2019/05/30
# Author: chen chunyun
# Desc: 包含工具类函数

# 从参数列表中取得最值
function min()
{
	min=$1
	for i in $@
	do
	    if [[ $min -gt $i ]]; then
	    	min=$i
	    fi
	done
	echo $min

	return 0
}

function max()
{
	max=$1
	for i in $@
	do
	    if [[ $max -lt $i ]]; then
	    	max=$i
	    fi
	done
	echo $max
	
	return 0
}

# 以指定字符长度为单位翻转字符串，如: abcdef,指定2个字符为长度单位，翻转后便是: efcdab
function reverse_fixed_char()
{
    fixed_len=2
    source_str=$1

    if [[ $# -ge 2 ]]; then
        fixed_len=$2
    fi

    str_len=${#source_str}
    half_str_len=$(($str_len / $fixed_len))
    reversed_str=""
    for ((i=1; i<=$half_str_len; i++))
    do
        pos=$(($str_len - $fixed_len * $i))
        reversed_str=${reversed_str}${source_str:$pos:$fixed_len}
    done

    echo $reversed_str

	return 0
}

function get_transaction_by_id()
{
    if [[ $# -lt 2 ]]; then
        echo "Usage: get_transaction_by_id <trx_id> <block_num> [num_steps]"
        exit 1
    fi

    trx_id=$1
    step=17
    if [[ $# -ge 3 ]]; then
        step=$3
    fi

    start_num=$2
    end_num=$(($2 + $step))

    confirm_words="actions"
    echo "Will find trx in block from $start_num to $end_num"
    for((bn = ${start_num}; bn <= ${end_num}; bn++))
    do
        echo "in $bn?"
        res=$(cleos get transaction $trx_id -b $bn 2>&1)
        echo $res | grep -qw $confirm_words  && echo $res && return 0
    done
    echo "Ooops! Not found the given trx.(You could modify the block num or steps, then try it again.)"

	return 0
}

function checksum256()
{
    seed="$*"
    if [[ "X" == "X$seed" ]]; then
        ARCH=$(uname -s)
        if [[ "Linux" == "$ARCH" ]]; then
            seed=$(date +%s.%N)
        else
            seed=$(gdate +%s.%N)
        fi
    fi
    echo -n "$seed" | shasum -a 256 | awk '{print $1}'

	return 0
}

