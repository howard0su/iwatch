CC      = /opt/local/bin/msp430-gcc
CFLAGS  = -mmcu=msp430f5438a -std=c99 -Os -Wall -mc20 -md20 -msr20
LDFLAGS = -mmcu=msp430f5438a -lm
ECHO	= echo

ALL_DEFINES = AUTOSTART_ENABLE=1 HAVE_BLE=1
ALL_INCLUDEDIRS = \
          core \
          core/lib \
          cpu/msp430 \
          platform/iwatch \
          platform/iwatch/btstack \
          platform/iwatch/btstack/src \
          platform/iwatch/btstack/ble \
          platform/iwatch/btstack \
          platform/iwatch/btstack/chipset-cc256x \
          platform/iwatch/btstack/include \

CORE   = \
    core/sys/autostart.c \
    core/sys/ctimer.c \
    core/sys/etimer.c \
    core/sys/energest.c \
    core/sys/process.c \
    core/sys/procinit.c \
    core/sys/rtimer.c \
    core/sys/stimer.c \
    core/sys/timer.c \
    core/lib/assert.c \
    core/lib/list.c 

PLATFORM = \
	platform/iwatch/backlight.c \
	platform/iwatch/button-sensor.c \
	platform/iwatch/clock.c \
	platform/iwatch/contiki-exp5438-main.c \
	platform/iwatch/flash.c \
	platform/iwatch/i2c.c \
	platform/iwatch/isr.c \
	platform/iwatch/msp430.c \
	platform/iwatch/printf.c \
	platform/iwatch/rtc.c \
	platform/iwatch/rtimer-arch.c \
	platform/iwatch/uart1-putchar.c \
	platform/iwatch/uart1x.c \
	platform/iwatch/watchdog.c \
	platform/iwatch/Template_Driver.c
    
ANT0 = \
	base/ANTInterface.c \
	base/cbsc_rx.c \
	base/hrm_rx.c \
	base/main_hrm.c \
	base/serial.c \
	Timer_impl.c
ANT = $(addprefix platform/iwatch/ant/, $(ANT0))

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
	fontnova28b.c
GRLIB = $(addprefix platform/iwatch/grlib/, $(GRLIB0)) $(addprefix platform/iwatch/grlib/fonts/, $(GRLIB_FONTS))

BTSTACK0 = \
	bluetooth.c \
	codec.c \
	deviceid.c \
	hal.c \
	hal_compat.c \
	hal_uart_dma.c \
	run_loop.c \
	sdp_client.c \
	spp.c \
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

BTSTACK = $(addprefix platform/iwatch/btstack/, $(BTSTACK0))

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
    watch/menu.c \
    watch/watch.c \
    watch/window.c

OBJDIR_1 = objs
SRCS = $(CORE) $(WATCH) $(PLATFORM) $(ANT) $(BTSTACK) $(GRLIB) $(MPL)
OBJS0 = $(SRCS:.c=.o)
OBJS = $(addprefix objs/, $(OBJS0))

#####################
# rules to build the object files
$(OBJDIR_1)/%.o: %.c
	@$(ECHO) "$< -> $@"
	@test -d $(OBJDIR_1) || mkdir -pm 775 $(OBJDIR_1)
	@test -d $(@D) || mkdir -pm 775 $(@D)
	@-$(RM) $@
	$(CC) $(CFLAGS) $(CFLAGS_1) $(ALL_FLAGS) $(ALL_DEFINES:%=-D%) $(ALL_INCLUDEDIRS:%=-I%) -c $< -o $@

# create .hex file from .out
%.hex: %.out
	msp430-objcopy -O ihex $< $@    

# create firmware image from common objects and example source file

all: iwatch.hex

iwatch.out: ${OBJS}
	${CC} $^ ${LDFLAGS} -o $@

clean:
	rm -f $ *.out *.hex
	rm -Rf objs/

size: all
	msp430-size *.out 
