#!/bin/bash

if [ $# != 1 ]; then
    echo "Usage bash wait-pending-jobs.sh log_path"
    exit
fi

log=$1

a=-1
b=`grep JOB $log | wc -l`

while [ $a != $b ]; do
    a=$b
    b=`grep JOB $log | wc -l`
    sleep 15
done