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

#define COURSE 4

struct buffer
{
        void   *start;
        size_t  length;
};


double fnow=0.0, fstart=0.0, fstop=0.0;
struct timespec time_now, time_start, time_stop;

static int read_frame(void)
{
    CLEAR(frame_buf);

    frame_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frame_buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(camera_device_fd, VIDIOC_DQBUF, &frame_buf))
    {
        switch (errno)
        {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, but drivers should only set for serious errors, although some set for
                   non-fatal errors too.
                 */
                return 0;


            default:
                printf("mmap failure\n");
                errno_exit("VIDIOC_DQBUF");
        }
    }

    read_framecnt++;

    //printf("frame %d ", read_framecnt);
    if(read_framecnt == 0) 
    {
        clock_gettime(CLOCK_MONOTONIC, &time_start);
        fstart = (double)time_start.tv_sec + (double)time_start.tv_nsec / 1000000000.0;
    }

    assert(frame_buf.index < n_buffers);

    return 1;
}


int seq_frame_read(void)
{
    fd_set fds;
    struct timeval tv;
    int rc;

    FD_ZERO(&fds);
    FD_SET(camera_device_fd, &fds);

    /* Timeout */
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    rc = select(camera_device_fd + 1, &fds, NULL, NULL, &tv);

    read_frame();

    // save off copy of image with time-stamp here
    //printf("memcpy to %p from %p for %d bytes\n", (void *)&(ring_buffer.save_frame[ring_buffer.tail_idx].frame[0]), buffers[frame_buf.index].start, frame_buf.bytesused);
    //syslog(LOG_CRIT, "memcpy to %p from %p for %d bytes\n", (void *)&(ring_buffer.save_frame[ring_buffer.tail_idx].frame[0]), buffers[frame_buf.index].start, frame_buf.bytesused);
    memcpy((void *)&(ring_buffer.save_frame[ring_buffer.tail_idx].frame[0]), buffers[frame_buf.index].start, frame_buf.bytesused);

    ring_buffer.tail_idx = (ring_buffer.tail_idx + 1) % ring_buffer.ring_size;
    ring_buffer.count++;

    clock_gettime(CLOCK_MONOTONIC, &time_now);
    fnow = (double)time_now.tv_sec + (double)time_now.tv_nsec / 1000000000.0;

    if(read_framecnt > 0)
    {	
        //printf("read_framecnt=%d, rb.tail=%d, rb.head=%d, rb.count=%d at %lf and %lf FPS", read_framecnt, ring_buffer.tail_idx, ring_buffer.head_idx, ring_buffer.count, (fnow-fstart), (double)(read_framecnt) / (fnow-fstart));

        //syslog(LOG_CRIT, "read_framecnt=%d, rb.tail=%d, rb.head=%d, rb.count=%d at %lf and %lf FPS", read_framecnt, ring_buffer.tail_idx, ring_buffer.head_idx, ring_buffer.count, (fnow-fstart), (double)(read_framecnt) / (fnow-fstart));
        //syslog(LOG_CRIT, "read_framecnt=%d at %lf and %lf FPS", read_framecnt, (fnow-fstart), (double)(read_framecnt) / (fnow-fstart));

    }
    else 
    {
       // printf("at %lf\n", fnow);
    }

    if (-1 == xioctl(camera_device_fd, VIDIOC_QBUF, &frame_buf))
        errno_exit("VIDIOC_QBUF");
}

