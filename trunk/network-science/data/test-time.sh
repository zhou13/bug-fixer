#!/bin/bash
for (( i=50000; i<=2000000; i=i+10000000 ))
do
	echo "n=$i q=1000000"
	./data-gen $i 1000000 0.5 > data.in
	time ./algo -i data.in -con -deg -act
done
