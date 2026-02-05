#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <syslog.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <time.h>
#include "../headers/capturelib.h"
#include "../headers/v4l2_interface.h"

// Format is used by a number of functions, so made as a file global
//static struct v4l2_format fmt;
//struct v4l2_buffer frame_buf;

struct buffer
{
        void   *start;
        size_t  length;
};


struct motion_buffer_t
{
    unsigned int ring_size;
    int tail_idx;
    int count;
    unsigned char frames[2][HRES*VRES];  // Store up to 2 grayscale frames for comparison
};

static struct motion_buffer_t motion_buffer = {
    .ring_size = 2,
    .tail_idx = 0,
    .count = 0
};

//int camera_device_fd = -1;
//struct buffer          *buffers;
//unsigned int n_buffers;
//int force_format=1;

//static double fnow=0.0, fstart=0.0, fstop=0.0;
//static struct timespec time_now, time_start, time_stop;

char ppm_header[]="P6\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
char ppm_dumpname[]="frames/test0000.ppm";

char pgm_header[]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
char pgm_dumpname[]="frames/test0000.pgm";

// always ignore STARTUP_FRAMES while camera adjusts to lighting, focuses, etc.
//int read_framecnt=-STARTUP_FRAMES;
int process_framecnt=0;
int save_framecnt=0;

//unsigned char scratchpad_buffer[MAX_HRES*MAX_VRES*MAX_PIXEL_SIZE];


//Placeholder until I get all the functions declared in the header, because the ordering here is messed up. 
int process_motion_detection(const unsigned char *current_frame, int frame_size);
void reset_motion_buffer(void);



static void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time)
{
    int written, i, total, dumpfd;
   
    snprintf(&ppm_dumpname[11], 9, "%04d", tag);
    strncat(&ppm_dumpname[15], ".ppm", 5);
    dumpfd = open(ppm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);

    snprintf(&ppm_header[4], 11, "%010d", (int)time->tv_sec);
    strncat(&ppm_header[14], " sec ", 5);
    snprintf(&ppm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));
    strncat(&ppm_header[29], " msec \n"HRES_STR" "VRES_STR"\n255\n", 19);

    // subtract 1 from sizeof header because it includes the null terminator for the string
    written=write(dumpfd, ppm_header, sizeof(ppm_header)-1);

    total=0;

    do
    {
        written=write(dumpfd, p, size);
        total+=written;
    } while(total < size);

    clock_gettime(CLOCK_MONOTONIC, &time_now);
    fnow = (double)time_now.tv_sec + (double)time_now.tv_nsec / 1000000000.0;
    printf("Frame written to flash at %lf, %d, bytes\n", (fnow-fstart), total);

    close(dumpfd);
    
}

static void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time)
{
    int written, i, total, dumpfd;
   
    snprintf(&pgm_dumpname[11], 9, "%04d", tag);
    strncat(&pgm_dumpname[15], ".pgm", 5);
    dumpfd = open(pgm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);

    snprintf(&pgm_header[4], 11, "%010d", (int)time->tv_sec);
    strncat(&pgm_header[14], " sec ", 5);
    snprintf(&pgm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));
    strncat(&pgm_header[29], " msec \n"HRES_STR" "VRES_STR"\n255\n", 19);

    // subtract 1 from sizeof header because it includes the null terminator for the string
    written=write(dumpfd, pgm_header, sizeof(pgm_header)-1);

    total=0;

    do
    {
        written=write(dumpfd, p, size);
        total+=written;
    } while(total < size);

    clock_gettime(CLOCK_MONOTONIC, &time_now);
    fnow = (double)time_now.tv_sec + (double)time_now.tv_nsec / 1000000000.0;
    printf("Frame written to flash at %lf, %d, bytes\n", (fnow-fstart), total);

    close(dumpfd);
    
}



static int save_image(const void *p, int size, struct timespec *frame_time)
{
    int i, newi, newsize=0;
    unsigned char *frame_ptr = (unsigned char *)p;

    // Next line should prob be a global var 
    int most_recent_idx = (motion_buffer.tail_idx - 1 + motion_buffer.ring_size) % motion_buffer.ring_size;
    save_framecnt++;
    printf("save frame %d: ", save_framecnt);
    

#ifdef DUMP_FRAMES	

    if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY)
    {
        printf("Dump graymap as-is size %d\n", size);
       //dump_pgm(frame_ptr, size, save_framecnt, frame_time);
        dump_pgm(motion_buffer.frames[most_recent_idx], size, save_framecnt, frame_time);
    }

    else if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
    {

#if defined(COLOR_CONVERT_RGB)
       
        if(save_framecnt > 0) 
        {
            dump_ppm(frame_ptr, ((size*6)/4), save_framecnt, frame_time);
            printf("Dump YUYV converted to RGB size %d\n", size);
        }
#elif defined(COLOR_CONVERT_GRAY)
        if(save_framecnt > 0)
        {  
            dump_pgm(motion_buffer.frames[most_recent_idx], HRES*VRES, save_framecnt, frame_time);
           // dump_pgm(frame_ptr, (size/2), process_framecnt, frame_time);
            printf("Dump YUYV converted to YY size %d\n", HRES*VRES);
        }
#endif

    }

    else if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24)
    {
        printf("Dump RGB as-is size %d\n", size);
        dump_ppm(frame_ptr, size, process_framecnt, frame_time);
    }
    else
    {
        printf("ERROR - unknown dump format\n");
    }
#endif

    return save_framecnt;
}



int seq_frame_store(void)
{
    int cnt;

   //cnt=save_image(scratchpad_buffer, HRES*VRES*PIXEL_SIZE, &time_now);   
   // Get the index of the most recent frame in motion_buffer
   int most_recent_idx = (motion_buffer.tail_idx - 1 + motion_buffer.ring_size) % motion_buffer.ring_size;
   cnt=save_image(motion_buffer.frames[most_recent_idx], HRES*VRES, &time_now);
   
   printf("save_framecnt=%d ", save_framecnt);


    if(save_framecnt > 0)
    {	
        clock_gettime(CLOCK_MONOTONIC, &time_now);
        fnow = (double)time_now.tv_sec + (double)time_now.tv_nsec / 1000000000.0;
                printf(" saved at %lf, @ %lf FPS\n", (fnow-fstart), (double)(process_framecnt+1) / (fnow-fstart));
    }
    else 
    {
        printf("at %lf\n", fnow-fstart);
    }

    return cnt;
}



/**
 * process_motion_detection - Analyzes frame differences for motion detection
 *
 * This function takes the current frame from scratchpad_buffer and:
 * 1. If it's the first frame, simply stores it in the motion buffer
 * 2. If there are existing frames, computes the absolute difference with the most recent frame
 * 3. Calculates the percentage difference and prints success if > 0.5%
 *
 * @param current_frame: Pointer to the current frame data (grayscale)
 * @param frame_size: Size of the frame in bytes (should be HRES*VRES for grayscale)
 *
 * @return: 0 on first frame, 1 if motion detected (diff > 0.5%), -1 if no significant motion
 */
int process_motion_detection(const unsigned char *current_frame, int frame_size)
{
    unsigned int diffsum = 0;
    double diff_percentage = 0.0;
    int i;
    int current_idx = motion_buffer.tail_idx;
    int prev_idx;
    unsigned int max_possible_diff;

    // If this is the first frame, just store it
    if (motion_buffer.count == 0)
    {
        // Copy current frame to motion buffer
        memcpy(motion_buffer.frames[current_idx], current_frame, frame_size);

        motion_buffer.tail_idx = (motion_buffer.tail_idx + 1) % motion_buffer.ring_size;
        motion_buffer.count++;

        printf("Motion detection: First frame stored (no comparison yet)\n");
        return 0;
    }

    // Get the previous frame index (most recent frame in buffer)
    prev_idx = (current_idx - 1 + motion_buffer.ring_size) % motion_buffer.ring_size;

    // Compute absolute difference sum between current and previous frame
    for (i = 0; i < frame_size; i++)
    {
        int diff = abs((int)current_frame[i] - (int)motion_buffer.frames[prev_idx][i]);
        diffsum += diff;
    }

    // Calculate percentage difference
    // Maximum possible difference = frame_size * 255 (every pixel differs by max amount)
    max_possible_diff = frame_size * 255;
    diff_percentage = (double)diffsum / (double)max_possible_diff * 100.0;

    printf("Motion detection: diffsum=%u, percentage=%.3f%%", diffsum, diff_percentage);

    // Check if difference exceeds threshold
    if (diff_percentage > 0.5)
    {
        printf(" - SUCCESS: Motion detected!\n");

        // Store current frame for next comparison
        memcpy(motion_buffer.frames[current_idx], current_frame, frame_size);
        motion_buffer.tail_idx = (motion_buffer.tail_idx + 1) % motion_buffer.ring_size;
        if (motion_buffer.count < motion_buffer.ring_size) {
            motion_buffer.count++;
        }

        return 1;
    }
    else
    {
        printf(" - No significant motion\n");

        // Store current frame for next comparison
        memcpy(motion_buffer.frames[current_idx], current_frame, frame_size);
        motion_buffer.tail_idx = (motion_buffer.tail_idx + 1) % motion_buffer.ring_size;
        if (motion_buffer.count < motion_buffer.ring_size) {
            motion_buffer.count++;
        }

        return -1;
    }
}


/**
 * reset_motion_buffer - Resets the motion detection buffer
 *
 * Call this function to clear the motion buffer and start fresh
 */
void reset_motion_buffer(void)
{
    motion_buffer.tail_idx = 0;
    motion_buffer.count = 0;
    memset(motion_buffer.frames, 0, sizeof(motion_buffer.frames));
    printf("Motion buffer reset\n");
}

