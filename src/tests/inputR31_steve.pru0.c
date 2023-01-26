////////////////////////////////////////
//	blinkR30.pru0.c
//	Reads input in P9_25 via the R31 register and blinks the USR3 LED
//	Wiring:	A switch between P9_25 and 3.3V (P9_3 or P9_4)
//	Setup:	config_pin P9_25 pruin
//	See:	prugpio.h to see which pins attach to R31
//	PRU:	pru0
////////////////////////////////////////
/* Pin Table
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
*/
/* These are defined in prugpio.h
// P9
#define P9_14 (1<<18)
#define P9_16 (1<<19)
#define P9_19	(1<<2)
#define P9_20	(1<<1)
#define P9_25   (1<<7)  // Limit One
#define P9_27   (1<<5)  // Limit Two
#define P9_28   (1<<3)  // Motor A11
#define P9_29   (1<<1)  // NeoPixel
#define P9_30   (1<<2)  // Motor A12
#define P9_31   (1<<0)  // Motor PWM
#define P9_92   (1<<4)
#define P9_91   (1<<6)


// P8
#define P8_13	(1<<7)
#define P8_15	(1<<16)
#define P8_16	(1<<18)
#define P8_17   (1<18)
#define P8_18	(1<<5)
#define P8_19	(1<<6)
#define P8_26	(1<<17)
*/
/* Give Python access to the shared memory
$ sudo setcap "cap_dac_override+ep cap_sys_rawio+ep" /media/sdcard/home/cck/.pyenv/versions/3.7.13/bin/python3.7
$ getcap /media/sdcard/home/cck/.pyenv/versions/3.7.13/bin/python3.7
/media/sdcard/home/cck/.pyenv/versions/3.7.13/bin/python3.7 = cap_dac_override,cap_sys_rawio+ep
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>			// atoi
#include <string.h>
#include <stdbool.h>
#include <pru_cfg.h>
#include <pru_intc.h>
#include <rsc_types.h>
#include <pru_rpmsg.h>
// #include "resource_table_empty.h"
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
#define CHAN_NAME			"rpmsg-pru"
#define CHAN_DESC			"Channel 30"
#define CHAN_PORT			30

/*
 * Used to make sure the Linux drivers are ready for RPMsg communication
 * Found at linux-x.y.z/include/uapi/linux/virtio_config.h
 */
#define VIRTIO_CONFIG_S_DRIVER_OK	4


#define NANO_SEC_PER_CYCLE  5
#define NANOSECOND_PER_SECOND 1e9
#define FIVE_SECONDS ( ((5 * (NANOSECOND_PER_SECOND))/(NANO_SEC_PER_CYCLE)) )

#define LIMIT_SWITCH_ONE (P9_25)
#define LIMIT_SWITCH_TWO (P9_27)
#define MOTOR_A11 (P9_28)
#define MOTOR_A12 (P9_30)
#define MOTOR_PWM (P9_31)

#define LOW(pin)  ( ((__R30) & (~pin))  )
#define HIGH(pin) ( ((__R30) | (pin))  )

#define PRU0_DRAM		0x00000			// Offset to DRAM

volatile register unsigned int __R30; // Output register
volatile register unsigned int __R31; // Input register
// Skip the first 0x200 byte of DRAM since the Makefile allocates
// 0x100 for the STACK and 0x100 for the HEAP.
volatile unsigned int *pru0_dram = (unsigned int *) (PRU0_DRAM + 0x200);

char payload[RPMSG_BUF_SIZE] = "TEST STEVE";

void ccw(void);
void cw(void);
void brake(void);
void stop(void);

void main(void) {
	struct pru_rpmsg_transport transport;
	uint16_t src = 1024, dst = 30, len=7;
	volatile uint8_t *status;

	uint32_t *gpio1 = (uint32_t *)GPIO1;

	/* Clear SYSCFG[STANDBY_INIT] to enable OCP master port */
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
    /** https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/tree/include/pru_rpmsg.h?id=aa9606013059eb8728bcc1165c5032f0589469e0
    * Summary		:	pru_rpmsg_receive receives a message, if available, from
    * 					the ARM host.
    *
    * Parameters	:	transport: a pointer to the transport layer from which the
    * 							   message should be received
    * 					src: a pointer that is populated with the source address
    * 						 where the message originated
    * 					dst: a pointer that is populated with the destination
    * 						 address where the message was sent (can help determine
    * 						 for which channel the message is intended on the PRU)
    * 					data: a pointer that is populated with a local data buffer
    * 						  containing the message payload
    * 					len: a pointer that is populated with the length of the
    * 						 message payload
    *
    * Description	:	pru_rpmsg_receive uses the pru_virtqueue interface to get
    * 					an available buffer, copy the buffer into local memory,
    * 					add the buffer as a used buffer to the vring, and then kick
    * 					the remote processor if necessary. The src, dst, data, and
    * 					len pointers are populated with the information about the
    * 					message and local buffer data if the reception is
    * 					successful.
    *
    * Return Value	:	Returns PRU_RPMSG_NO_BUF_AVAILABLE if there is currently no
    * 					buffer available for receive. Returns PRU_RPMSG_INVALID_HEAD
    * 					if the head index returned for the available buffer is
    * 					invalid. Returns PRU_RPMSG_SUCCESS if the message is
    * 					successfully received.
        int16_t pru_rpmsg_receive (
            struct pru_rpmsg_transport 	*transport,
            uint16_t 					*src,
            uint16_t 					*dst,
            void 						*data,
            uint16_t 					*len
        );
    */
	/* Create the RPMsg channel between the PRU and ARM user space using the transport structure. */
	while (pru_rpmsg_channel(RPMSG_NS_CREATE, &transport, CHAN_NAME, CHAN_DESC, CHAN_PORT) != PRU_RPMSG_SUCCESS);

    // Write some data to shared mem. Will attempt to read this in Python
    pru0_dram[0] = 0xBEFFAEED;

	while(1) {
        /**
        * Summary		:	pru_rpmsg_send sends a message to the ARM host using the
        * 					virtqueues in the pru_rpmsg_transport structure. The
        * 					source and destination address of the message are passed
        * 					in as parameters to the function. The data to be sent and
        * 					its length are passed in the data and len parameters.
        *
        * Parameters	:	transport: a pointer to the transport layer from which the
        * 							   message should be sent
        * 					src: the source address where this message will originate
        * 					dst: the destination address where the message will be sent
        * 					data: a pointer to a local data buffer containing the
        * 						  message payload
        * 					len: the length of the message payload
        *
        * Description	:	pru_rpmsg_send sends a message to the src parameter and
        * 					from the dst parameter. The transport structure defines the
        * 					underlying transport mechanism that will be used. The
        * 					data parameter is a pointer to a local buffer that should
        * 					be sent to the destination address and the len parameter is
        * 					the length of that buffer.
        *
        * Return Value	:	Returns PRU_RPMSG_NO_BUF_AVAILABLE if there is currently no
        * 					buffer available for send. Returns PRU_RPMSG_BUF_TOO_SMALL
        * 					if the buffer from the vring is too small to hold the
        * 					message payload being sent. Returns PRU_RPMSG_INVALID_HEAD
        * 					if the head index returned for the send buffer is invalid.
        * 					Returns PRU_RPMSG_SUCCESS if the message is successfully
        * 					sent.
            int16_t pru_rpmsg_send (
                struct pru_rpmsg_transport 	*transport,
                uint32_t 					src,
                uint32_t 					dst,
                void 						*data,
                uint16_t 					len
            );
        */
        /* Check bit 30 of register R31 to see if the ARM has kicked us */
        if (__R31 & HOST_INT) {
            /* Clear the event status */
            CT_INTC.SICR_bit.STS_CLR_IDX = FROM_ARM_HOST;
            /* Receive all available messages, multiple messages can be sent per kick */
            pru_rpmsg_send(&transport, src, dst, &payload, len);
        }
        __delay_cycles(FIVE_SECONDS);

		if(__R31 & P9_25) {
            gpio1[GPIO_SETDATAOUT]   = USR3;      // Turn on LED
        } else {
            gpio1[GPIO_CLEARDATAOUT] = USR3;      // Turn off LED
        }

		if(__R31 & P9_27) {
            gpio1[GPIO_SETDATAOUT]   = USR1;      // Turn on LED
        } else {
            gpio1[GPIO_CLEARDATAOUT] = USR1;      // Turn off LED
        }
        // Test motor
        /* stop(); */
        /* ccw(); */
        /* __delay_cycles(FIVE_SECONDS); */
        /* brake(); */
        /* __delay_cycles(1000000000); */
        /* cw(); */
        /* __delay_cycles(1000000000); */
        /* brake(); */
        /* __delay_cycles(1000000000); */
        /* stop(); */
        /* break; */
	}
	__halt();
}

void ccw(void) {
    __R30 = LOW(MOTOR_A11);
    __R30 = HIGH(MOTOR_A12);
    __R30 = HIGH(MOTOR_PWM);
}

void cw(void){
    __R30 = HIGH(MOTOR_A11);
    __R30 = LOW(MOTOR_A12);
    __R30 = HIGH(MOTOR_PWM);
}

void stop(void) {
    __R30 = LOW(MOTOR_A11);
    __R30 = LOW(MOTOR_A12);
    __R30 = HIGH(MOTOR_PWM);
}

void brake(void) {
    __R30 = LOW(MOTOR_A11);
    __R30 = LOW(MOTOR_A12);
    __R30 = HIGH(MOTOR_PWM);
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
