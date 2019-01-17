#!/usr/bin/env bash

generator=$1

echo "Compiling"

set -e
cd cmake-build-debug
make;
cd ..;


rm -rf "input"
mkdir input;
cp cmake-build-debug/player cmake-build-debug/manager input
echo "Generating tests"
echo "${generator} $RANDOM $(pwd)/input" | ./cmake-build-debug/testgen
cd input;
echo "Running"
cat manager.in | ./manager;

if [ $(grep -rnw "Invalid" *.out | wc -l) -ne 0 ]; then
  echo "Found invalid occurence";
  grep -rnw "Invalid" *.out;
fi




