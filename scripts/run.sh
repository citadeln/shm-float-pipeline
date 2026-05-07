#!/bin/bash

cd build
./producer ../data/sample.bin & 
sleep 1
./consumer ../data/sample.bin ../data/output.bin
