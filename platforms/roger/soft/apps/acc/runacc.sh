#!/bin/sh

for test in out.jpeg.*; do
    echo $test && perf stat -e cycles -e instructions ./$test
done
