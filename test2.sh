#!/bin/sh

# time ./test2.sh 2>&1 | tee out2.log

rm files/file-*.txt

now=`date "+%s"`
echo "STARTED [$now]"

for i in $(seq 0 20); do
#for i in $(seq 0 2); do
    ./x.sh $i &
done

for i in $(seq 1 130); do
    sleep 1
    echo "=== $i ==="
done

wait

now=`date "+%s"`
echo "FINISH [$now]"

