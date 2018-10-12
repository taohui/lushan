#!/bin/sh

if [ $# -lt 4 ]
then
	echo "$0 <host> <port> <hmid> <status|reset|write_weights|write_state|remove_expired> [expire_time]"
	exit 0
fi

HOST=$1
PORT=$2
HMID=$3
OP=$4
EXP=$5

if [ "$OP"x = "status"x ]
then
	while true
	do
    		echo -ne "gets m${HMID}?op=status 0 0 0\r\n \r\n" | nc $HOST $PORT
    		sleep 1
	done
fi

if [ "$OP"x = "reset"x ]
then
    	echo -ne "gets m${HMID}?op=reset 0 0 0\r\n \r\n" | nc $HOST $PORT
fi

if [ "$OP"x = "write_weights"x ]
then
    	echo -ne "gets m${HMID}?op=write_weights 0 0 0\r\n \r\n" | nc $HOST $PORT
fi

if [ "$OP"x = "write_state"x ]
then
    	echo -ne "gets m${HMID}?op=write_state 0 0 0\r\n \r\n" | nc $HOST $PORT
fi

if [ "$OP"x = "remove_expired"x ]
then
    	echo -ne "gets m${HMID}?op=remove_expired&exp=${EXP} 0 0 0\r\n \r\n" | nc $HOST $PORT
fi
