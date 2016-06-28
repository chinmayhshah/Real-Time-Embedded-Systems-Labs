NCLUDE_DIRS = 
LIB_DIRS = 

CDEFS= 
CFLAGS= -O3 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lpthread -lrt

PRODUCT=binSem_Linux

HFILES=
CFILES=binSem_Linux.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${PRODUCT} ${GARBAGE}

binSem_Linux:	binSem_Linux.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ binSem_Linux.o $(LIBS)

depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<
