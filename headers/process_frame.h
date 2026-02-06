#ifndef _PROCESS_FRAME_
#define _PROCESS_FRAME_


struct motion_buffer_t
{
    unsigned int ring_size;
    int tail_idx;
    int count;
    unsigned char frames[2][HRES*VRES];  // Store up to 2 grayscale frames for comparison
};

extern struct motion_buffer_t motion_buffer;


extern unsigned char scratchpad_buffer[];


void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b);
static int process_image(const void *p, int size);
int seq_frame_process(void);


//Placeholder until I get all the functions declared in the header, because the ordering here is messed up. 
int process_motion_detection(const unsigned char *current_frame, int frame_size);
void reset_motion_buffer(void);

extern int process_framecnt;
#endif
