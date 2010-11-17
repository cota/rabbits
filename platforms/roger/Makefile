HW           = $(RABBITS_DIR)
SYSTEMC      = $(RABBITS_SYSTEMC)
QEMU         = $(HW)/rabbits/libs
COMPONENTS   = rabbits/components
PLATFORMS    = platforms
SRC_BASE_DIR = $(HW)
SUB_DIRS     = $(COMPONENTS)/abstract_noc       \
               $(COMPONENTS)/mem                \
               $(COMPONENTS)/qemu_wrapper       \
               $(COMPONENTS)/sem                \
               $(COMPONENTS)/slave_device       \
               $(COMPONENTS)/master_device      \
               $(COMPONENTS)/framebuffer        \
               $(COMPONENTS)/tg                 \
               $(COMPONENTS)/timer              \
               $(COMPONENTS)/tty_serial         \
               $(COMPONENTS)/etrace             \
               $(PLATFORMS)/roger

CC           = g++
OPT          = -O0 -fpermissive
DEBUG        = -g
OTHER        =  -Wno-deprecated
CFLAGS       = $(DEBUG) $(OTHER) $(OPT)
LIBDIR       = -L$(SYSTEMC)/lib-$(TARGET_ARCH)
LIBS         = -lsystemc -ldl
RLIBDIR      = -Wl,-rpath $(QEMU)
TARGET_ARCH  = linux

INCDIR += -I$(SYSTEMC)/include
INCDIR += $(foreach SUB_DIR, $(SUB_DIRS), $(addprefix -I$(SRC_BASE_DIR)/, $(SUB_DIR)))

MODULE       = run
EXE          = $(MODULE).x

BUILD_DIR    = ./objs
define FIND_SRCS
$(addprefix $(1)/, $(notdir $(wildcard $(SRC_BASE_DIR)/$(1)/*.cpp)))
endef
TMPSRCS=$(foreach SUB_DIR, $(SUB_DIRS), $(call FIND_SRCS,$(SUB_DIR)))
SRCS=$(addprefix $(SRC_BASE_DIR)/, $(TMPSRCS))
OBJS=$(addprefix $(BUILD_DIR)/, $(TMPSRCS:.cpp=.o))
DEPS=$(OBJS:.o=.d)

$(shell mkdir -p $(addprefix $(BUILD_DIR)/, $(SUB_DIRS)))

.SUFFIXES: .cc .cpp .o .x
.PHONY: clean all graphs verif

all: $(EXE)

$(EXE): $(OBJS)
	@echo Building $@ ...
	@$(CC) $(CFLAGS) $(INCDIR) $(LIBDIR) $(RLIBDIR) -o $@ $(OBJS) $(LIBS) 2>&1 | c++filt

$(BUILD_DIR)/%.o:$(SRC_BASE_DIR)/%.cpp
	@echo "Compiling $(notdir $<) ..."
	@rm -fr $(addsuffix .d, $(basename $@))
	@$(CC) $(CFLAGS) $(INCDIR) -o $@ -c $< -fvisibility=hidden 
	@echo -n "$(dir $@)" > $(addsuffix .d, $(basename $@))
	@$(CC) $(CFLAGS) $(INCDIR) -M $< >> $(addsuffix .d, $(basename $@))

clean:
	@echo Cleaning ...
	@rm -f $(EXE)
	@rm -f $(addsuffix /*.o, $(addprefix $(BUILD_DIR)/, $(SUB_DIRS)))
	@rm -f $(addsuffix /*.d, $(addprefix $(BUILD_DIR)/, $(SUB_DIRS)))

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
