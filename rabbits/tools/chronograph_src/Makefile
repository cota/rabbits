CC     = g++
OPT    = -O2 -fpermissive
DEBUG  = -g
OTHER  =  -Wno-deprecated 

GTK = $(shell pkg-config --list-all|grep gtk+|head -1|cut -d ' ' -f1)
GTHREAD = $(shell pkg-config --list-all|grep gthread-|head -1|cut -d ' ' -f1)
GTK_CFLAGS = $(shell pkg-config $(GTK) $(GTHREAD) --cflags)
GTK_LIBS = $(shell pkg-config $(GTK) $(GTHREAD) --libs)

EXE    = chronograph
SRCS = $(EXE).cpp
OBJS = $(SRCS:.cpp=.o)
CFLAGS = $(DEBUG) $(OTHER) $(OPT) $(GTK_CFLAGS)
LIBDIR =
INCDIR =
LIBS   = $(GTK_LIBS) -lpthread
PREFIX =../

.SUFFIXES: .cpp .o
.PHONY: clean all

all: $(EXE)

$(EXE): $(OBJS)
	@echo Building $@ ...
	@$(CC) $(LIBDIR) $(OBJS) $(LIBS) -o $(PREFIX)/$@ 2>&1 | c++filt

.cpp.o:
	@echo Compiling $< ...
	@$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@

deps.mak:
	@echo Making deps ....
	@$(CC) $(CFLAGS) $(INCDIR) -MM $(SRCS) >> deps.mak

-include deps.mak

clean:
	@echo Cleaning ...
	@rm -f $(OBJS) *~ $(PREFIX)/$(EXE) deps.mak
