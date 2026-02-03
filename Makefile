INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= headers/sequencer.h headers/capturelib.c
CFILES= source/sequencer.c source/capturelib.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	sequencer

clean:
	-rm -f *.o *.d frames/*.ppm
	-rm -f sequencer 

sequencer: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o capturelib.o -lpthread -lrt

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<

