#!/bin/bash

chmod +x build.sh
./build.sh
cd build
./producer ../data/input.bin & ./consumer ../data/input.bin ../data/output.bin
