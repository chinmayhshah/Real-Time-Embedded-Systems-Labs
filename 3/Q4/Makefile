INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS= 
CFLAGS= -O3 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lpthread -lrt

PRODUCT=heap_mq posix_linux_demo posix_mq heap_mq_Posix_Linux

HFILES=
CFILES= heap_mq.c posix_linux_demo.c posix_mq.c heap_mq_Posix_Linux.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${PRODUCT} ${GARBAGE}

posix_linux_demo:	posix_linux_demo.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ posix_linux_demo.o $(LIBS)

heap_mq:	heap_mq.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ heap_mq.o $(LIBS)

posix_mq:	posix_mq.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ posix_mq.o $(LIBS)

heap_mq_Posix_Linux: heap_mq_Posix_Linux.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ heap_mq_Posix_Linux.o $(LIBS)
depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<
