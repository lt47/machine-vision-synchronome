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
#include "../headers/process_frame.h"



struct motion_buffer_t motion_buffer = {
    .ring_size = 2,
    .tail_idx = 0,
    .count = 0
};

unsigned char scratchpad_buffer[MAX_HRES*MAX_VRES*MAX_PIXEL_SIZE];
int process_framecnt=0;


void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b)
{
   int r1, g1, b1;

   // replaces floating point coefficients
   int c = y-16, d = u - 128, e = v - 128;

   // Conversion that avoids floating point
   r1 = (298 * c           + 409 * e + 128) >> 8;
   g1 = (298 * c - 100 * d - 208 * e + 128) >> 8;
   b1 = (298 * c + 516 * d           + 128) >> 8;

   // Computed values may need clipping.
   if (r1 > 255) r1 = 255;
   if (g1 > 255) g1 = 255;
   if (b1 > 255) b1 = 255;

   if (r1 < 0) r1 = 0;
   if (g1 < 0) g1 = 0;
   if (b1 < 0) b1 = 0;

   *r = r1 ;
   *g = g1 ;
   *b = b1 ;
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
    if (diff_percentage > 0.4)
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

static int process_image(const void *p, int size)
{
    int i, newi, newsize=0;
    int y_temp, y2_temp, u_temp, v_temp;
    unsigned char *frame_ptr = (unsigned char *)p;

    process_framecnt++;
    printf("process frame %d: ", process_framecnt);
    
    if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY)
    {
        printf("NO PROCESSING for graymap as-is size %d\n", size);
    }

    else if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
    {
#if defined(COLOR_CONVERT_RGB)
       
        // Pixels are YU and YV alternating, so YUYV which is 4 bytes
        // We want RGB, so RGBRGB which is 6 bytes
        //
        for(i=0, newi=0; i<size; i=i+4, newi=newi+6)
        {
            y_temp=(int)frame_ptr[i]; u_temp=(int)frame_ptr[i+1]; y2_temp=(int)frame_ptr[i+2]; v_temp=(int)frame_ptr[i+3];
            yuv2rgb(y_temp, u_temp, v_temp, &scratchpad_buffer[newi], &scratchpad_buffer[newi+1], &scratchpad_buffer[newi+2]);
            yuv2rgb(y2_temp, u_temp, v_temp, &scratchpad_buffer[newi+3], &scratchpad_buffer[newi+4], &scratchpad_buffer[newi+5]);
        }
#elif defined(COLOR_CONVERT_GRAY)
        // Pixels are YU and YV alternating, so YUYV which is 4 bytes
        // We want Y, so YY which is 2 bytes
        //
        for(i=0, newi=0; i<size; i=i+4, newi=newi+2)
        {
            // Y1=first byte and Y2=third byte
            scratchpad_buffer[newi]=frame_ptr[i];
            scratchpad_buffer[newi+1]=frame_ptr[i+2];
        }

        process_motion_detection(scratchpad_buffer, size/2);
#endif
    }

    else if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24)
    {
        printf("NO PROCESSING for RGB as-is size %d\n", size);
    }
    else
    {
        printf("NO PROCESSING ERROR - unknown format\n");
    }

    return process_framecnt;
}




int seq_frame_process(void)
{
    int cnt = 0;

    printf("processing rb.tail=%d, rb.head=%d, rb.count=%d\n", ring_buffer.tail_idx, ring_buffer.head_idx, ring_buffer.count);

    ring_buffer.head_idx = (ring_buffer.head_idx + 2) % ring_buffer.ring_size;

    if (read_framecnt > 0){
    	cnt=process_image((void *)&(ring_buffer.save_frame[ring_buffer.head_idx].frame[0]), HRES*VRES*PIXEL_SIZE);
    }

    ring_buffer.head_idx = (ring_buffer.head_idx + 3) % ring_buffer.ring_size;
    ring_buffer.count = ring_buffer.count - 5;

     	
    printf("rb.tail=%d, rb.head=%d, rb.count=%d ", ring_buffer.tail_idx, ring_buffer.head_idx, ring_buffer.count);
       
    if(process_framecnt > 0)
    {	
        clock_gettime(CLOCK_MONOTONIC, &time_now);
        fnow = (double)time_now.tv_sec + (double)time_now.tv_nsec / 1000000000.0;
                printf(" processed at %lf, @ %lf FPS\n", (fnow-fstart), (double)(process_framecnt+1) / (fnow-fstart));
    }
    else 
    {
        printf("at %lf\n", fnow-fstart);
    }

    return cnt;
}



