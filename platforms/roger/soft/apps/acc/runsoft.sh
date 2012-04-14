#!/bin/sh

for test in out.code0.*; do
    echo $test && perf stat -e cycles -e instructions ./$test
done
