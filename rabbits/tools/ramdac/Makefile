NAME=   xramdac

CC=	gcc
OPT= -O3
DEBUG= -g
WARN=	-Wall
CFLAGS=	$(OPT) $(DEBUG) $(WARN) `pkg-config --cflags gtk+-2.0` 

SRCS=   $(wildcard ./src/*.c) 
OBJS=   $(SRCS:.c=.o)
BUILDDIR= ./build

INCDIR = -I./include	`pkg-config --cflags gtk+-2.0` 

LD = gcc
LIBDIR =
LIBS = -lpthread
LDFLAGS = `pkg-config --cflags --libs gtk+-2.0` 

.SUFFIXES: .c .o .x

all: $(BUILDDIR) $(NAME)

$(NAME): $(OBJS)
	@$(LD)  $(LDFLAGS)  $(LIBDIR) -o $@ $(OBJS) $(LIBS)
	@echo "--------| $@"
	@cp $@ ../$@

.c.o:
	@$(CC) $(CFLAGS) $(INCDIR) -o $@ -c $<

ultraclean: clean
	@rm -f deps.mak

clean:
	@rm -f $(OBJS)
	@rm -fr $(BUILDDIR)

install : $(BUILDDIR)/$(NAME)
	@echo "[Copying $< ]" 
	@$(INSTALL_PROGRAM) $< $(prefix)/bin/

deps.mak:
	@echo 'Making dependencies ...'
	@$(CC) $(CFLAGS) $(INCDIR) -M $(SRCS) >> deps.mak

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

-include deps.mak
