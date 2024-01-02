CC = cc
CFLAGS = -Wall -Werror -O2
PROGNAME = gbemu
OBJS = main.o \
	   cart.o \
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
