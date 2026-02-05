#ifndef _V4LT_INTERFACE_
#define _V4LT_INTERFACE_


#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define MAX_HRES (1920)
#define MAX_VRES (1080)
#define MAX_PIXEL_SIZE (3)

#define HRES (640)
#define VRES (480)
#define PIXEL_SIZE (2)
#define HRES_STR "640"
#define VRES_STR "480"

#define FRAMES_PER_SEC (1) 
#define STARTUP_FRAMES (30)

#define DRIVER_MMAP_BUFFERS (6)  // request buffers for delay

//Variables

// Format is used by a number of functions, so made as a file global

struct v4l2_format;
struct v4l2_buffer;
struct buffer;
extern struct v4l2_format fmt;
extern struct v4l2_buffer frame_buf;

extern int read_framecnt;

extern struct buffer          *buffers;

void errno_exit(const char *s);

int xioctl(int fh, int request, void *arg);

#endif
