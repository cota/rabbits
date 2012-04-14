JPG := /home/cota/src/jpeg_acc/jpeg0

INC = -I. -I$(JPG)/src -I$(JPG)/tb
LIB =

CFLAGS = $(INC)
CPPFLAGS = $(CFLAGS)

CC := $(CROSS_COMPILE)gcc
CPP := $(CROSS_COMPILE)g++
CXX := $(CROSS_COMPILE)g++

OBJS := code0 jpeg n_cycles

all: $(OBJS)

code0: code0.o comcas.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

jpeg: jpeg.o comcas.o
	$(CPP) $(CFLAGS) -o $@ $^ $(LIB)

n_cycles: n_cycles.c comcas.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

clean :
	$(RM) $(OBJS) *.o
