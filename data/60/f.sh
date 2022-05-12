#!/bin/bash
if [[ $# -lt 1 ]]
then
	echo "usage $0 <file>"
	exit 0
fi
F=$1 
shift
if [[ ! -f $F ]]
then
	echo "$F existiert nicht"
	exit 0
fi
curl  192.168.2.228/api/v1/file -d @$F
