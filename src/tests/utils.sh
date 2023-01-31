#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source "$SCRIPT_DIR/variables.sh"

function stopPRU() {
    pruNo=${1:-0}
    cd "$COMMON_MAKE_DIR" || return 1
    make TARGET="gpio.pru$pruNo" stop
    cd - || return 1
}

function startPRU() {
    pruNo=${1:-0}
    cd "$COMMON_MAKE_DIR" || return 1
    make TARGET="gpio.pru$pruNo" start
    cd - || return 1
}

function draw() {
    echo -1 0 0 0 > "$PRU_RPMSG_TX"
}

function setSegment() {
    s=$1
    s=$((s + SEGMENT_ONE))
    r=$2
    g=$3
    b=$4
    echo "$s $r $g $b" > "$PRU_RPMSG_TX"
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
            echo "$i $r $g $b" > "$PRU_RPMSG_TX"
    done
}

function setAndDrawAll() {
    setAll "$@"
    draw
}

function clearDisplay() {
	echo -2 0 0 0 > "$PRU_RPMSG_TX"
}

function clearDisplay_old() {
	for (( i=0; i<LED_COUNT; i++ )); do
		echo "$i 0 0 0" > "$PRU_RPMSG_TX"
	done

    draw
}

function motorCw() {
    echo "$CODE_MOTOR $MOTOR_CW" > "$PRU_RPMSG_TX"
}

function motorCCw() {
    echo "$CODE_MOTOR $MOTOR_CCW" > "$PRU_RPMSG_TX"
}

function motorStop() {
    echo "$CODE_MOTOR $MOTOR_STOP" > "$PRU_RPMSG_TX"
}

function motorBrake() {
    echo "$CODE_MOTOR $MOTOR_BRAKE" > "$PRU_RPMSG_TX"
}
