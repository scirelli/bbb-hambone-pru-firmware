#!/usr/bin/env bash
echo 'none' > '/sys/class/leds/beaglebone:green:usr3/trigger'
echo 'pruin' > '/sys/devices/platform/ocp/ocp:P9_25_pinmux/state'
echo 'pruin' > '/sys/devices/platform/ocp/ocp:P9_27_pinmux/state'
echo 'pruout' > '/sys/devices/platform/ocp/ocp:P9_28_pinmux/state'
echo 'pruout' > '/sys/devices/platform/ocp/ocp:P9_29_pinmux/state'
echo 'pruout' > '/sys/devices/platform/ocp/ocp:P9_30_pinmux/state'
echo 'pruout' > '/sys/devices/platform/ocp/ocp:P9_31_pinmux/state'
