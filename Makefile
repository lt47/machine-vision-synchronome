INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= headers/sequencer.h headers/capturelib.h headers/v4l2_interface.h
CFILES= source/sequencer.c source/v4l2_interface.c source/capturelib.c source/read_frames.c source/process_frame.c

SRCS= ${HFILES} ${CFILES}
OBJS= $(patsubst source/%.c,object/%.o,$(CFILES))

all:	sequencer

clean:
	-rm -f object/*.o *.d frames/*.ppm frames/*.pgm
	-rm -f sequencer 

sequencer: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) -lpthread -lrt

object/%.o: source/%.c
	$(CC) $(CFLAGS) -c $< -o $@

depend:
