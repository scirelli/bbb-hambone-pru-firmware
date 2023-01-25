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
#include <pru_cfg.h>
#include "resource_table_empty.h"
#include "prugpio.h"

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

void ccw(void);
void cw(void);
void brake(void);
void stop(void);

void main(void) {
	uint32_t *gpio1 = (uint32_t *)GPIO1;

	/* Clear SYSCFG[STANDBY_INIT] to enable OCP master port */
	CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

    pru0_dram[0] = 0xBEFFAEED;

	while(1) {
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
