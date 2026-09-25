#
# VeebhaOS - QEMU RDA8809 MIPS XCPU Makefile
#
# SPDX-License-Identifier: MIT
#

COMPILER_PATH = /home/vixxkigoli/applications/cooltools/gcc/bin/
TOOLCHAIN_LIB_PATH = /home/vixxkigoli/applications/cooltools/gcc/lib

export LD_LIBRARY_PATH := $(TOOLCHAIN_LIB_PATH):$(LD_LIBRARY_PATH)

CC      = $(COMPILER_PATH)mips-elf-gcc
LD      = $(COMPILER_PATH)mips-elf-ld
OBJCOPY = $(COMPILER_PATH)mips-elf-objcopy
OBJDUMP = $(COMPILER_PATH)mips-elf-objdump

TOP_DIR       = ../..
BOARD_DIR     = .
OUT_DIR       = $(TOP_DIR)/build/qemu_xcpu
LINK_SCRIPT   = $(BOARD_DIR)/link.ld

QEMU_BIN      = /home/vixxkigoli/pm/qemu_xcpu/qemu/build/qemu-system-mipsel
QEMU_SDCARD   = /home/vixxkigoli/pm/qemu_xcpu/sdcard.img

TARGET_ELF    = $(OUT_DIR)/veebha_os.elf
TARGET_BIN    = $(OUT_DIR)/veebha_os.bin
DISASM_TXT    = $(OUT_DIR)/disassembly.txt

# Architecture & Compiler Flags
ARCH_FLAGS    = -march=xcpu -mtune=xcpu -msoft-float
RDA_ALIGN     = -fno-align-functions -fno-align-jumps -fno-align-labels
RDA_ISA       = -minterlink-mips16 -mexplicit-relocs -mmemcpy -finline-small-functions -frename-registers -fweb

INCLUDES      = -I$(TOP_DIR) \
                -I$(BOARD_DIR) \
                -I$(BOARD_DIR)/include \
                -I$(BOARD_DIR)/drivers \
                -I$(BOARD_DIR)/port \
                -I$(TOP_DIR)/kernel/freertos/include \
                -I$(TOP_DIR)/third_party/lwip/include \
                -I$(TOP_DIR)/apps/settings \
                -I$(TOP_DIR)/sdk/include \
                -I$(TOP_DIR)/sdk/core \
                -I$(TOP_DIR)/sdk/storage \
                -I$(TOP_DIR)/sdk/vfs \
                -I$(TOP_DIR)/sdk/text \
                -I$(TOP_DIR)/fonts \
                -I$(TOP_DIR)/apps/telephony \
                -I$(TOP_DIR)/apps/files \
                -I$(TOP_DIR)/apps/calendar \
                -I$(TOP_DIR)/apps/tools \
                -I$(TOP_DIR)/apps/game \
                -I$(TOP_DIR)/apps/gallery \
                -I$(TOP_DIR)/apps/recorder \
                -I$(TOP_DIR)/apps/overlays \
                -I$(TOP_DIR)/apps/vapps \
                -I$(TOP_DIR)/drivers \
                -I$(TOP_DIR)/third_party/lvgl

CFLAGS        = -c -g -Os $(ARCH_FLAGS) -mabi=32 -EL -G0 -ffreestanding -nostdlib -std=c99 \
                -DCONFIG_BOARD_QEMU_XCPU=1 \
                -DCONFIG_IS_RAMRUN_ONLY=1 \
                -DLV_CONF_INCLUDE_SIMPLE=1 \
                -ffunction-sections -fdata-sections \
                $(RDA_ALIGN) $(RDA_ISA) $(INCLUDES) -mno-mips16

LDFLAGS       = -EL -T $(LINK_SCRIPT) -e bl_entry -G0 --gc-sections

# ============================================================================
# SOURCE FILES
# ============================================================================

BOARD_C_SRCS  = $(BOARD_DIR)/main_qemu.c \
                $(BOARD_DIR)/board_info.c \
                $(BOARD_DIR)/board_hwlayer.c \
                $(BOARD_DIR)/port/os_vector_table.c \
                $(BOARD_DIR)/port/port.c \
                $(BOARD_DIR)/drivers/hal_display_qemu.c \
                $(BOARD_DIR)/drivers/hal_keypad_qemu.c \
                $(BOARD_DIR)/drivers/hal_sys_ctrl.c \
                $(BOARD_DIR)/drivers/hal_cfg_regs.c \
                $(BOARD_DIR)/drivers/hal_comregs.c \
                $(BOARD_DIR)/drivers/hal_gpio.c \
                $(BOARD_DIR)/drivers/hal_ispi.c \
                $(BOARD_DIR)/drivers/hal_pmd.c \
                $(BOARD_DIR)/drivers/hal_pwm.c \
                $(BOARD_DIR)/drivers/keypad.c \
                $(BOARD_DIR)/drivers/timer.c \
                $(BOARD_DIR)/drivers/irq.c \
                $(BOARD_DIR)/drivers/hal_gouda.c \
                $(BOARD_DIR)/drivers/lcd_ili9225g.c \
                $(BOARD_DIR)/drivers/hal_calendar.c \
                $(BOARD_DIR)/drivers/hal_torch.c \
                $(BOARD_DIR)/drivers/hal_backlight.c \
                $(BOARD_DIR)/drivers/hal_lps.c \
                $(BOARD_DIR)/drivers/hal_uart.c \
                $(BOARD_DIR)/drivers/hal_i2c.c \
                $(BOARD_DIR)/drivers/hal_bt.c \
                $(BOARD_DIR)/drivers/bnep.c \
                $(BOARD_DIR)/drivers/ethernetif_bnep.c \
                $(BOARD_DIR)/drivers/board_connectivity.c \
                $(BOARD_DIR)/drivers/net_app.c \
                $(BOARD_DIR)/drivers/hal_usb_cdc.c \
                $(BOARD_DIR)/drivers/hal_sdmmc.c \
                $(BOARD_DIR)/drivers/mcd_sdmmc.c \
                $(BOARD_DIR)/port/heap_4.c \
                $(BOARD_DIR)/port/extern.c

BOARD_ASM_SRCS = $(BOARD_DIR)/port/isram_stub.S

RTOS_SRCS     = $(TOP_DIR)/kernel/freertos/Source/tasks.c \
                $(TOP_DIR)/kernel/freertos/Source/queue.c \
                $(TOP_DIR)/kernel/freertos/Source/list.c \
                $(TOP_DIR)/kernel/freertos/Source/timers.c \
                $(TOP_DIR)/kernel/freertos/Source/event_groups.c \
                $(TOP_DIR)/kernel/freertos/Source/stream_buffer.c

LWIP_DIR      = $(TOP_DIR)/third_party/lwip
LWIP_SRCS     = $(LWIP_DIR)/core/init.c \
                $(LWIP_DIR)/core/def.c \
                $(LWIP_DIR)/core/dns.c \
                $(LWIP_DIR)/core/inet_chksum.c \
                $(LWIP_DIR)/core/ip.c \
                $(LWIP_DIR)/core/mem.c \
                $(LWIP_DIR)/core/memp.c \
                $(LWIP_DIR)/core/netif.c \
                $(LWIP_DIR)/core/pbuf.c \
                $(LWIP_DIR)/core/raw.c \
                $(LWIP_DIR)/core/stats.c \
                $(LWIP_DIR)/core/sys.c \
                $(LWIP_DIR)/core/tcp.c \
                $(LWIP_DIR)/core/tcp_in.c \
                $(LWIP_DIR)/core/tcp_out.c \
                $(LWIP_DIR)/core/timeouts.c \
                $(LWIP_DIR)/core/udp.c \
                $(LWIP_DIR)/core/ipv4/autoip.c \
                $(LWIP_DIR)/core/ipv4/dhcp.c \
                $(LWIP_DIR)/core/ipv4/etharp.c \
                $(LWIP_DIR)/core/ipv4/icmp.c \
                $(LWIP_DIR)/core/ipv4/igmp.c \
                $(LWIP_DIR)/core/ipv4/ip4.c \
                $(LWIP_DIR)/core/ipv4/ip4_addr.c \
                $(LWIP_DIR)/core/ipv4/ip4_frag.c \
                $(LWIP_DIR)/netif/ethernet.c

VEEBHA_CORE   = $(TOP_DIR)/sdk/core/os_kernel.c \
                $(TOP_DIR)/sdk/core/softkey_bar.c \
                $(TOP_DIR)/sdk/core/status_bar.c \
                $(TOP_DIR)/sdk/core/win_mgr.c \
                $(TOP_DIR)/sdk/core/theme_mgr.c \
                $(TOP_DIR)/sdk/core/wallpaper.c \
                $(TOP_DIR)/sdk/core/live_pill.c \
                $(TOP_DIR)/sdk/core/veebha_log.c \
                $(TOP_DIR)/sdk/core/os_event.c \
                $(TOP_DIR)/sdk/core/app_registry.c \
                $(TOP_DIR)/sdk/input/t9_engine.c \
                $(TOP_DIR)/sdk/templates/tpl_list.c \
                $(TOP_DIR)/sdk/templates/tpl_grid.c \
                $(TOP_DIR)/sdk/templates/tpl_dialog.c \
                $(TOP_DIR)/sdk/templates/tpl_editor.c \
                $(TOP_DIR)/sdk/templates/tpl_media.c \
                $(TOP_DIR)/sdk/storage/os_nvram.c \
                $(TOP_DIR)/sdk/vfs/os_vfs.c \
                $(TOP_DIR)/sdk/text/font_fallback.c \
                $(TOP_DIR)/sdk/text/indic_shaper.c \
                $(TOP_DIR)/sdk/core/veebha_i18n.c \
                $(TOP_DIR)/sdk/i18n/strings_en.c \
                $(TOP_DIR)/sdk/i18n/strings_hi.c \
                $(TOP_DIR)/sdk/i18n/strings_ru.c \
                $(TOP_DIR)/fonts/veebha_font_latin_cyrillic_14.c \
                $(TOP_DIR)/fonts/veebha_font_devanagari_16.c \
                $(TOP_DIR)/sdk/core/vapp_loader.c

APPS_SRCS     = $(TOP_DIR)/apps/home/app_idle.c \
                $(TOP_DIR)/apps/home/app_launcher.c \
                $(TOP_DIR)/apps/dialer/app_dialer.c \
                $(TOP_DIR)/apps/telephony/app_incall.c \
                $(TOP_DIR)/apps/contacts/app_contacts.c \
                $(TOP_DIR)/apps/messages/app_messages.c \
                $(TOP_DIR)/apps/calllogs/app_calllogs.c \
                $(TOP_DIR)/apps/music/app_music.c \
                $(TOP_DIR)/apps/files/app_files.c \
                $(TOP_DIR)/apps/settings/app_settings.c \
                $(TOP_DIR)/apps/settings/app_bt.c \
                $(TOP_DIR)/apps/settings/app_bt_scan.c \
                $(TOP_DIR)/apps/settings/app_tethering.c \
                $(TOP_DIR)/apps/calendar/app_calendar.c \
                $(TOP_DIR)/apps/tools/app_tools.c \
                $(TOP_DIR)/apps/tools/app_calc.c \
                $(TOP_DIR)/apps/tools/app_stopwatch.c \
                $(TOP_DIR)/apps/tools/app_alarm.c \
                $(TOP_DIR)/apps/tools/app_textread.c \
                $(TOP_DIR)/apps/tools/app_funzone.c \
                $(TOP_DIR)/apps/browser/app_browser.c \
                $(TOP_DIR)/apps/game/app_game.c \
                $(TOP_DIR)/apps/gallery/app_gallery.c \
                $(TOP_DIR)/apps/recorder/app_recorder.c \
                $(TOP_DIR)/apps/overlays/boot_screen.c \
                $(TOP_DIR)/apps/overlays/screen_saver.c \
                $(TOP_DIR)/apps/overlays/usb_select.c \
                $(TOP_DIR)/apps/overlays/notif_panel.c \
                $(TOP_DIR)/apps/overlays/task_mgr.c \
                $(TOP_DIR)/apps/common/mock_telephony.c \
                $(TOP_DIR)/apps/vapps/vapp_tetris.c \
                $(TOP_DIR)/apps/vapps/vapp_brick_breaker.c \
                $(TOP_DIR)/apps/vapps/vapp_unit_converter.c \
                $(TOP_DIR)/apps/vapps/vapp_morse_flasher.c \
                $(TOP_DIR)/apps/vapps/vapp_chip_synth.c \
                $(TOP_DIR)/apps/vapps/vapp_sys_monitor.c

# Find all LVGL core source files
LVGL_SRCS     = $(shell find $(TOP_DIR)/third_party/lvgl/src -name "*.c")

ALL_C_SRCS    = $(BOARD_C_SRCS) $(RTOS_SRCS) $(LWIP_SRCS) $(VEEBHA_CORE) $(APPS_SRCS) $(LVGL_SRCS)
ALL_OBJS      = $(patsubst $(TOP_DIR)/%.c, $(OUT_DIR)/%.o, $(patsubst $(BOARD_DIR)/%.c, $(OUT_DIR)/board/%.o, $(ALL_C_SRCS))) \
                $(OUT_DIR)/board/port/isram_stub.o \
                $(BOARD_DIR)/port/mgr_ecc.o

# ============================================================================
# BUILD TARGETS
# ============================================================================

all: $(TARGET_BIN) disasm

$(OUT_DIR)/board/%.o: $(BOARD_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

$(OUT_DIR)/board/port/isram_stub.o: $(BOARD_DIR)/port/isram_stub.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@

$(OUT_DIR)/%.o: $(TOP_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

$(TARGET_ELF): $(ALL_OBJS) $(LINK_SCRIPT)
	@mkdir -p $(OUT_DIR)
	$(LD) $(LDFLAGS) --start-group $(ALL_OBJS) --end-group -o $(TARGET_ELF)

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $(TARGET_ELF) $(TARGET_BIN)
	@echo "===================================================="
	@echo "[+] VeebhaOS QEMU XCPU Binary Built: $(TARGET_BIN)"
	@echo "===================================================="

disasm: $(TARGET_ELF)
	$(OBJDUMP) -D -EL -m mips:isa32 $(TARGET_ELF) > $(DISASM_TXT)
	@echo "[+] Disassembly generated: $(DISASM_TXT)"

run-qemu: $(TARGET_BIN)
	$(QEMU_BIN) -M rda_xcpu,bev0-vector=0x81c00280,bev1-vector=0x81e00180,entry-point=0x82000280 \
	  -bios $(TARGET_BIN) \
	  $(shell [ -f "$(QEMU_SDCARD)" ] && echo "-drive file=$(QEMU_SDCARD),format=raw,if=none,id=sdcard")

clean:
	rm -rf $(OUT_DIR)
	@echo "Cleaned $(OUT_DIR)"

.PHONY: all clean disasm run-qemu
