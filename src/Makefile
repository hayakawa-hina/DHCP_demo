CPROG=		mydhcpc
DPROG=		mydhcpd
COBJS=		mydhcpc.o func.o
DOBJS=		mydhcpd.o func.o
CC=			gcc

PROGS=		$(CPROG) $(DPROG)

CFLAGS=	-O -DDEBUG
#CFLAGS=	-O -g -Wall -Werror

all: $(CPROG) $(DPROG)

$(CPROG): $(COBJS) $(LIBS)
	$(CC) $(CFLAGS) -o $(CPROG) $(COBJS)

$(DPROG): $(DOBJS) $(LIBS)
	$(CC) $(CFLAGS) -o $(DPROG) $(DOBJS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $*.c

mydhcpc.o: def.h
mydhcpd.o: def.h
func.o: def.h

clean:
	rm -f $(PROGS) *.o core *.core *.bak *_r *~
