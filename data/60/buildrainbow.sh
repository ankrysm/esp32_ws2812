#!/bin/bash
N=60
D=$(( 360 / $N))
S=100
V=100
echo "i,$N;"
echo "c;"

for (( i=0; i < $N; i++ ))
do 
	H=$(($i*$D))
	echo "h,$i,$H,$S,$V;"
done
