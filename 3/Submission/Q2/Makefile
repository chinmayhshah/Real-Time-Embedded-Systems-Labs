INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -O -g $(INCLUDE_DIRS) $(CDEFS) -DLINUX
LIBS=-lpthread -lrt

HFILES=
CFILES1= thread_safe.c 

SRCS1= ${HFILES} ${CFILES1}
OBJS1= ${CFILES1:.c=.o}

all: thread_safe

clean:
	-rm -f *.o *.d *.exe thread_safe

thread_safe: thread_safe.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS1) $(LIBS)
	
depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<
