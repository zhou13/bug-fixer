#!/bin/bash
for (( i=200000; i<=3000000; i=i+200000 ))
do
	echo "n=20000 q=$i"
	./data-gen 20000 $i 0.5 > data.in
	time ./algo -i data.in -con -deg -act
done
