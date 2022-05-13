#!/bin/bash
N=60
R=255
G=$R
B=$R
echo "i,$N;"
echo "c;"

for (( i=0; i < $N; i++ ))
do 
	echo "p,$i,$R,$G,$B;"
done
