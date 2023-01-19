#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

color=$1
source "$SCRIPT_DIR/variables.sh"
source "$SCRIPT_DIR/utils.sh"

clearDisplay

setAndDrawAll $color $color $color
