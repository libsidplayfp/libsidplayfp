#!/bin/sh

dir=$(dirname $0)

while read line; do
        if [[ $line =~ ^# ]]; then continue; fi
        echo "Running test $line"
        $dir/test $line
        retcode=$?
        if [[ $retcode -ne 0 ]]; then
                echo "Failed test $line"
                exit $retcode
        fi
done <  $dir/testlist

echo "Success!"
