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
    int cnt;

    printf("processing rb.tail=%d, rb.head=%d, rb.count=%d\n", ring_buffer.tail_idx, ring_buffer.head_idx, ring_buffer.count);

    ring_buffer.head_idx = (ring_buffer.head_idx + 2) % ring_buffer.ring_size;

    cnt=process_image((void *)&(ring_buffer.save_frame[ring_buffer.head_idx].frame[0]), HRES*VRES*PIXEL_SIZE);

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

