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

# cleanup before success build
rm -rf bin

RELEASE_FLAG=""
if [ -n "$JAVA_RELEASE" ]; then
    RELEASE_FLAG="--release $JAVA_RELEASE"
fi

javac $RELEASE_FLAG -d bin $(find ./src -name "*.java")
check_ret $? "Compile"

CLIENT_VERSION=$(grep -o 'VERSION = "[^"]*"' $(find ./src -name "ClientVersion.java") | grep -o '[0-9][^"]*')
check_ret $? "Detect client version"

jar -cmf manifest.mf s5run-client-$CLIENT_VERSION.jar -C bin .
check_ret $? "Build s5run-client-$CLIENT_VERSION.jar"

# cleanup bin files after success build
rm -rf bin
check_ret $? "rm -rf bin"

# java -jar s5run-client-$CLIENT_VERSION.jar "$@"
# check_ret $? "Run s5run-client-$CLIENT_VERSION.jar"
