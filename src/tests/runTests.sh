#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source "$SCRIPT_DIR/test.sh"

for i in "${!TESTS[@]}"; do
    echo "Test $i"
    ${TESTS[$i]}
    nextTest "$DELAY_BETWEEN_TESTS"
done

cleanUp
