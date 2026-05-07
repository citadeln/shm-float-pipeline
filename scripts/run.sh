#!/bin/bash

./build.sh
cd build
./producer ../data/input.bin & 
sleep 1
./consumer ../data/input.bin ../data/output.bin