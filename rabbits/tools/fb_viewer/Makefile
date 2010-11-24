CC         = gcc
LD         = gcc

CPPFLAGS   = #-DDEBUG_CONV -DDEBUG_FBVIEWER
CFLAGS     = -Wall -Wextra -std=c99 -g -O3
LDFLAGS    =

CFILES     = fb_viewer.c conv.c

SRCDIR     = src
OBJDIR     = obj
DEPDIR     = .deps
PROG       = fbviewer

# SDL
CFLAGS    += `pkg-config --cflags sdl`
LDFLAGS   += `pkg-config --libs sdl` 

OBJECTS    = $(addprefix $(OBJDIR)/, $(notdir $(CFILES:.c=.o)))
DEP        = $(addprefix $(OBJDIR)/, $(DEPDIR))
DEPFILES   = $(addprefix $(DEP)/, $(notdir $(CFILES:%.c=%.d)))

ifndef $(NO_DEP)
DEP_CFLAGS = -MP -MMD -MF $(DEP)/$*.d
CFLAGS    += $(DEP_CFLAGS)
endif

quiet-command = $(if $(VERB),$1,$(if $(2),@echo $2 && $1, @$1))

.PHONY  : clean distclean

.SUFFIXES: .c .o .x

all: $(OBJDIR) $(DEP) $(PROG)
	$(call quiet-command, cp $(PROG) ../, "INSTALL")

$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(call quiet-command, $(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<, "  CC     $<")

ifndef $(NO_DEP)
-include $(DEPFILES)
endif

$(PROG) : $(OBJECTS)
	$(call quiet-command, $(LD) $^ $(LDFLAGS) -o $@, "  LD     $@")

$(OBJDIR):
	$(call quiet-command, mkdir -p $(OBJDIR),)

$(DEP):
	@mkdir -p $(DEP)

link    : binclean $(PROG)

clean    :
	$(call quiet-command, rm -f $(OBJECTS) $(PROG) $(SRCDIR)/*~ *~, "  CLEAN    ")

distclean: clean
	$(call quiet-command, rm -fr $(DEP) $(OBJDIR), "  CLEAN    deps")

binclean :
	$(call quiet-command, rm -f $(PROG), "  BINCLEAN ")
