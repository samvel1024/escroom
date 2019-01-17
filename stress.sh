#!/usr/bin/env bash

set -e

while [[ true ]]; do
    players=$(shuf -i 2-$1 -n 1)
    rooms=$(shuf -i 1-$2 -n 1)
    count=$(shuf -i 1-$3-n 1)
    echo "Testing with $players players $rooms rooms $count requests"
    ./test.sh "$players $rooms $count"
done;