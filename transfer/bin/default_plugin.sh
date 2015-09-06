#!/bin/sh

if [ $# -lt 2 ]
then
	echo "$0 <hadoop file> <hdict file>"
fi

if [ ! -d $1 ]
then
	echo "$1 is not a dir"
	exit 1
fi

if [ -d $2 ]
then
	echo "$2 already exists"
	exit 1
fi

mv $1 $2

exit $?
