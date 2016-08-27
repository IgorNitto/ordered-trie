#!/bin/bash

echo "Building tests..."

cmake -H. -Bbuild
make -C build

echo
echo "Running tests..."

./build/bin/test_ordered_trie
