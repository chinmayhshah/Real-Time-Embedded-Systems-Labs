INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS= 
CFLAGS= -O3 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lpthread -lrt

PRODUCT=Posix_mq_Linux heap_mq_Posix_Linux

HFILES=
CFILES= Posix_mq_Linux.c heap_mq_Posix_Linux.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${PRODUCT} ${GARBAGE}

Posix_mq_Linux:	Posix_mq_Linux.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ Posix_mq_Linux.o $(LIBS)

heap_mq_Posix_Linux: heap_mq_Posix_Linux.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ heap_mq_Posix_Linux.o $(LIBS)
depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<
