/*
 * timingTest.c
 *
 * Simple mock for testing PRU code
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>			// atoi
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/time.h>

#define OPTSTR "vi:o:h"
#define USAGE_FMT  "%s [-v] [-i inputfile] [-o outputfile] [-h]"
#define ERR_FOPEN_INPUT  "fopen(input, r)"
#define ERR_FOPEN_OUTPUT "fopen(output, w)"
#define ERR_DO_THE_NEEDFUL "do_the_needful blew up"
#define DEFAULT_PROGNAME "timingTest"

#define STR_LEN 42
#define	oneCyclesOn		700/5	// Stay on for 700ns
#define oneCyclesOff	600/5
#define zeroCyclesOn	350/5
#define zeroCyclesOff	800/5
#define resetCycles		51000/5	// Must be at least 50u, use 51u
#define SPEED 20000000/5		// Time to wait between updates

#define START 0                 // Segment index
#define END 1                   // Segment index
#define SEGMENT_ONE 0
#define SEGMENT_TWO 1
#define SEGMENT_THREE 2
#define CLOCK_TICK_MS 10        // Segment max clock tick

#define CODE_DRAW -1
#define CODE_COLOR_SEGMENT_ONE 127
#define CODE_COLOR_SEGMENT_TWO 128


//This is defined in the PRU header. Mocked here.
#define RPMSG_BUF_SIZE 100
#define P9_29 29
#define PRU_RPMSG_SUCCESS 1
#define PRU_RPMSG_NO_BUF_AVAILABLE 2
#define PRU_RPMSG_INVALID_HEAD 3

#define DELTA_US(start, stop) (((stop).tv_sec - (start).tv_sec) * 1000000 + ((stop).tv_usec - (start).tv_usec))
#define DELTA_MS(start, stop) (DELTA_US((start), (stop))/1000)

extern int errno;
extern char *optarg;
extern int opterr, optind;

typedef struct {
  int           verbose;
  uint32_t      flags;
  FILE         *input;
  FILE         *output;
} options_t;

struct pru_rpmsg_transport {};
uint32_t __R30;
uint32_t __R31;
char payload[RPMSG_BUF_SIZE];


uint32_t color[STR_LEN];	    // 3 bytes each: green, red, blue
uint32_t destColor[STR_LEN];	// 3 bytes each: green, red, blue
size_t segments[3][2] = {
       {0, 5},   // 6 pixels long
       {6, 15},  // 10 pixels long
       {16, 41} // 26 pixels long
};



int16_t pru_rpmsg_receive (
    struct pru_rpmsg_transport 	*transport,
    uint16_t 					*src,
    uint16_t 					*dst,
    void 						*data,
    uint16_t 					*len
);
void __delay_cycles(unsigned long n);
void drawToLEDs(void);
void updateSegments(void);


int main(int argc, char *argv[])
{
	struct pru_rpmsg_transport transport;
	uint16_t src, dst, len;
	uint8_t r, g, b;
	int i, iterations = 1;
    struct timeval stop, start;
    uint64_t delta_us;
    uint32_t colr;

	// Set everything to background
	for(i=0; i<STR_LEN; i++) {
		color[i] = 0x00010000;
	}

	while (iterations) {
        gettimeofday(&start, NULL);
        while (pru_rpmsg_receive(&transport, &src, &dst, payload, &len) == PRU_RPMSG_SUCCESS) {
            char *ret;	// rest of payload after front character is removed
            int index;	// index of LED to control

            printf("Input = '%s'\n", payload);

            // Input format is:  index red green blue
            index = atoi(payload);
            ret = strchr(payload, ' ');	// Skip over index
            r = strtol(&ret[1], NULL, 0);
            ret = strchr(&ret[1], ' ');	// Skip over r, etc.
            g = strtol(&ret[1], NULL, 0);
            ret = strchr(&ret[1], ' ');
            b = strtol(&ret[1], NULL, 0);
            colr = (g<<16)|(r<<8)|b;	// String wants GRB

            // Update the array, but don't write it out.
            if((index >=0) & (index < STR_LEN)) {
                color[index] = colr;
            }else {
                switch(index) {
                case CODE_DRAW:                // Index = CODE_DRAW; send the array to the LED string
                    drawToLEDs(); // Output the string
                    break;
                case CODE_COLOR_SEGMENT_ONE:   // Index = CODE_COLOR_SEGMENT_ONE
                    for(int i=segments[SEGMENT_ONE][START]; i<segments[SEGMENT_ONE][END]; i++){
                        color[i] = colr;
                    }
                    break;
                }
            }

            gettimeofday(&stop, NULL);
            if(DELTA_MS(start, stop) >= CLOCK_TICK_MS){
                updateSegments();
            }

            break;  //TODO: REMOVE
        }
        gettimeofday(&stop, NULL);
        if(DELTA_MS(start, stop) >= CLOCK_TICK_MS){
            updateSegments();
        }

        iterations--;
    }

    return EXIT_SUCCESS;
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

    // Wait
    __delay_cycles(SPEED);
}

void updateSegments(void){
}


/* MOCK: Receive all available messages, multiple messages can be sent per kick */
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
*/
int16_t pru_rpmsg_receive (
    struct pru_rpmsg_transport 	*transport,
    uint16_t 					*src,
    uint16_t 					*dst,
    void 						*data,
    uint16_t 					*len
) {
    char str[] = "0 255 0 0\n";
    char *buf = data;
    strcpy(buf, str);
    *len = strlen(str);
    return PRU_RPMSG_SUCCESS;
}

/*
 * MOCK: Basic mock for __delay_cycles
 */
void __delay_cycles(unsigned long n)
{
    usleep(n * 0.001);
}
