#ifndef _SEQUENCER_
#define _SEQUENCER_


// This is necessary for CPU affinity macros in Linux
#define _GNU_SOURCE

#define USEC_PER_MSEC (1000)
#define NANOSEC_PER_MSEC (1000000)
#define NANOSEC_PER_SEC (1000000000)
#define NUM_CPU_CORES (4)
#define TRUE (1)
#define FALSE (0)

#define RT_CORE (2)

#define NUM_THREADS (3)


// Of the available user space clocks, CLOCK_MONONTONIC_RAW is typically most precise and not subject to
// updates from external timer adjustments
//
// However, some POSIX functions like clock_nanosleep can only use adjusted CLOCK_MONOTONIC or CLOCK_REALTIME
//
//#define MY_CLOCK_TYPE CLOCK_REALTIME
//#define MY_CLOCK_TYPE CLOCK_MONOTONIC
#define MY_CLOCK_TYPE CLOCK_MONOTONIC_RAW
//#define MY_CLOCK_TYPE CLOCK_REALTIME_COARSE
//#define MY_CLOCK_TYPE CLOCK_MONTONIC_COARSE

void Sequencer(int id);

void *Service_1_frame_acquisition(void *threadp);
void *Service_2_frame_process(void *threadp);
void *Service_3_frame_storage(void *threadp);

int seq_frame_read(void);
int seq_frame_process(void);
int seq_frame_store(void);

double getTimeMsec(void);
double realtime(struct timespec *tsptr);
void print_scheduler(void);

int v4l2_frame_acquisition_initialization(char *dev_name);
int v4l2_frame_acquisition_shutdown(void);
int v4l2_frame_acquisition_loop(char *dev_name);

#endif
