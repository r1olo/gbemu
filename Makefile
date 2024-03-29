CC = cc
CFLAGS = -Wall -Werror -O3
PROGNAME = gbemu
OBJS = main.o \
	   alloc.o \
	   cart.o \
	   dma.o \
	   cpu.o \
	   gb.o \
	   input.o \
	   ppu.o \
	   mem.o \
	   serial.o \
	   tim.o \
	   utils.o

${PROGNAME}: ${OBJS}
	${CC} -o ${PROGNAME} ${OBJS}

clean:
	rm -f ${OBJS} ${PROGNAME}

.PHONY: clean
