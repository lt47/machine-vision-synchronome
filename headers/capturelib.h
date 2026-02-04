#ifndef _CAPTURELIB_
#define _CAPTURELIB_


#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define MAX_HRES (1920)
#define MAX_VRES (1080)
#define MAX_PIXEL_SIZE (3)

#define HRES (640)
#define VRES (480)
#define PIXEL_SIZE (2)
#define HRES_STR "640"
#define VRES_STR "480"


#define STARTUP_FRAMES (30)
#define LAST_FRAMES (1)
#define CAPTURE_FRAMES (300+LAST_FRAMES)
#define FRAMES_TO_ACQUIRE (CAPTURE_FRAMES + STARTUP_FRAMES + LAST_FRAMES)

#define FRAMES_PER_SEC (1) 

//#define COLOR_CONVERT_RGB
#define COLOR_CONVERT_GRAY

#define DUMP_FRAMES

#define DRIVER_MMAP_BUFFERS (6)  // request buffers for delay


// Format is used by a number of functions, so made as a file global
static struct v4l2_format fmt;
struct v4l2_buffer frame_buf;



#endif
