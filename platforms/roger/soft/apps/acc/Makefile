JPG := /home/cota/src/jpeg_acc/jpeg0

INC = -I. -I$(JPG)/src -I$(JPG)/tb
LIB =

CFLAGS = $(INC)

CC := $(CROSS_COMPILE)gcc
CPP := $(CROSS_COMPILE)g++

OBJS := code0 jpeg

all: $(OBJS)

code0: code0.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

jpeg: jpeg.cpp
	$(CPP) $(CFLAGS) -o $@ $^ $(LIB)

clean :
	$(RM) $(OBJS)
