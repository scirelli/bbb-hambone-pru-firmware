#!/usr/bin/env bash

export PRU_SYSFS=/dev/rpmsg_pru30
export LED_COUNT=42
# export SEGMENT_START_INDEX=120
export SEGMENT_START_INDEX=$((LED_COUNT + LED_COUNT))
export SEGMENT_ONE="$SEGMENT_START_INDEX"
