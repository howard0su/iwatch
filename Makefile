ifeq ($(MSPPATH),)
MSPPATH = /home/junsu/mspgccx/bin
endif
CC      = $(MSPPATH)/msp430-gcc
OBJCOPY = $(MSPPATH)/msp430-objcopy
SIZE    = $(MSPPATH)/msp430-size
NM		= $(MSPPATH)/msp430-nm
MAKETXT = srec_cat
ECHO	= echo

TARGET_CPU = msp430f5438a
MEMORY_MODEL = medium
WARNING_FLAGS = -W -Wall -Wno-narrowing -Wwrite-strings -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wmissing-format-attribute -pedantic -Wno-long-long -Wno-variadic-macros -Wno-overlength-strings -Wold-style-definition -Wc++-compat
CFLAGS0  = -mmcu=$(TARGET_CPU) -g -std=c99 -Os $(WARNING_FLAGS) -mmemory-model=$(MEMORY_MODEL) \
	-ffunction-sections -fdata-sections
LDFLAGS = -mmcu=$(TARGET_CPU) -g -std=c99 -Os $(WARNING_FLAGS) -Wl,-gc-sections -mmemory-model=$(MEMORY_MODEL)


CFLAGS = $(CFLAGS0) -flto
LDFLAGS += -flto

ALL_DEFINES = AUTOSTART_ENABLE=1 HAVE_BLE=1
ALL_INCLUDEDIRS = \
	. \
	core \
	core/lib \
	cpu/msp430 \
	ant \
	platform/iwatch \
	platform/iwatch/btstack \
	btstack/src \
	btstack/ble \
	btstack/chipset-cc256x \
	btstack/include \

#######################################
# source files
CORE   = \
    core/sys/autostart.c \
    core/sys/ctimer.c \
    core/sys/etimer.c \
    core/sys/energest.c \
    core/sys/process.c \
    core/sys/rtimer.c \
    core/sys/stimer.c \
    core/sys/timer.c \
    core/lib/assert.c \
    core/lib/list.c

PLATFORM = \
	platform/iwatch/backlight.c \
	platform/iwatch/battery.c \
	platform/iwatch/button-sensor.c \
	platform/iwatch/clock.c \
	platform/iwatch/flash.c \
	platform/iwatch/i2c.c \
	platform/iwatch/isr.c \
	platform/iwatch/msp430.c \
	platform/iwatch/rtc.c \
	platform/iwatch/rtimer-arch.c \
	platform/iwatch/uart1-putchar.c \
	platform/iwatch/uart1x.c \
	platform/iwatch/watchdog.c \
	platform/iwatch/Template_Driver.c

ANT = \
	ant/antinterface.c \
	ant/cbsc_rx.c \
	ant/hrm_rx.c \
	platform/iwatch/ant/main_hrm.c \
	platform/iwatch/ant/serial.c \
	platform/iwatch/ant/ant_timer.c

GRLIB0 = \
	circle.c \
	context.c \
	image.c \
	line.c \
	rectangle.c \
	string.c

GRLIB_FONTS = \
	fontnova10.c \
	fontnova9b.c \
	fontnova12.c \
	fontnova12b.c \
	fontnova28.c \
	fontnova28b.c \
	fontnova38b.c \
	fonticon16.c
GRLIB = $(addprefix grlib/, $(GRLIB0)) $(addprefix grlib/fonts/, $(GRLIB_FONTS))

BTSTACK0 = \
	bluetooth.c \
	codec.c \
	deviceid.c \
	hal.c \
	hal_compat.c \
	hal_uart_dma.c \
	run_loop.c \
	sdp_client.c \
	spp.c
BTSTACK1 = \
	ble/att.c \
	chipset-cc256x/bluetooth_init_cc2564_2.5.c \
	chipset-cc256x/bt_control_cc256x.c \
	src/avctp.c \
	src/avrcp.c \
	src/btstack_memory.c \
	src/hci.c \
	src/hci_cmds.c \
	src/hci_dump.c \
	src/hci_transport_h4_ehcill_dma.c \
	src/hfp.c \
	src/l2cap.c \
	src/l2cap_signaling.c \
	src/linked_list.c \
	src/memory_pool.c \
	src/mns.c \
	src/remote_device_db_memory.c \
	src/rfcomm.c \
	src/sdp.c \
	src/sdp_util.c \
	src/utils.c

BTSTACK = $(addprefix platform/iwatch/btstack/, $(BTSTACK0)) $(addprefix btstack/, $(BTSTACK1))

MPL0 = inv_mpu.c inv_mpu_dmp_motion_driver.c mpu6050.c
MPL = $(addprefix platform/iwatch/mpl/, $(MPL0))

WATCH = \
    watch/analog-watch.c \
    watch/btconfig.c \
    watch/configtime.c \
    watch/countdown.c \
    watch/controller.c \
    watch/controls.c \
    watch/digit-watch.c \
    watch/stopwatch.c \
    watch/menu.c \
    watch/watch.c \
    watch/cordic.c \
    watch/calendar.c \
    watch/selftest.c \
    watch/status.c \
    watch/sportswatch.c \
    watch/notification.c \
    watch/worldclock.c \
    watch/window.c

OBJDIR = objs
SRCS = $(CORE) $(WATCH) $(PLATFORM) $(ANT) $(BTSTACK) $(GRLIB) $(MPL)
OBJS0 = $(SRCS:.c=.o)
OBJS = $(addprefix objs/, $(OBJS0))
DEPFILES = $(OBJS:.o=.d)

#####################
# rules to build the object files
$(OBJDIR)/%.o: %.c
	@$(ECHO) "Compiling $<"
	@test -d $(OBJDIR) || mkdir -pm 775 $(OBJDIR)
	@test -d $(@D) || mkdir -pm 775 $(@D)
	@-$(RM) $@
	@$(CC) $(CFLAGS) $(ALL_DEFINES:%=-D%) $(ALL_INCLUDEDIRS:%=-I%) -c $< -o $@

$(OBJDIR)/%.d: %.c
	@test -d $(OBJDIR) || mkdir -pm 775 $(OBJDIR)
	@test -d $(@D) || mkdir -pm 775 $(@D)
	@-$(RM) $@
	@$(CC) $(ALL_DEFINES:%=-D%) $(ALL_INCLUDEDIRS:%=-I%) -MM $< -MT $(patsubst %.d, %.o, $@)> $@


# create .hex file from .elf
%.hex: %.elf
	@$(OBJCOPY) -O ihex $< $@
	@$(SIZE) $<
%.txt: %.hex
	$(MAKETXT) -O $@ -TITXT $< -I
	unix2dos $@

# create firmware image from common objects and example source file

all: $(DEPFILES) $(OBJS) iwatch.hex iwatch.txt

iwatch.elf: ${OBJS} $(OBJDIR)/main.o0 $(OBJDIR)/$(OBJDIR)/symbols.o
	@$(ECHO) "Linking $@ second pass"
	@${CC} $^ ${LDFLAGS} -o $@

size: all
	msp430-size *.elf

flash: iwatch.hex
	~/bin/mspdebug tilib 'prog iwatch.elf'
	~/bin/mspdebug tilib run

$(OBJDIR)/iwatch.elf: ${OBJS} $(OBJDIR)/main.o0
	@echo "Link $@ first pass"
	@${CC} $^ ${LDFLAGS} -o $@

$(OBJDIR)/symbols.c: $(OBJDIR)/iwatch.elf
	@echo "Generate symbol table"
	@echo "#include \"loader/symbols-def.h\"" > $@
	@echo "const struct symbols symbols[] = {" >> $@
	@$(NM) $^ | awk '/([0-9a-f])+ [ABDRST] ([a-zA-Z][0-9A-Za-z_]+)$$/{print "{(void*)0x" $$1 ",\"" $$3 "\"}," }' | sort -f -k+2 -t ','>> $@
	@echo "{(void*)0, 0}};" >> $@

$(OBJDIR)/main.o0: platform/iwatch/contiki-exp5438-main.c
	@$(CC) $(CFLAGS0) $(ALL_DEFINES:%=-D%) $(ALL_INCLUDEDIRS:%=-I%) -c $< -o $@

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPFILES)
endif
.SILENT:
.PHONY:	clean
clean:
	rm -f $ *.elf *.hex
	rm -Rf objs/
