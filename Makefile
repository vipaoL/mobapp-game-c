# make PLATFORM=desktop to build with SDL2
PLATFORM ?= fp

NAME := app
BUILDDIR := build
OBJDIR := $(BUILDDIR)/obj/$(PLATFORM)

##########
# FP
ifeq ($(PLATFORM), fp)

# 0 - detect, 1 - SC6531E, 2 - SC6531DA, 3 - SC6530
CHIP        := 0
TWO_STAGE   := 0
LIBC_SDIO   := 0
FPDOOM      := fpdoom
PACK_RELOC  := $(FPDOOM)/pack_reloc/pack_reloc

APP_SRCS_BASE         := $(notdir $(patsubst %.c,%,$(wildcard src/*.c)))
BOX2D_SRCS_BASE       := $(notdir $(patsubst %.c,%,$(wildcard box2d/src/*.c)))
FP_COMPAT_SRCS_BASE   := $(notdir $(patsubst %.c,%,$(wildcard fpcompat/*.c)))
FP_FRAMEWORK_SRCS_BASE:= asmcode usbio common libc syscomm syscode

SRCS := $(APP_SRCS_BASE) $(BOX2D_SRCS_BASE) $(FP_COMPAT_SRCS_BASE) $(FP_FRAMEWORK_SRCS_BASE)

ifneq ($(LIBC_SDIO), 0)
SRCS += microfat
endif

ifeq ($(TWO_STAGE), 0)
SRCS := start entry $(SRCS)
OBJS := $(SRCS:%=$(OBJDIR)/%.o)
else
SRCS1 := start1 entry $(SRCS)
SRCS2 := start2 entry2 $(SRCS)
OBJS1 := $(SRCS1:%=$(OBJDIR)/%.o)
OBJS2 := $(SRCS2:%=$(OBJDIR)/%.o)
OBJS  := $(OBJS1) $(OBJS2)
endif

TOOLCHAIN ?= arm-none-eabi
CC        := $(TOOLCHAIN)-gcc
OBJCOPY   := $(TOOLCHAIN)-objcopy

CFLAGS := -Wall -Wextra -funsigned-char -std=c99 -pedantic
CFLAGS += -fno-PIE -ffreestanding -march=armv5te $(EXTRA_CFLAGS) -fno-strict-aliasing
CFLAGS += -fomit-frame-pointer
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0

CFLAGS += -isystem $(FPDOOM)/fpdoom/include
CFLAGS += -I$(FPDOOM)/fpdoom
CFLAGS += -Isrc
CFLAGS += -Ibox2d/include
CFLAGS += -Ifpcompat

##### Include compat.h globally
# This is required to patch the environment for Box2D to compile without modification
# (e.g., by revealing math functions that are hidden by fpdoom/fpdoom/include/math.h).
CFLAGS += -include compat.h
#####

# armv5te doesn't support it
CFLAGS += -DBOX2D_DISABLE_SIMD=1

COMPILER = $(findstring clang,$(notdir $(CC)))
ifeq ($(COMPILER), clang)
# Clang
CFLAGS += -Oz
CXX = "$(CC)++"
else
# GCC
CFLAGS += -Os
endif

ifeq ($(COMPILER), clang)
# Clang's bug workaround
CFLAGS += -mcpu=+nodsp
endif

LTO = 0
ifneq ($(LTO), 0)
# Clang's LTO doesn't work with the GCC toolchain
ifeq ($(findstring -gcc-toolchain,$(notdir $(CC))),)
CFLAGS += -flto
endif
endif

ifdef SYSROOT
CFLAGS += --sysroot="$(SYSROOT)"
endif

ifneq ($(CHIP), 0)
CFLAGS += -DCHIP=$(CHIP)
endif
CFLAGS += -DTWO_STAGE=$(TWO_STAGE)
ifneq ($(LIBC_SDIO), 0)
CFLAGS += -DLIBC_SDIO=$(LIBC_SDIO) -DFAT_WRITE=1
#CFLAGS += -DFAT_DEBUG=1
#CFLAGS += -DCHIPRAM_ARGS=1
endif

LDSCRIPT := $(FPDOOM)/fpdoom/sc6531e_fdl.ld
LFLAGS := -pie -nostartfiles -Wl,-T,$(LDSCRIPT) -Wl,--gc-sections -Wl,-z,notext -nostdlib -specs=sync-dmb.specs $(LD_EXTRA)

ifeq ($(CHIP), 2)
LFLAGS += -Wl,--defsym,IMAGE_START=0x34000000
else
LFLAGS += -Wl,--defsym,IMAGE_START=0x14000000
endif

VPATH := src:box2d/src:fpcompat:fpdoom/fpdoom

TARGET_BIN := $(BUILDDIR)/$(NAME).bin

endif
##########

########
# DESKTOP
ifeq ($(PLATFORM), desktop)

APP_SRCS_BASE         := $(notdir $(patsubst %.c,%,$(wildcard src/*.c)))
BOX2D_SRCS_BASE       := $(notdir $(patsubst %.c,%,$(wildcard box2d/src/*.c)))
DESKTOP_COMPAT_SRCS_BASE := $(notdir $(patsubst %.c,%,$(wildcard desktopcompat/*.c)))

SRCS := $(APP_SRCS_BASE) $(BOX2D_SRCS_BASE) $(DESKTOP_COMPAT_SRCS_BASE)
OBJS := $(SRCS:%=$(OBJDIR)/%.o)

CC     := gcc
CFLAGS := -g -O2 -Wall -Wextra -std=c99 -pedantic
CFLAGS += -D_DEFAULT_SOURCE
CFLAGS += -Isrc -Ibox2d/include -Idesktopcompat
CFLAGS += $(shell sdl2-config --cflags)
LFLAGS += $(shell sdl2-config --libs) -lSDL2_gfx

VPATH := src:box2d/src:desktopcompat

TARGET_BIN := $(BUILDDIR)/$(NAME)

endif
########

#####
# targets
.PHONY: all clean

all: $(TARGET_BIN)

clean:
	$(RM) -r $(BUILDDIR)
#####

-include $(OBJS:.o=.d)

$(OBJDIR)/%.o: %.c
	mkdir -p $(@D)
	@echo "Compiling [C] $< -> $@"
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJDIR)/%.o: %.s
	mkdir -p $(@D)
	@echo "Assembling [ASM] $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@


##
# DESKTOP linking
ifeq ($(PLATFORM), desktop)
$(TARGET_BIN): $(OBJS)
	mkdir -p $(@D)
	@echo "Linking $@..."
	$(CC) $(OBJS) -o $@ $(LFLAGS)
endif
##

##
# FP linking
ifeq ($(PLATFORM), fp)

%.rel: %.elf
	$(PACK_RELOC) $< $@

ifeq ($(TWO_STAGE), 0)
$(OBJDIR)/$(NAME).elf: $(OBJS)
	@echo "Linking $@..."
	$(CC) $(LFLAGS) $(OBJS) -Wl,--start-group -lm -lgcc -Wl,--end-group -o $@

$(TARGET_BIN): $(OBJDIR)/$(NAME).elf
	mkdir -p $(@D)
	@echo "Creating final binary $@"
	$(OBJCOPY) -O binary -j .text $< $(OBJDIR)/text.bin
	$(PACK_RELOC) $< $(OBJDIR)/reloc.bin
	cat $(OBJDIR)/text.bin $(OBJDIR)/reloc.bin > $@
	$(RM) $(OBJDIR)/text.bin $(OBJDIR)/reloc.bin
else

$(OBJDIR)/$(NAME)_part1.elf: $(OBJS1)
	$(CC) $(LFLAGS) $(OBJS1) -Wl,--start-group -lm -lgcc -Wl,--end-group -o $@
$(OBJDIR)/$(NAME)_part2.elf: $(OBJS2)
	$(CC) $(LFLAGS) $(OBJS2) -Wl,--start-group -lm -lgcc -Wl,--end-group -o $@
%.bin: %.elf
	$(OBJCOPY) -O binary -j .text $< $@
$(TARGET_BIN): $(patsubst %,$(OBJDIR)/$(NAME)_part%,2.bin 1.bin 1.rel 2.rel)
	mkdir -p $(@D)
	cat $^ > $@
endif

endif
##
