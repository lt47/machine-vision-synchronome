INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= headers/sequencer.h headers/capturelib.c
CFILES= source/sequencer.c source/capturelib.c

SRCS= ${HFILES} ${CFILES}
OBJS= $(patsubst source/%.c,object/%.o,$(CFILES))

all:	sequencer

clean:
	-rm -f object/*.o *.d frames/*.ppm
	-rm -f sequencer 

sequencer: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) -lpthread -lrt

object/%.o: source/%.c
	$(CC) $(CFLAGS) -c $< -o $@

depend:
