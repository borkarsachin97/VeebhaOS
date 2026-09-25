/*
 * VeebhaOS - QEMU RDA8809 MIPS XCPU Main Entry & Hardware Bringup
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "boards/board_config.h"
#include "boards/qemu_xcpu/drivers/hal_display_qemu.h"
#include "boards/qemu_xcpu/drivers/hal_keypad_qemu.h"
#include "boards/qemu_xcpu/include/hal_sys_ctrl.h"
#include "boards/qemu_xcpu/include/hal_cfg_regs.h"
#include "boards/qemu_xcpu/include/hal_comregs.h"
#include "boards/qemu_xcpu/include/hal_gpio.h"
#include "boards/qemu_xcpu/include/hal_ispi.h"
#include "boards/qemu_xcpu/include/hal_pmd.h"
#include "boards/qemu_xcpu/include/hal_pwm.h"
#include "boards/qemu_xcpu/include/hal_gouda.h"
#include "boards/qemu_xcpu/include/lcd_ili9225g.h"
#include "boards/qemu_xcpu/include/timer.h"
#include "boards/qemu_xcpu/include/os_vector_table.h"
#include "kernel/freertos/include/FreeRTOS.h"
#include "kernel/freertos/include/task.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_i18n.h"
#include "sdk/core/os_kernel.h"
#include "apps/home/app_idle.h"
#include "apps/home/app_launcher.h"
#include "third_party/lvgl/lvgl.h"
#include "sdk/include/app_registry.h"
#include "sdk/storage/os_nvram.h"
#include "sdk/include/veebha_hardware.h"
#include "apps/settings/app_settings.h"
#include "apps/files/app_files.h"
#include "apps/music/app_music.h"
#include "apps/calendar/app_calendar.h"
#include "apps/tools/app_tools.h"
#include "apps/tools/app_stopwatch.h"
#include "apps/tools/app_alarm.h"
#include "apps/tools/app_textread.h"
#include "apps/game/app_game.h"
#include "apps/gallery/app_gallery.h"
#include "apps/recorder/app_recorder.h"

extern void os_log_printf(const char *fmt, ...);
#include "apps/overlays/screen_saver.h"
#include "apps/settings/app_bt.h"
#include "drivers/mock/mock_connectivity.h"
#include "boards/qemu_xcpu/include/hal_calendar.h"
#include "boards/qemu_xcpu/include/hal_torch.h"
#include "boards/qemu_xcpu/include/hal_backlight.h"
#include "boards/qemu_xcpu/include/keypad.h"
#include "sdk/include/veebha_event.h"
#include "sdk/include/veebha_status_bar.h"
#include "sdk/include/veebha_log.h"
#include <stdint.h>
#include <stdbool.h>

#define TAG "QEMU_MAIN"

/* Hardware Entry Point at 0x82000280 */
extern uint32_t _bss_start;
extern uint32_t _bss_end;

void _main(void);

void __attribute__((section("._start"))) bl_entry(void) {
    __asm__ volatile (
        "move  $gp, $zero\n\t"
        "lui   $s8, 0x827f\n\t"
        "ori   $s8, $s8, 0xfff8\n\t"
        "move  $sp, $s8\n\t"
        "lui   $t0, %hi(_bss_start)\n\t"
        "addiu $t0, $t0, %lo(_bss_start)\n\t"
        "lui   $t1, %hi(_bss_end)\n\t"
        "addiu $t1, $t1, %lo(_bss_end)\n\t"
        "1:\n\t"
        "sltu  $t2, $t0, $t1\n\t"
        "beqz  $t2, 2f\n\t"
        "nop\n\t"
        "sw    $zero, 0($t0)\n\t"
        "addiu $t0, $t0, 4\n\t"
        "b     1b\n\t"
        "nop\n\t"
        "2:\n\t"
        "j _main\n\t"
        "nop\n\t"
    );
}

static void veebha_main_ui_task(void *pvParameters)
{
	
    (void)pvParameters;
    os_log_printf("[UI_TASK] Starting LVGL & GUI bringup...\n");

    /* 1. Initialize LVGL 9 Graphics Engine & Tick Callback */
    lv_init();
    lv_tick_set_cb(timer_get_ms);
    os_log_printf("[UI_TASK] lv_init & tick_cb done.\n");

    /* 2. Initialize Hardware GOUDA Blitter Display Driver */
    hal_display_qemu_init();
    os_log_printf("[UI_TASK] hal_display_qemu_init done.\n");

    /* 3. Initialize Keypad Group & Window Manager */
    lv_group_t *keypad_group = lv_group_create();
    lv_group_set_default(keypad_group);
    win_mgr_init(keypad_group);
    theme_init();
    os_log_printf("[UI_TASK] win_mgr & theme init done.\n");

    /* 4. Initialize Hardware Keypad Driver & Bind LVGL Group */
    hal_keypad_qemu_init();
    os_log_printf("[UI_TASK] hal_keypad_qemu_init done.\n");

    /* 5. Initialize Localization & i18n */
    veebha_i18n_init(LANG_EN);

    /* 6. Initialize Core Subsystems & App Registry */
    os_nvram_init();
    app_settings_init();
    os_app_registry_init();
    app_files_init();
    app_music_init();
    app_calendar_init();
    app_tools_init();
    app_stopwatch_init();
    app_alarm_init();
    app_textread_init();
    app_game_init();
    app_gallery_init();
    app_recorder_init();
    screen_saver_init();
    mock_connectivity_init();
    app_bt_init();

    /* Initialize Low Power Scheme (LPS) with 10s default screen timeout */
    veebha_hw_lps_init(10);
    veebha_hw_usb_console_init();
    veebha_hw_sdcard_init();

    /* Initial Hardware RTC synchronization */
    uint8_t init_h = 12, init_m = 0, init_s = 0;
    uint16_t init_yr = 2026; uint8_t init_mo = 1, init_dy = 1;
    if (veebha_hw_rtc_get_time(&init_h, &init_m, &init_s)) {
        status_bar_set_rtc_time(init_h, init_m);
    }
    if (veebha_hw_rtc_get_date(&init_yr, &init_mo, &init_dy)) {
        status_bar_set_rtc_date(init_yr, init_mo, init_dy);
    }

    os_log_printf("[UI_TASK] app registry & core apps initialized.\n");
    /* 7. Launch VeebhaOS Standby / Idle Screen */
    app_idle_open();
    os_log_printf("[UI_TASK] app_idle_open done. Entering UI loop.\n");

    /* 8. Main LVGL Execution & Render Loop */
    static uint32_t last_rtc_sync = 0;
    while (1) {
        uint32_t now = timer_get_ms();
        if (now - last_rtc_sync >= 1000) {
            last_rtc_sync = now;
            uint8_t cur_h = 0, cur_m = 0, cur_s = 0;
            uint16_t cur_yr = 0; uint8_t cur_mo = 0, cur_dy = 0;
            if (veebha_hw_rtc_get_time(&cur_h, &cur_m, &cur_s)) {
                status_bar_set_rtc_time(cur_h, cur_m);
            }
            if (veebha_hw_rtc_get_date(&cur_yr, &cur_mo, &cur_dy)) {
                status_bar_set_rtc_date(cur_yr, cur_mo, cur_dy);
            }
        }

        static uint32_t last_bat_sync = 0;
        if (now - last_bat_sync >= 2000) {
            last_bat_sync = now;
            uint8_t bat_pct = veebha_hw_battery_get_percent();
            bool is_chg = veebha_hw_battery_is_charging();
            status_bar_set_battery(bat_pct, is_chg);
        }

        /* Service Low Power Scheme display timeout & wake transitions */
        veebha_hw_lps_poll();
        veebha_hw_usb_console_poll();

        extern void board_connectivity_poll(void);
        board_connectivity_poll();

        /* Drain OS system event queue */
        os_event_t evt;
        while (os_event_poll(&evt)) {
            os_dispatch_system_event(&evt);
        }

        os_lvgl_lock();
        uint32_t delay_ms = lv_timer_handler();
        os_lvgl_unlock();
        if (delay_ms < 5) delay_ms = 5;
        if (delay_ms > 100) delay_ms = 100;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

void _main(void)
{
    /* 1. Defensive BSS clear in C */
    uint32_t *p_bss = &_bss_start;
    while (p_bss < &_bss_end) {
        *p_bss++ = 0;
    }

    /* 2. Initialize RDA8809 Hardware Subsystems at Maximum 312 MHz CPU Frequency */
    hal_SysCtrlInit();
    hal_SysSetCpuFreq(HAL_SYS_FREQ_312M);
    timer_init();
    hal_CfgRegsInit();
    hal_ComregsInit();
    hal_GpioInit();
    os_vector_system_init();
    os_kernel_init();
    hal_IspiInit();
    hal_PmdInit();
    hal_PmdSetKeypadLed(TRUE, 4);
    hal_CalendarInit();
    hal_TorchInit();
    hal_BacklightInit();

    /* 3. Initialize Keypad Hardware Controller & Interrupt Handler */
    keypad_init();
    keypad_set_irq_mode(TRUE);

    /* 4. Power cycle LCD display rail (V_LCD 2.8V) via PMU */
    hal_PmdSetLcdPower(FALSE);
    timer_delay_ms(30);
    hal_PmdSetLcdPower(TRUE);
    timer_delay_ms(50);

    /* 5. Initialize Hardware GOUDA Blitter Controller */
    hal_GoudaInit();

    /* 6. Retrieve and initialize ILI9225G LCD Panel Driver */
    const lcd_panel_t *panel = lcd_ili9225g_get_panel();
    if (panel && panel->init) {
        panel->init(NULL);
    }
    if (panel && panel->set_window) {
        panel->set_window(0, 0, 175, 219);
    }
    hal_GoudaFillRect(0, 0, 176, 220, 0x0000);

    /* 7. Configure Backlight */
    hal_BacklightSetLevel(180);

    /* 8. Create Dedicated UI & Keypad FreeRTOS Tasks
     * Keypad task is created at high priority (4) and UI task at normal priority (2)
     * so continuous game/music drawing loops can NEVER starve keypad input processing.
     */
    extern TaskHandle_t g_keypad_task_handle;
    BaseType_t r1 = xTaskCreate(hal_keypad_task,    "Keypad",   2048, NULL, 4, &g_keypad_task_handle);
    BaseType_t r2 = xTaskCreate(veebha_main_ui_task, "VeebhaUI", 4096, NULL, 2, NULL);
    os_log_printf("[MAIN] xTaskCreate Keypad=%d (pri 4), VeebhaUI=%d (pri 2)\n", (int)r1, (int)r2);
    /* 9. Start FreeRTOS Scheduler */
    os_log_printf("[MAIN] Starting FreeRTOS Scheduler...\n");
    vTaskStartScheduler();

    while (1) {
        /* Should never be reached */
    }
}

void vApplicationIdleHook(void)
{
    /* Low power idle hook: yield / nop on XCPU */
    __asm__ volatile ("nop");
}

