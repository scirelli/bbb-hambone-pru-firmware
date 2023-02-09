////////////////////////////////////////
//	hambone-display-motor.pru0.c
//	Uses rpmsg to control the NeoPixels via /dev/rpmsg_pru30 on the ARM
//	Usage:	echo index R G B > /dev/rpmsg_pru30 to set the color at the given index
//			echo -1 0 0 0    > /dev/rpmsg_pru30 to update the string
//			echo 0 0xf0 0 0  > /dev/rpmsg_pru30 Turns pixel 0 to Red
//			neopixelRainbow.py to display moving rainbow pattern
//	Wiring:	The NeoPixel Data In goes to P9_29, the plus lead to P9_3 or P9_4
//			and the ground to P9_1 or P9_2.  If you have more then 40 some
//			NeoPixels you will need and external supply.
//	Setup:	config_pin P9_29 pruout
//	See:
//	PRU:	pru0
//  Refernce:
//  * http://cdn.sparkfun.com/datasheets/Components/LED/WS2812.pdf or https://cdn-shop.adafruit.com/datasheets/WS2812.pdf
//  * https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
//  * https://software-dl.ti.com/processor-sdk-linux/esd/docs/latest/linux/Foundational_Components_PRU-ICSS_PRU_ICSSG.html
//  * https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/tree/include?id=aa9606013059eb8728bcc1165c5032f0589469e0

/* ******************* NOTES *************************************************************************************
WS2812 and WS2812B have slightly different times. Other versions do as well.

Data Transfer time (TH+TL=1.25μs +-600ns) (WS2812)
┏━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━┓
┃  T0H               │ 0 code, high voltage time         │ 0.35μs             │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T1H               │ 1 code, high voltage time         │ 0.7μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T0L               │ 0 code, low voltage time          │ 0.8μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T1L               │ 1 code, low voltage time          │ 0.6μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  RES               │ low voltage time                  │ Above 50μs         │                    ┃
┗━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━┛

Data Transfer time (TH+TL=1.25μs +-600ns) (WS2812B)
┏━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━┓
┃  T0H               │ 0 code, high voltage time         │ 0.4μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T1H               │ 1 code, high voltage time         │ 0.8μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T0L               │ 0 code, low voltage time          │ 0.85μs             │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T1L               │ 1 code, low voltage time          │ 0.45μs             │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  RES               │ low voltage time                  │ Above 50μs         │                    ┃
┗━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━┛

Data Transfer time (TH+TL=1.25μs +-600ns) (SK6812 RGB)
┏━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━┓
┃  T0H               │ 0 code, high voltage time         │ 0.3μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T1H               │ 1 code, high voltage time         │ 0.6μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T0L               │ 0 code, low voltage time          │ 0.9μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T1L               │ 1 code, low voltage time          │ 0.6μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  RES               │ low voltage time                  │ Above 80μs         │                    ┃
┗━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━┛

Data Transfer time (TH+TL=1.25μs +-600ns) (SK6812 RGBW)
┏━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━┓
┃  T0H               │ 0 code, high voltage time         │ 0.3μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T1H               │ 1 code, high voltage time         │ 0.6μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T0L               │ 0 code, low voltage time          │ 0.9μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T1L               │ 1 code, low voltage time          │ 0.6μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  RES               │ low voltage time                  │ Above 80μs         │                    ┃
┗━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━┛


Sequence Chart
         ┏━━━━━━━━━┓     T0L    ┃
         ┃<------->┃<---------->┃
0 code   ┃   T0H   ┗━━━━━━━━━━━━┛


         ┏━━━━━━━━━━━━┓   T1L   ┃
         ┃<---------->┃<------->┃
1 code   ┃   T1H      ┗━━━━━━━━━┛


         ┃       Treset         ┃
RET code ┃<-------------------->┃
         ┗━━━━━━━━━━━━━━━━━━━━━━┛

Cascade method:

D1  ┏━━━━━━━━━━┓ D2   ┏━━━━━━━━━━┓ D3   ┏━━━━━━━━━━┓ D4
--->┃DIN     D0┃----->┃DIN     D0┃----->┃DIN     D0┃----->
    ┃   PIX1   ┃      ┃   PIX2   ┃      ┃   PIX3   ┃
    ┗━━━━━━━━━━┛      ┗━━━━━━━━━━┛      ┗━━━━━━━━━━┛

Data Transmission method:
                                            reset code
                                             >= 50μs
    │                                 ---->│          │<----                                       │
    │<-- Data refresh cycle 1 ------------>│          │<----------- Data refresh cycle 2 --------->│
    ├────────────┬────────────┬────────────┤          ├────────────┬────────────┬────────────┐     │
    │ 1st 24bits │ 2nd 24bits │ 3rd 24bits │          │ 1st 24bits │ 2nd 24bits │ 3rd 24bits │     │
D1 ─┼────────────┴────────────┴────────────┼──────────┼────────────┴────────────┴────────────┴─────┼──
    │                                      │          │                                            │
    │            ┌────────────┬────────────┤          │            ┌────────────┬────────────┐     │
    │            │ 2nd 24bits │ 3rd 24bits │          │            │ 2nd 24bits │ 3rd 24bits │     │
D2 ─┼────────────┴────────────┴────────────┼──────────┼────────────┴────────────┴────────────┴─────┼──
    │                                      │          │                                            │
    │                         ┌────────────┤          │                         ┌────────────┐     │
    │                         │ 3rd 24bits │          │                         │ 3rd 24bits │     │
D3 ─┼─────────────────────────┴────────────┼──────────┼─────────────────────────┴────────────┴─────┼──
    │                                      │          │                                            │
    │                                      │          │                                            │
    │                                      │          │                                            │
D4 ─┼──────────────────────────────────────┼──────────┼────────────────────────────────────────────┼──

Note: The data of D1 is send by the MCU. D2, D3, and D4 through pixel internal reshaping amplification to
transmit.

Composition of 24bit data:
┏━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┓
┃ G7 │ G6 │ G5 │ G4 │ G3 │ G2 │ G1 │ G0 │ R7 │ R6 │ R5 │ R4 │ R3 │ R2 │ R1 │ R0 │ B7 │ B6 │ B5 │ B4 │ B3 │ B2 │ B1 │ B0 ┃
┗━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┛
Note: Follow the order of GRB to sent data and the high bit sent at first. Big-endian



Motor driver (TB6612FNG) Pin Table
 ┏━━━━━━━━━━┯━━━━━━━━━━┯━━━━━━━━━━┯━━━━━━━━━━┯━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━┓
 ┃ In1      │ In2      │ PWM      │ Out1     │ Out2     │ Mode             ┃
 ┠──────────┼──────────┼──────────┼──────────┼──────────┼──────────────────┨
 ┃    H     │    H     │    H/L   │   L      │   L      │ Short Brake      ┃
 ┠──────────┼──────────┼──────────┼──────────┼──────────┼──────────────────┨
 ┃    L     │    H     │    H     │   L      │   H      │ CCW              ┃
 ┠──────────┼──────────┼──────────┼──────────┼──────────┼──────────────────┨
 ┃    L     │    H     │    L     │   L      │   L      │ Short Brake      ┃
 ┠──────────┼──────────┼──────────┼──────────┼──────────┼──────────────────┨
 ┃    H     │    L     │    H     │   H      │   L      │ CW               ┃
 ┠──────────┼──────────┼──────────┼──────────┼──────────┼──────────────────┨
 ┃    H     │    L     │    L     │   L      │   L      │ Short Brake      ┃
 ┠──────────┼──────────┼──────────┼──────────┼──────────┼──────────────────┨
 ┃    L     │    L     │    H     │  OFF     │   OFF    │ Stop             ┃
 ┗━━━━━━━━━━┷━━━━━━━━━━┷━━━━━━━━━━┷━━━━━━━━━━┷━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━┛
 Don't forget STBY must be high for the motors to drive.
 https://learn.sparkfun.com/tutorials/tb6612fng-hookup-guide

These are which GPIO to connect the motor drive and limit switches.  The GPIO is defined in prugpio.h
#define P9_25   (1<<7)  // Limit One
#define P9_27   (1<<5)  // Limit Two
#define P9_28   (1<<3)  // Motor A11
#define P9_29   (1<<1)  // NeoPixel
#define P9_30   (1<<2)  // Motor A12
#define P9_31   (1<<0)  // Motor PWM

If using shared memory
    Give Python access to the shared memory. This can cause security vulnerabilities.
        $ sudo setcap "cap_dac_override+ep cap_sys_rawio+ep" /media/sdcard/home/cck/.pyenv/versions/3.7.13/bin/python3.7
        $ getcap /media/sdcard/home/cck/.pyenv/versions/3.7.13/bin/python3.7
            /media/sdcard/home/cck/.pyenv/versions/3.7.13/bin/python3.7 = cap_dac_override,cap_sys_rawio+ep

Starting and stopping the PRUs
    make TARGET=gpio.pru0 stop
    make TARGET=gpio.pru0 start

https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/tree/include/pru_rpmsg.h?id=aa9606013059eb8728bcc1165c5032f0589469e0
Summary		:	pru_rpmsg_receive receives a message, if available, from
                the ARM host.

Parameters	:	transport: a pointer to the transport layer from which the
                            message should be received
                src: a pointer that is populated with the source address
                        where the message originated
                dst: a pointer that is populated with the destination
                        address where the message was sent (can help determine
                        for which channel the message is intended on the PRU)
                data: a pointer that is populated with a local data buffer
                        containing the message payload
                len: a pointer that is populated with the length of the
                        message payload

Description	:	pru_rpmsg_receive uses the pru_virtqueue interface to get
                an available buffer, copy the buffer into local memory,
                add the buffer as a used buffer to the vring, and then kick
                the remote processor if necessary. The src, dst, data, and
                len pointers are populated with the information about the
                message and local buffer data if the reception is
                successful.

Return Value	:	Returns PRU_RPMSG_NO_BUF_AVAILABLE if there is currently no
                buffer available for receive. Returns PRU_RPMSG_INVALID_HEAD
                if the head index returned for the available buffer is
                invalid. Returns PRU_RPMSG_SUCCESS if the message is
                successfully received.
int16_t pru_rpmsg_receive (
    struct pru_rpmsg_transport 	*transport,
    uint16_t 					*src,
    uint16_t 					*dst,
    void 						*data,
    uint16_t 					*len
);

Summary		:	pru_rpmsg_send sends a message to the ARM host using the
                virtqueues in the pru_rpmsg_transport structure. The
                source and destination address of the message are passed
                in as parameters to the function. The data to be sent and
                its length are passed in the data and len parameters.

Parameters	:	transport: a pointer to the transport layer from which the
                            message should be sent
                src: the source address where this message will originate
                dst: the destination address where the message will be sent
                data: a pointer to a local data buffer containing the
                        message payload
                len: the length of the message payload

Description	:	pru_rpmsg_send sends a message to the src parameter and
                from the dst parameter. The transport structure defines the
                underlying transport mechanism that will be used. The
                data parameter is a pointer to a local buffer that should
                be sent to the destination address and the len parameter is
                the length of that buffer.

Return Value	:	Returns PRU_RPMSG_NO_BUF_AVAILABLE if there is currently no
                buffer available for send. Returns PRU_RPMSG_BUF_TOO_SMALL
                if the buffer from the vring is too small to hold the
                message payload being sent. Returns PRU_RPMSG_INVALID_HEAD
                if the head index returned for the send buffer is invalid.
                Returns PRU_RPMSG_SUCCESS if the message is successfully
                sent.
int16_t pru_rpmsg_send (
    struct pru_rpmsg_transport 	*transport,
    uint32_t 					src,
    uint32_t 					dst,
    void 						*data,
    uint16_t 					len
);
*********************************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>			// atoi
#include <string.h>
#include <stdbool.h>
#include <pru_cfg.h>
#include <pru_intc.h>
#include <rsc_types.h>
#include <pru_rpmsg.h>
#include "resource_table_0.h"
#include "prugpio.h"

/* Host-0 Interrupt sets bit 30 in register R31 */
#define HOST_INT			((uint32_t) 1 << 30)

/* The PRU-ICSS system events used for RPMsg are defined in the Linux device tree
 * PRU0 uses system event 16 (To ARM) and 17 (From ARM)
 * PRU1 uses system event 18 (To ARM) and 19 (From ARM)
 * Be sure to change the values in resource_table_0.h too.
 */
#define TO_ARM_HOST			16
#define FROM_ARM_HOST		17

/*
* Using the name 'rpmsg-pru' will probe the rpmsg_pru driver found
* at linux-x.y.z/drivers/rpmsg/rpmsg_pru.c
*/
#define CHAN_RX_NAME	"rpmsg-pru"
#define CHAN_RX_DESC	"Channel 30"
#define CHAN_RX_PORT	30
#define RX_DST_ADDR     ((uint16_t)1024)
#define RX_SRC_ADDR     ((uint16_t)(CHAN_RX_PORT))

#define CHAN_TX_NAME	"rpmsg-pru"
#define CHAN_TX_DESC	"Channel 31"
#define CHAN_TX_PORT	31
#define TX_DST_ADDR     ((uint16_t)1025)
#define TX_SRC_ADDR     ((uint16_t)(CHAN_TX_PORT))

/*
 * Used to make sure the Linux drivers are ready for RPMsg communication
 * Found at linux-x.y.z/include/uapi/linux/virtio_config.h
 */
#define VIRTIO_CONFIG_S_DRIVER_OK	4

#define PRU0_DRAM		0x00000			// Offset to DRAM

#define NANO_SEC_PER_CYCLE  5
#define MILLISECOND_PER_NANOSECOND 1e6
#define MICROSECOND_PER_NANOSECOND 1e3
#define NANOSECOND_PER_SECOND 1e9
#define MICROSECOND_PER_SECOND 1e6
#define MILLISECOND_PER_SECOND 1e3
#define ONE_SECOND ( ((1 * (NANOSECOND_PER_SECOND))/(NANO_SEC_PER_CYCLE)) )
#define ONE_MILLISECOND ((ONE_SECOND)/(MILLISECOND_PER_SECOND))
#define ONE_MICROSECOND ((ONE_SECOND)/(MICROSECOND_PER_SECOND))

//====================
// MSG Codes
//====================
#define CODE_DRAW           -1
#define CODE_CLEAR          -2
#define CODE_MOTOR          -3

//====================
// NeoPixel
//====================
/*
#define zeroCyclesOn	    350/NANO_SEC_PER_CYCLE      // T0H
#define	oneCyclesOn		    700/NANO_SEC_PER_CYCLE	    // T1H Stay on for 700ns
#define zeroCyclesOff   	800/NANO_SEC_PER_CYCLE      // T0L
#define oneCyclesOff	    600/NANO_SEC_PER_CYCLE      // T1L
#define resetCycles		    70000/NANO_SEC_PER_CYCLE    // Must be at least 50u, use 51u. 70us seems to be the sweet spot
*/

#define STR_LEN             42                          // Number of pixels
                                                        //
#define zeroCyclesOn	    350/NANO_SEC_PER_CYCLE      // T0H
#define	oneCyclesOn		    700/NANO_SEC_PER_CYCLE	    // T1H Stay on for 700ns
#define zeroCyclesOff   	850/NANO_SEC_PER_CYCLE      // T0L
#define oneCyclesOff	    525/NANO_SEC_PER_CYCLE      // T1L
#define resetCycles		    70000/NANO_SEC_PER_CYCLE    // Must be at least 50u, use 51u. 70us seems to be the sweet spot

#define PREDEFINED_SEGMENT_COUNT 4

#define INDEX_DESTINATION_BUFFER_WRITE_START   STR_LEN                                                             // For user defined fades
#define INDEX_DESTINATION_BUFFER_WRITE_END     (STR_LEN + (STR_LEN - 1))
#define INDEX_DEFAULT_SEGMENT_START            (INDEX_DESTINATION_BUFFER_WRITE_END + 1)                            // For built in entire segment fades
#define INDEX_DEFAULT_SEGMENT_END              (INDEX_DEFAULT_SEGMENT_START + (PREDEFINED_SEGMENT_COUNT - 1))
#define NEOPIXEL_END_INDEX                     (INDEX_DEFAULT_SEGMENT_END)

#define DELTA_US(start, stop) (((stop).tv_sec - (start).tv_sec) * 1000000 + ((stop).tv_usec - (start).tv_usec))
#define DELTA_MS(start, stop) (DELTA_US((start), (stop))/1000)
#define RED(color)            (((color) & 0x0000FF00) >> 8)
#define GREEN(color)          (((color) & 0x00FF0000) >> 16)
#define BLUE(color)           (((color) & 0x000000FF) >> 0)
#define PACK_COLOR(r, g, b)   (((g)<<16)|((r)<<8)|(b))

#define FADE_SPEED 3

#define START 0                 // Segment begin index
#define END 1                   // Segment end index

#define NEOPIXEL_OUT P9_29
//====================
// Motor Driver
//====================
#define LIMIT_SWITCH_ONE (P9_25)
#define LIMIT_SWITCH_TWO (P9_27)
#define MOTOR_A11 (P9_28)
#define MOTOR_A12 (P9_30)
#define MOTOR_PWM (P9_31)

#define LOW(pin)  ( ((__R30) & (~pin))  )
#define HIGH(pin) ( ((__R30) | (pin))  )

#define LIMIT_SWITCH_ONE_PRESSED  "L11\n"   // Front button
#define LIMIT_SWITCH_ONE_RELEASED "L10\n"
#define LIMIT_SWITCH_TWO_PRESSED  "L21\n"   // Rear button
#define LIMIT_SWITCH_TWO_RELEASED "L20\n"
#define LS_MSG_LEN  4

#define MOTOR_STATE_CCW     "MCCw\n"
#define MOTOR_STATE_CW      "MCw\n"
#define MOTOR_STATE_STOP    "MS\n"
#define MOTOR_STATE_BRAKE   "MB\n"
#define MOTOR_STATE_UNKNOWN "UNK\n"

#define BUTTON_DEBONUCE_DELAY (5 * ONE_MILLISECOND)

#define MOTOR_STOP 0
#define MOTOR_BRAKE 1
#define MOTOR_CW 2                          // Drive pawl backward
#define MOTOR_CCW 3                         // Drive pawl forward

#define READ_GPIO(pin)  ((__R31) & (pin))
#define IS_PRESSED(pin) ( (__R31) & (pin) )
//====================


volatile register uint32_t __R30;
volatile register uint32_t __R31;

// Skip the first 0x200 byte of DRAM since the Makefile allocates
// 0x100 for the STACK and 0x100 for the HEAP.
volatile unsigned int *pru0_dram = (unsigned int *) (PRU0_DRAM + 0x200);

char payloadRX[RPMSG_BUF_SIZE];
struct pru_rpmsg_transport transport;

//====================
// NeoPixel Globals
//====================
uint32_t color[STR_LEN];    	// 3 bytes each: green, red, blue
uint32_t destColor[STR_LEN];	// If dest color differs from color, transition from color to dest.
// Default segments
size_t segments[PREDEFINED_SEGMENT_COUNT][2] = {       //[x][y] x segments, y segment range: begin index, end index
    {0, STR_LEN-1},             // Entire display
    {0, 5},                     // 6 pixels long
    {6, 15},                    // 10 pixels long
    {16, STR_LEN-1}             // 26 pixels long
};

void initColors(void);
void drawToLEDs(void);
bool doFade(void);
int8_t convergeFactor(uint8_t a, uint8_t b);
//====================

//======================
// Motor Driver Globals
//======================
unsigned int prevButState1 = 0, prevButState2 = 0, butState = 0;
int motorState = MOTOR_STOP, prevMotorState = MOTOR_STOP;

void motorCCw(/*double dutyCycle*/ void);
void motorCw(/*double dutyCycle*/ void);
void motorBrake(void);
void motorStop(void);
void processMotorState(void);
void notifyButtonStateChange(void);
void notifiyMotorStateChange(void);
void notifiyStateChanges(void);
//======================

/*
 * main.c
*/
void main(void) {
	volatile uint8_t *status;
	uint8_t r, g, b;
	uint16_t srcRX, dstRX, lenRX;
    uint32_t colr;
	int i, k=0;
    int code;	// Command code or index of LED to control
    char *rest;	// rest of payload after front character is removed

    initColors();
    motorStop();

	/* Allow OCP master port access by the PRU so the PRU can read external memories */
	CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

	/* Clear the status of the PRU-ICSS system event that the ARM will use to 'kick' us */
	CT_INTC.SICR_bit.STS_CLR_IDX = FROM_ARM_HOST;

	/* Make sure the Linux drivers are ready for RPMsg communication */
	status = &resourceTable.rpmsg_vdev.status;
	while (!(*status & VIRTIO_CONFIG_S_DRIVER_OK));

	/* Initialize the RPMsg transport structure */
	pru_rpmsg_init(&transport, &resourceTable.rpmsg_vring0, &resourceTable.rpmsg_vring1, TO_ARM_HOST, FROM_ARM_HOST);

    /* Receive all available messages, multiple messages can be sent per kick */
	/* Create the RPMsg channel between the PRU and ARM user space using the transport structure. https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/tree/include/pru_rpmsg.h?id=aa9606013059eb8728bcc1165c5032f0589469e0 */
	while (pru_rpmsg_channel(RPMSG_NS_CREATE, &transport, CHAN_RX_NAME, CHAN_RX_DESC, CHAN_RX_PORT) != PRU_RPMSG_SUCCESS);
	while (pru_rpmsg_channel(RPMSG_NS_CREATE, &transport, CHAN_TX_NAME, CHAN_TX_DESC, CHAN_TX_PORT) != PRU_RPMSG_SUCCESS);
	while (1) {
		/* Check bit 30 of register R31 to see if the ARM has kicked us */
		if (__R31 & HOST_INT) {
			/* Clear the event status */
			CT_INTC.SICR_bit.STS_CLR_IDX = FROM_ARM_HOST;
			/* Receive all available messages, multiple messages can be sent per kick */
			while (pru_rpmsg_receive(&transport, &srcRX, &dstRX, payloadRX, &lenRX) == PRU_RPMSG_SUCCESS) {
                // Input format is: code <data>
                code = atoi(payloadRX);
                rest = strchr(payloadRX, ' ');	// Skip over code

                switch(code) {
                    case CODE_DRAW:                                     // NeoPixel Msg: send the array to the LED string
                    do {
                        drawToLEDs();
                    } while(doFade());
                    break;
                case CODE_CLEAR:                                        // NeoPixel Msg: Clear the display
                    for(k=0; k < STR_LEN; k++){
                        color[k] = destColor[k] = 0;
                    }
                    drawToLEDs();
                    break;
                case CODE_MOTOR:                                        // Motor MSG: Perform motor movements
                    // Input format is:  [MOTOR_STOP, MOTOR_BRAKE, MOTOR_CW, MOTOR_CCW] [dutyCycle]
                    motorState = atoi(&rest[1]);
                    //rest = strchr(&rest[1], ' ');	// Skip over dir, etc.
                    //dc = atof(&rest[1]);   // duty cycle is optional so if it's zero we'll ignore it.
                    break;
                default:                                                // All others are NeoPixel Pixel Index
                    if(code < 0 || code > INDEX_DEFAULT_SEGMENT_END) continue;

                    // Input format is:  index red green blue
                    r = strtol(&rest[1], NULL, 0);
                    rest = strchr(&rest[1], ' ');	// Skip over r, etc.
                    g = strtol(&rest[1], NULL, 0);
                    rest = strchr(&rest[1], ' ');
                    b = strtol(&rest[1], NULL, 0);
                    colr = PACK_COLOR(r, g, b);	// String wants GRB

                    if((code >= 0) & (code < STR_LEN)) {                      // Codes to write to pixel buffer. Update the array, but don't write/draw it out.
                        color[code] = destColor[code] = colr;
                    }else if(code >= INDEX_DESTINATION_BUFFER_WRITE_START && code <= INDEX_DESTINATION_BUFFER_WRITE_END) {   // Codes to write to destination buffer only
                        destColor[code-INDEX_DESTINATION_BUFFER_WRITE_START] = colr;
                    } else if (code >= INDEX_DEFAULT_SEGMENT_START && code <= INDEX_DEFAULT_SEGMENT_END) {  // Codes to write to default segments
                        for(i=segments[code - INDEX_DEFAULT_SEGMENT_START][START]; i<=segments[code - INDEX_DEFAULT_SEGMENT_START][END]; i++) {
                            destColor[i] = colr;
                        }
                    }
                }
                processMotorState();
                notifiyStateChanges();
			}
		}

        processMotorState();
        notifiyStateChanges();
	}
}

void initColors(void){
    int i;
	// Set everything to background
	for(i=0; i<STR_LEN; i++) {
		color[i] = destColor[i] = 0x000000;
	}
}

void drawToLEDs(void){
	// Select which pins to output to.  These are all on pru1_1
	uint32_t gpio = P9_29;
    int i, j;

    for(j=0; j<STR_LEN; j++) {
        // Cycle through each bit
        for(i=23; i>=0; i--) {
            if(color[j] & (0x1<<i)) {
                __R30 |= gpio;		// Set the GPIO pin to 1
                __delay_cycles(oneCyclesOn-1);
                __R30 &= ~gpio;		// Clear the GPIO pin
                __delay_cycles(oneCyclesOff-14);
            } else {
                __R30 |= gpio;		// Set the GPIO pin to 1
                __delay_cycles(zeroCyclesOn-1);
                __R30 &= ~gpio;		// Clear the GPIO pin
                __delay_cycles(zeroCyclesOff-14);
            }
        }
    }
    // Send Reset
    __R30 &= ~gpio;	// Clear the GPIO pin
    __delay_cycles(resetCycles);
}

/*
 * doFade
 * return: true (1) if the function should be called again to continue the fade. False if fade is complete.
 */
bool doFade(void) {
	uint8_t r, g, b, d_r, d_g, d_b;
    int i;
    bool colorNeedsFade = false;
    for(i=0; i < STR_LEN; i++){
        if(color[i] == destColor[i]) continue;
        colorNeedsFade = true;
        b = BLUE(color[i]);
        r = RED(color[i]);
        g = GREEN(color[i]);

        d_b = BLUE(destColor[i]);
        d_r = RED(destColor[i]);
        d_g = GREEN(destColor[i]);

        b += convergeFactor(b, d_b);
        r += convergeFactor(r, d_r);
        g += convergeFactor(g, d_g);

        color[i] = PACK_COLOR(r, g, b);
    }
    return colorNeedsFade;
}

int8_t convergeFactor(uint8_t a, uint8_t b) {
   int8_t dif = b - a;
   if(dif > FADE_SPEED) return FADE_SPEED;
   if(dif < -FADE_SPEED) return -FADE_SPEED;
   return dif;
}

void processMotorState(void) {
    switch(motorState) {
        case MOTOR_STOP:
            motorStop();
            break;
        case MOTOR_BRAKE:
            motorBrake();
            break;
        case MOTOR_CW:
            motorCw();
            break;
        case MOTOR_CCW:
            motorCCw();
            break;
        default:
            motorStop();
            motorState = MOTOR_STOP;
    }
}

void notifyButtonStateChange(void) {
    butState = READ_GPIO(LIMIT_SWITCH_ONE);
    if(butState != prevButState1) {
        prevButState1 = butState;
        if(butState) {
            pru_rpmsg_send(&transport, TX_SRC_ADDR, TX_DST_ADDR, LIMIT_SWITCH_ONE_PRESSED, LS_MSG_LEN);
        } else {
            pru_rpmsg_send(&transport, TX_SRC_ADDR, TX_DST_ADDR, LIMIT_SWITCH_ONE_RELEASED, LS_MSG_LEN);
        }
        __delay_cycles(BUTTON_DEBONUCE_DELAY);
    }

    butState = READ_GPIO(LIMIT_SWITCH_TWO);
    if(butState != prevButState2) {
        prevButState2 = butState;
        if(butState) {
            pru_rpmsg_send(&transport, TX_SRC_ADDR, TX_DST_ADDR, LIMIT_SWITCH_TWO_PRESSED, LS_MSG_LEN);
        } else {
            pru_rpmsg_send(&transport, TX_SRC_ADDR, TX_DST_ADDR, LIMIT_SWITCH_TWO_RELEASED, LS_MSG_LEN);
        }
        __delay_cycles(BUTTON_DEBONUCE_DELAY);
    }
}

void notifiyMotorStateChange(){
    if(motorState != prevMotorState) {
        switch(motorState) {
            case MOTOR_STOP:
                pru_rpmsg_send(&transport, TX_SRC_ADDR, TX_DST_ADDR, MOTOR_STATE_STOP, 3);
                break;
            case MOTOR_BRAKE:
                pru_rpmsg_send(&transport, TX_SRC_ADDR, TX_DST_ADDR, MOTOR_STATE_BRAKE, 3);
                break;
            case MOTOR_CW:
                pru_rpmsg_send(&transport, TX_SRC_ADDR, TX_DST_ADDR, MOTOR_STATE_CW, 4);
                break;
            case MOTOR_CCW:
                pru_rpmsg_send(&transport, TX_SRC_ADDR, TX_DST_ADDR, MOTOR_STATE_CCW, 5);
                break;
            default:
                pru_rpmsg_send(&transport, TX_SRC_ADDR, TX_DST_ADDR, MOTOR_STATE_UNKNOWN, 4);
        }
        prevMotorState = motorState;
    }
}

void notifiyStateChanges(void) {
    notifyButtonStateChange();
    notifiyMotorStateChange();
}

void motorCCw(/*double dutyCycle*/ void) {
    __R30 = LOW(MOTOR_A11);
    __R30 = HIGH(MOTOR_A12);
    if(IS_PRESSED(LIMIT_SWITCH_TWO)){
        __R30 = LOW(MOTOR_PWM);
    }else{
        __R30 = HIGH(MOTOR_PWM);
    }
}

void motorCw(/*double dutyCycle*/ void){
    __R30 = HIGH(MOTOR_A11);
    __R30 = LOW(MOTOR_A12);
    if(IS_PRESSED(LIMIT_SWITCH_ONE)) {
        __R30 = LOW(MOTOR_PWM);
    }else{
        __R30 = HIGH(MOTOR_PWM);
    }
}

void motorStop(void) {
    __R30 = LOW(MOTOR_A11);
    __R30 = LOW(MOTOR_A12);
    __R30 = HIGH(MOTOR_PWM);
}

void motorBrake(void) {
    __R30 = LOW(MOTOR_PWM);
}

// Turns off triggers and sets pinmux
#pragma DATA_SECTION(init_pins, ".init_pins")
#pragma RETAIN(init_pins)
const char init_pins[] =
	"/sys/class/leds/beaglebone:green:usr3/trigger\0none\0" \
	"/sys/devices/platform/ocp/ocp:P9_25_pinmux/state\0pruin\0" \
	"/sys/devices/platform/ocp/ocp:P9_27_pinmux/state\0pruin\0" \
    "/sys/devices/platform/ocp/ocp:P9_28_pinmux/state\0pruout\0" \
    "/sys/devices/platform/ocp/ocp:P9_29_pinmux/state\0pruout\0" \
    "/sys/devices/platform/ocp/ocp:P9_30_pinmux/state\0pruout\0" \
    "/sys/devices/platform/ocp/ocp:P9_31_pinmux/state\0pruout\0" \
	"\0\0";
