#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source "$SCRIPT_DIR/variables.sh"

function draw() {
    echo -1 0 0 0 > "$PRU_SYSFS"
}

function setSegment() {
    s=$1
    s=$((s + SEGMENT_ONE))
    r=$2
    g=$3
    b=$4
    echo "$s $r $g $b" > "$PRU_SYSFS"
}

function setAndDrawSegment() {
    setSegment "$@"
    draw
}

function setAll() {
    r=$1
    g=$2
    b=$3
	for (( i=0; i<LED_COUNT; i++ )); do
            echo "$i $r $g $b" > "$PRU_SYSFS"
    done
}

function setAndDrawAll() {
    setAll "$@"
    draw
}

function clearDisplay() {
	echo -2 0 0 0 > "$PRU_SYSFS"
}

function clearDisplay_old() {
	for (( i=0; i<LED_COUNT; i++ )); do
		echo "$i 0 0 0" > "$PRU_SYSFS"
	done

    draw
}
