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


#define LAST_FRAMES (1)
#define CAPTURE_FRAMES (300+LAST_FRAMES)
#define FRAMES_TO_ACQUIRE (CAPTURE_FRAMES + STARTUP_FRAMES + LAST_FRAMES)

//#define FRAMES_PER_SEC (1) 

//#define COLOR_CONVERT_RGB
#define COLOR_CONVERT_GRAY

#define DUMP_FRAMES



struct timespec;
extern int camera_device_fd;
extern unsigned int n_buffers;
extern double fnow, fstart, fstop;
extern struct timespec time_now, time_start, time_stop;

#endif
