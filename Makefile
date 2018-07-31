#
# Makefile for kernel
# Original Makefile credit goes to wbcowan (from which this is based from)
#

XCC     = gcc
AS	= as
LD      = ld
HEADERPATH = ./include
CPUFLAGS = -mcpu=arm920t -msoft-float
CFLAGS  = -c -std=c99 -O2 -fPIC -Wall -I$(HEADERPATH) $(CPUFLAGS)
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings

OBJECTS = main.o bwio.o queue.o scheduler.o task_descriptor.o sys_call.o name_server.o string.o time.o ringbuffer.o event_handler.o clock_server.o
OBJECTS += clock_updater.o gui.o sensor_updater.o train_commands.o train_control_server.o user_init.o user_input_handler.o user_main.o io_server.o util.o
ASMFILES = ${OBJECTS:.o=.s}
DEPENDS = ${OBJECTS:.o=.d}
EXEC = kern.elf
MAPFILE = $(EXEC:.elf=.map)
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include
USR_SRC_DIR = user/src/
USR_INC_DIR = user/include/
INSTALL_DIR = /u3/cs452/tftp/ARM/g3szeto/k4/
BUILD_OB = $(addprefix $(BUILD_DIR)/,$(OBJECTS))

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs: always generate a complete stack frame

LDFLAGS = -init main -Map $(BUILD_DIR)/$(MAPFILE) -N  -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2

-include config.mk

.PHONY: all clean

.PRECIOUS: %.o %.s

all: $(EXEC)

# setup:
# 	mkdir -p $(BUILD_DIR)
$(BUILD_DIR):
	mkdir -p $@

$(INSTALL_DIR):
	mkdir -p $@

# just define one of these for each object for now, we can do something a little more scalable later
$(BUILD_DIR)/main.s: $(SRC_DIR)/main.c $(INCLUDE_DIR)/stddef.h $(INCLUDE_DIR)/bwio.h $(INCLUDE_DIR)/int_types.h $(INCLUDE_DIR)/internal/task_descriptor.h $(INCLUDE_DIR)/internal/scheduler.h $(INCLUDE_DIR)/ts7200.h | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $(SRC_DIR)/main.c -o $@

$(BUILD_DIR)/bwio.s: $(SRC_DIR)/bwio/bwio.c $(INCLUDE_DIR)/bwio.h | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $(SRC_DIR)/bwio/bwio.c -o $@

$(BUILD_DIR)/queue.s: $(SRC_DIR)/queue/queue.c $(INCLUDE_DIR)/internal/queue.h $(INCLUDE_DIR)/int_types.h
	$(XCC) -S $(CFLAGS) $(SRC_DIR)/queue/queue.c -o $@

$(BUILD_DIR)/scheduler.s: $(SRC_DIR)/scheduler/scheduler.c $(INCLUDE_DIR)/internal/scheduler.h $(INCLUDE_DIR)/int_types.h $(INCLUDE_DIR)/stddef.h $(INCLUDE_DIR)/bool.h
	$(XCC) -S $(CFLAGS) $(SRC_DIR)/scheduler/scheduler.c -o $@

$(BUILD_DIR)/task_descriptor.s: $(SRC_DIR)/task_descriptor/task_descriptor.c $(INCLUDE_DIR)/internal/task_descriptor.h $(INCLUDE_DIR)/int_types.h $(INCLUDE_DIR)/internal/queue.h $(INCLUDE_DIR)/internal/mem.h | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $(SRC_DIR)/task_descriptor/task_descriptor.c -o $@

$(BUILD_DIR)/user_tasks.s: $(SRC_DIR)/user_tasks.c $(INCLUDE_DIR)/int_types.h $(INCLUDE_DIR)/sys_call.h $(INCLUDE_DIR)/bool.h $(INCLUDE_DIR)/stddef.h $(INCLUDE_DIR)/bwio.h $(INCLUDE_DIR)/ringbuffer.h $(INCLUDE_DIR)/time.h $(INCLUDE_DIR)/name_server.h | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $(SRC_DIR)/user_tasks.c -o $@

$(BUILD_DIR)/string.s: $(SRC_DIR)/cstdlib/string.c $(INCLUDE_DIR)/int_types.h | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $< -o $@

$(BUILD_DIR)/time.s: $(SRC_DIR)/cstdlib/time.c $(INCLUDE_DIR)/time.h $(INCLUDE_DIR)/int_types.h | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $< -o $@

$(BUILD_DIR)/sys_call.s: $(SRC_DIR)/sys_call/sys_call.c $(INCLUDE_DIR)/sys_call.h | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $(SRC_DIR)/sys_call/sys_call.c -o $@

$(BUILD_DIR)/name_server.s: $(SRC_DIR)/name_server/name_server.c $(INCLUDE_DIR)/name_server.h $(INCLUDE_DIR)/internal/queue.h $(INCLUDE_DIR)/internal/task_descriptor.h $(INCLUDE_DIR)/bool.h $(INCLUDE_DIR)/sys_call.h $(INCLUDE_DIR)/bwio.h $(INCLUDE_DIR)/int_types.h $(INCLUDE_DIR)/string.h $(INCLUDE_DIR)/stddef.h | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $(SRC_DIR)/name_server/name_server.c -o $@

$(BUILD_DIR)/ringbuffer.s: $(SRC_DIR)/ringbuffer/ringbuffer.c $(INCLUDE_DIR)/ringbuffer.h $(INCLUDE_DIR)/int_types.h $(INCLUDE_DIR)/bool.h | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $(SRC_DIR)/ringbuffer/ringbuffer.c -o $@

$(BUILD_DIR)/event_handler.s: $(SRC_DIR)/event_handler/event_handler.c $(INCLUDE_DIR)/internal/event_handler.h $(INCLUDE_DIR)/internal/queue.h $(INCLUDE_DIR)/internal/task_descriptor.h | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $< -o $@

$(BUILD_DIR)/clock_server.s: $(SRC_DIR)/clock_server/clock_server.c $(INCLUDE_DIR)/clock_server.h $(INCLUDE_DIR)/int_types.h $(INCLUDE_DIR)/name_server.h $(INCLUDE_DIR)/sys_call.h $(INCLUDE_DIR)/bwio.h $(INCLUDE_DIR)/internal/queue.h $(INCLUDE_DIR)/internal/task_descriptor.h | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $(SRC_DIR)/clock_server/clock_server.c -o $@

$(BUILD_DIR)/clock_updater.s: $(USR_SRC_DIR)/clock_updater.c | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $< -o $@ -I $(USR_INC_DIR)

$(BUILD_DIR)/gui.s: $(USR_SRC_DIR)/gui.c | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $< -o $@ -I $(USR_INC_DIR)

$(BUILD_DIR)/sensor_updater.s: $(USR_SRC_DIR)/sensor_updater.c | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $< -o $@ -I $(USR_INC_DIR)

$(BUILD_DIR)/train_commands.s: $(USR_SRC_DIR)/train_commands.c | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $< -o $@ -I $(USR_INC_DIR)

$(BUILD_DIR)/train_control_server.s: $(USR_SRC_DIR)/train_control_server.c | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $< -o $@ -I $(USR_INC_DIR)

$(BUILD_DIR)/user_init.s: $(USR_SRC_DIR)/user_init.c | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $< -o $@ -I $(USR_INC_DIR)

$(BUILD_DIR)/user_input_handler.s: $(USR_SRC_DIR)/user_input_handler.c | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $< -o $@ -I $(USR_INC_DIR)

$(BUILD_DIR)/user_main.s: $(USR_SRC_DIR)/user_main.c | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $< -o $@ -I $(USR_INC_DIR)

$(BUILD_DIR)/util.s: $(USR_SRC_DIR)/util.c | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $< -o $@ -I $(USR_INC_DIR)

$(BUILD_DIR)/io_server.s: $(SRC_DIR)/io_server/io_server.c $(INCLUDE_DIR)/io_server.h $(INCLUDE_DIR)/int_types.h $(INCLUDE_DIR)/sys_call.h $(INCLUDE_DIR)/bwio.h $(INCLUDE_DIR)/ringbuffer.h $(INCLUDE_DIR)/name_server.h | $(BUILD_DIR)
	$(XCC) -S $(CFLAGS) $(SRC_DIR)/io_server/io_server.c -o $@

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $*.s

%.o: %.c

%: %.o

nolink: $(BUILD_OB:.o=.s)

$(EXEC): $(BUILD_OB)
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/$@ $^ -lgcc

install: $(EXEC) | $(INSTALL_DIR)
	cp $(BUILD_DIR)/$(EXEC) $(INSTALL_DIR)

clean:
	-rm -rf $(BUILD_DIR)/*
