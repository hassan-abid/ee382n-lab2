
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include "MemoryAccess.h"

#define LFSR 0x43C00000
#define BRAM_CTRL_0 0x40000000
#define BRAM_CTRL_1 0x40002000
#define GPIO 0x43C10000
/*
slvreg0 -> offset 0 controls interrupt start
slvreg1 -> offset 4 controls LEDs
slvreg2 -> offset 8 reads Switches
*/


#define fpga_MAJOR 200
#define MAP_SIZE 4096UL                                                                                     
#define MAP_MASK (MAP_SIZE - 1)
#define NUM_MEASURE 10000   

int sigio_signal_processed = 0;
int num_int = 0;
unsigned long intr_latency[NUM_MEASURE];

struct timeval start_timestamp, sigio_signal_timestamp;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*  This routine would do the interrupt handling for the main loop.
*
*/

void sighandler(int signo)
{
    assert(signo == SIGIO); // Confirm correct signal
    num_int++;
    //printf("sigio_signal_handler called (signo=%d)\n", signo); // DEBUG    

    (void)gettimeofday(&sigio_signal_timestamp, NULL);
    //set pin to low
    pm(GPIO,0x00);

    sigio_signal_processed = 1;

}

char buffer[4096];

int main(int argc, char **argv)
{
    int count,i;
    struct sigaction action;
    int fd;
    int rc;
    int fc;
    // CSV file generation
    char* filename = "interrupt_latency.csv";
    FILE *fpcsv;
    fpcsv = fopen(filename, "w+");
    if (fpcsv == NULL) {
    	perror("Unable to open csv file");
    	exit (-1);
    }

    memset(&action, 0, sizeof(action));
    //sigemptyset(&action.sa_mask);
    //sigaddset(&action.sa_mask, SIGIO);
    
    action.sa_handler = sighandler;
    //action.sa_flags = 0;

    (void)sigfillset(&action.sa_mask);

    rc = sigaction(SIGIO, &action, NULL);
    if (rc == -1) {
        perror("sigaction() failed");
        return -1;
    }
    
    fd = open("/dev/fpga_int", O_RDWR);
    if (fd == -1) {
    	perror("Unable to open /dev/fpga_int");
    	rc = fd;
    	exit (-1);
    }
    
    printf("\nmon_interrupt: /dev/fpga_int opened successfully\n");    	

    fc = fcntl(fd, F_SETOWN, getpid());
    
    if (fc == -1) {
    	perror("SETOWN failed\n");
    	rc = fd;
    	exit (-1);
    } 

      
    fc= fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_ASYNC);

    if (fc == -1) {
    	perror("SETFL failed\n");
    	rc = fd;
    	exit (-1);
    }   

    
    sigset_t signal_mask, signal_mask_old, signal_mask_most;
    

/*   This while loop emulates a program running the main
*   loop i.e. sleep(). The main loop is interrupted when
*   the SIGIO signal is received.
*/

    for(i = 0; i < NUM_MEASURE; i++)
    {

	    sigio_signal_processed = 0;

	    (void)sigfillset(&signal_mask);
	    (void)sigfillset(&signal_mask_most);
	    (void)sigdelset(&signal_mask_most, SIGIO);
	    (void)sigprocmask(SIG_SETMASK,&signal_mask, &signal_mask_old);

	    (void)gettimeofday(&start_timestamp, NULL);
	    //set pin to high to trigger interrupt
	    pm(GPIO,0x01);
	    //(void)gettimeofday(&start_timestamp, NULL);

	    if (sigio_signal_processed == 0) {
	        rc = sigsuspend(&signal_mask_most);
	        /* Confirm we are coming out of suspend mode correcly */
	        assert(rc == -1 && errno == EINTR && sigio_signal_processed);
	    }

	    (void)sigprocmask(SIG_SETMASK, &signal_mask_old, NULL);
	    assert(num_int == i + 1); // Critical assertion!!
	    
	    intr_latency[i] = (sigio_signal_timestamp.tv_sec - start_timestamp.tv_sec) * 1000000 + (sigio_signal_timestamp.tv_usec - start_timestamp.tv_usec);
	    //printf("latency : %lu\n", intr_latency);
	}
    close(fd);

// Calculation
    unsigned long min_lat=intr_latency[0], max_lat=intr_latency[0], avg_lat=0, st_dev=0;

    for(i = 0; i < NUM_MEASURE; i++)
    {
    	if(intr_latency[i] < min_lat)
    		min_lat = intr_latency[i];

    	if(intr_latency[i] > max_lat)
    		max_lat = intr_latency[i];

    	avg_lat += intr_latency[i];
    }
    avg_lat = avg_lat/NUM_MEASURE;

    for(i = 0; i < NUM_MEASURE; i++)
    {
    	fprintf(fpcsv, "%lu\n", intr_latency[i]);
    	st_dev += (intr_latency[i]-avg_lat)*(intr_latency[i]-avg_lat);
    }
    st_dev = st_dev/NUM_MEASURE;
    fclose(fpcsv);

    printf("Minimum Latency:\t%lu\n", min_lat);
    printf("Maximum Latency:\t%lu\n", max_lat);
    printf("Average Latency:\t%lu\n", avg_lat);
    printf("Variance:\t\t%lu\n", st_dev);
    printf("Number of Samples:\t%d\n", NUM_MEASURE);

    i = system("more /proc/interrupts | grep 164");
}
