#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

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

cd $SCRIPT_DIR/..

python3 -m pycodestyle \
    --config=misc/pycodestyle.config \
    libs5runcligen generate_client_code.py
check_ret $? "pycodestyle"

python3 -m pylint --max-line-length=120 \
    libs5runcligen generate_client_code.py
check_ret $? "pylint"