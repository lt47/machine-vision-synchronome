#ifndef _PROCESS_FRAME_
#define _PROCESS_FRAME_

void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b);
static int process_image(const void *p, int size);
int seq_frame_process(void);


extern int process_framecnt;
#endif
