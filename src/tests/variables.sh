#!/usr/bin/env bash

export PRU_SYSFS=/dev/rpmsg_pru30
export PRU_RPMSG_TX=/dev/rpmsg_pru30 # From script PoV
export PRU_RPMSG_RX=/dev/rpmsg_pru31 # From script PoV
export COMMON_MAKE_DIR="/var/lib/cloud9/common"
export COMMON_MAKE_FILE="$COMMON_MAKE_DIR/Makefile"

export LED_COUNT=42
# export SEGMENT_START_INDEX=120
export SEGMENT_START_INDEX=$((LED_COUNT + LED_COUNT))
export SEGMENT_ONE="$SEGMENT_START_INDEX"

export CODE_MOTOR=-3
export MOTOR_STOP=0
export MOTOR_BRAKE=1
export MOTOR_CW=2
export MOTOR_CCW=3
