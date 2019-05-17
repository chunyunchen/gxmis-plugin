#!/bin/bash

set -e
echo "[$(date)] check" > /root/accounttest/crontest

PID=`ps aux | grep mistransfer | grep -v grep | awk '{print $2}'`

if [ -z "$PID" ]; then
	echo try to run mistransfer >> /root/accounttest/crontest
	nohup /root/accounttest/mistransfer.sh 2>&1 > /dev/null &
fi
