#!/bin/bash

check_ret() {
    if [ $1 -ne 0 ]; then
        echo ""
        echo "!!! FAIL: $2"
        echo "********************************************************************************"
        echo ""
        exit $1
    else
        echo ""
        echo "*** SUCCESS: $2"
        echo "********************************************************************************"
        echo ""
    fi
}

cd "$(dirname "$0")"

rm -rf bin

javac -d bin $(find ./src -name "*.java")
check_ret $? "Compile"

jar -cmf manifest.mf s5run-client.jar  -C bin .
check_ret $? "Build s5run-client.jar"

java -jar s5run-client.jar
check_ret $? "Run s5run-client.jar"
