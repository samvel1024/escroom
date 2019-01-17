#!/usr/bin/env bash

set -e
ctr=0

while [[ true ]]; do
    ctr=$((ctr + 1))

    players=$(shuf -i 2-$1 -n 1)
    rooms=$(shuf -i 1-$2 -n 1)
    count=$(shuf -i 1-$3 -n 1)
    echo "************************ TEST $ctr ***********************"
    echo "Testing with $players players $rooms rooms $count requests"
    ./test.sh "$players $rooms $count"
done;