#!/usr/bin/env sh

make -p project_v1/bin
make -p project_v2/bin
make -p project_v3/bin

rm -rf scan_results/*
../tracking-platform clean --remove

cd project_v1
make scan

cd ../project_v2
make scan

cd ../project_v3
make scan

cd ..
../tracking-platform init
../tracking-platform add --few-runs ./scan_results
../tracking-platform log  