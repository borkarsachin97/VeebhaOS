/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * ILI9225G LCD Panel Driver Implementation for RDA8809
 */

#include "cs_types.h"
#include "global_macros.h"
#include "timer.h"
#include "hal_gpio.h"
#include "hal_gouda.h"
#include "lcd_panel.h"
#include "lcd_ili9225g.h"

// External USB logging
extern void os_log_printf(const char *fmt, ...);

#define ILI9225G_RESET_GPO_PIN  7

// =============================================================================
//  ILI9225G Driver Implementation
// =============================================================================

static uint16_t ili9225g_read_id(void)
{
    uint16_t id = hal_GoudaReadReg(0x0000);
    os_log_printf("[LCD] Probing panel ID register (0x0000)... Read: 0x%04X (%s)\n",
                  id, (id == 0x9226) ? "ILI9225G" : ((id == 0x9225) ? "ILI9225/B" : "Compatible"));
    return id;
}

static void ili9225g_power_on_seq(uint16_t id)
{
    // Initial display mode settings
    hal_GoudaWriteReg(0x0001, 0x011C); // set SS and NL bit
    hal_GoudaWriteReg(0x0002, 0x0100); // set 1 line inversion
    hal_GoudaWriteReg(0x0003, 0x1030); // set GRAM write direction and BGR=1

    if (id == 0x9226)
    {
        /* ILI9225G specific configuration */
        hal_GoudaWriteReg(0x00D0, 0x0003);
        hal_GoudaWriteReg(0x00EB, 0x0B00);
        hal_GoudaWriteReg(0x00EC, 0x000F);
        hal_GoudaWriteReg(0x00C7, 0x030F);
        hal_GoudaWriteReg(0x0007, 0x0000);
        hal_GoudaWriteReg(0x0008, 0x0808);
        hal_GoudaWriteReg(0x000F, 0x0901);

        timer_delay_ms(50);
        hal_GoudaWriteReg(0x0010, 0x0000);
        hal_GoudaWriteReg(0x0011, 0x1B41);
        timer_delay_ms(50);
        hal_GoudaWriteReg(0x0012, 0x200E);
        hal_GoudaWriteReg(0x0013, 0x0052);
        hal_GoudaWriteReg(0x0014, 0x4B5C);

        /* Set GRAM Area */
        hal_GoudaWriteReg(0x0030, 0x0000);
        hal_GoudaWriteReg(0x0031, 0x00DB);
        hal_GoudaWriteReg(0x0032, 0x0000);
        hal_GoudaWriteReg(0x0033, 0x0000);
        hal_GoudaWriteReg(0x0034, 0x00DB);
        hal_GoudaWriteReg(0x0035, 0x0000);
        hal_GoudaWriteReg(0x0036, 0x00AF);
        hal_GoudaWriteReg(0x0037, 0x0000);
        hal_GoudaWriteReg(0x0038, 0x00DB);
        hal_GoudaWriteReg(0x0039, 0x0000);

        /* ILI9225G Gamma Curve */
        hal_GoudaWriteReg(0x0050, 0x0000);
        hal_GoudaWriteReg(0x0051, 0x0705);
        hal_GoudaWriteReg(0x0052, 0x0C0A);
        hal_GoudaWriteReg(0x0053, 0x0401);
        hal_GoudaWriteReg(0x0054, 0x040C);
        hal_GoudaWriteReg(0x0055, 0x0608);
        hal_GoudaWriteReg(0x0056, 0x0000);
        hal_GoudaWriteReg(0x0057, 0x0104);
        hal_GoudaWriteReg(0x0058, 0x0E06);
        hal_GoudaWriteReg(0x0059, 0x060E);
    }
    else
    {
        /* ILI9225 / ILI9225B / ST7775R standard configuration */
        hal_GoudaWriteReg(0x0008, 0x0808); /* Set BP and FP */
        hal_GoudaWriteReg(0x000C, 0x0000); /* RGB interface setting */
        hal_GoudaWriteReg(0x000F, 0x0601); /* Set frame rate (OSC frequency) */
        hal_GoudaWriteReg(0x0020, 0x0000); /* Set GRAM Address X */
        hal_GoudaWriteReg(0x0021, 0x0000); /* Set GRAM Address Y */

        timer_delay_ms(50);
        hal_GoudaWriteReg(0x0010, 0x0A00); /* Set SAP,DSTB,STB */
        hal_GoudaWriteReg(0x0011, 0x1038); /* Set APON,PON,AON,VCI1EN,VC */
        timer_delay_ms(50);
        hal_GoudaWriteReg(0x0012, 0x1121); /* Internal reference voltage = Vci */
        hal_GoudaWriteReg(0x0013, 0x006E); /* Set GVDD */
        hal_GoudaWriteReg(0x0014, 0x6563); /* Set VCOMH/VCOML voltage */

        /* Set GRAM Area */
        hal_GoudaWriteReg(0x0030, 0x0000);
        hal_GoudaWriteReg(0x0031, 0x00DB);
        hal_GoudaWriteReg(0x0032, 0x0000);
        hal_GoudaWriteReg(0x0033, 0x0000);
        hal_GoudaWriteReg(0x0034, 0x00DB);
        hal_GoudaWriteReg(0x0035, 0x0000);
        hal_GoudaWriteReg(0x0036, 0x00AF);
        hal_GoudaWriteReg(0x0037, 0x0000);
        hal_GoudaWriteReg(0x0038, 0x00DB);
        hal_GoudaWriteReg(0x0039, 0x0000);

        /* ILI9225B Gamma Curve */
        hal_GoudaWriteReg(0x0050, 0x0000);
        hal_GoudaWriteReg(0x0051, 0x0705);
        hal_GoudaWriteReg(0x0052, 0x0E0A);
        hal_GoudaWriteReg(0x0053, 0x0300);
        hal_GoudaWriteReg(0x0054, 0x0A0E);
        hal_GoudaWriteReg(0x0055, 0x0507);
        hal_GoudaWriteReg(0x0056, 0x0000);
        hal_GoudaWriteReg(0x0057, 0x0003);
        hal_GoudaWriteReg(0x0058, 0x090A);
        hal_GoudaWriteReg(0x0059, 0x0A09);
    }

    timer_delay_ms(50);
    hal_GoudaWriteReg(0x0020, 0x0000);
    hal_GoudaWriteReg(0x0021, 0x0000);
    hal_GoudaWriteReg(0x0007, 0x1017); // Display ON

    hal_GoudaWriteCmd(0x00);
    hal_GoudaWriteCmd(0x22);
}

static bool ili9225g_init(const lcd_panel_config_t *config)
{
    (void)config;
    os_log_printf("[LCD] Initializing ILI9225 display panel (%ux%u RGB565)...\n",
                  ILI9225G_LCD_WIDTH, ILI9225G_LCD_HEIGHT);

    // 1. Hardware Reset via dedicated GPO Pin 7 (active-low) alongside Gouda resetb
    hal_GpoClr(ILI9225G_RESET_GPO_PIN);
    timer_delay_ms(15);
    hal_GpoSet(ILI9225G_RESET_GPO_PIN);
    timer_delay_ms(50);

    // 2. Query Controller ID
    uint16_t id = ili9225g_read_id();

    // 3. Hardware Power-On & Step-Up Circuit Initialization
    ili9225g_power_on_seq(id);

    os_log_printf("[LCD] Panel initialization complete (ID: 0x%04X, Display ON).\n", id);
    return true;
}

static void ili9225g_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    if (x0 >= ILI9225G_LCD_WIDTH)  x0 = ILI9225G_LCD_WIDTH - 1;
    if (x1 >= ILI9225G_LCD_WIDTH)  x1 = ILI9225G_LCD_WIDTH - 1;
    if (y0 >= ILI9225G_LCD_HEIGHT) y0 = ILI9225G_LCD_HEIGHT - 1;
    if (y1 >= ILI9225G_LCD_HEIGHT) y1 = ILI9225G_LCD_HEIGHT - 1;

    // Configure ILI9225G Horizontal Window (Reg 0x37 = Start, Reg 0x36 = End)
    hal_GoudaWriteReg(0x0037, x0);
    hal_GoudaWriteReg(0x0036, x1);

    // Configure ILI9225G Vertical Window (Reg 0x39 = Start, Reg 0x38 = End)
    hal_GoudaWriteReg(0x0039, y0);
    hal_GoudaWriteReg(0x0038, y1);

    // Set RAM Address Counter to starting pixel
    hal_GoudaWriteReg(0x0020, x0);
    hal_GoudaWriteReg(0x0021, y0);

    // Prepare GRAM write (Reg 0x22)
    hal_GoudaWriteCmd(0x00);
    hal_GoudaWriteCmd(0x22);
}

static void ili9225g_sleep(void)
{
    hal_GoudaWaitIdle(100);
    os_log_printf("[LCD] Putting ILI9225G panel into sleep mode (0x0007->0x0000, 0x0010->0x0801)...\n");
    hal_GoudaWriteReg(0x0007, 0x0000); // Display OFF
    timer_delay_ms(10);
    hal_GoudaWriteReg(0x0010, 0x0801); // Power Control 1: Enter Sleep (SLP=1)
    timer_delay_ms(20);
}

static void ili9225g_wakeup(void)
{
    os_log_printf("[LCD] Waking ILI9225G panel from sleep mode (Exit Sleep & Step-Up Active)...\n");

    // 1. Exit sleep mode (SLP=0, SAP=1000)
    hal_GoudaWriteReg(0x0010, 0x0800);
    timer_delay_ms(15);

    // 2. Re-enable step-up circuits
    hal_GoudaWriteReg(0x0011, 0x103B);
    timer_delay_ms(40);

    // 3. Confirm Driver Output and Entry mode (SS=1 unmirrored, BGR=1 RGB565)
    hal_GoudaWriteReg(0x0001, 0x011C);
    hal_GoudaWriteReg(0x0003, 0x1030);

    // 4. Set full screen active window & cursor
    hal_GoudaWriteReg(0x0037, 0x0000); // Horizontal Start
    hal_GoudaWriteReg(0x0036, 0x00AF); // Horizontal End (175)
    hal_GoudaWriteReg(0x0039, 0x0000); // Vertical Start
    hal_GoudaWriteReg(0x0038, 0x00DB); // Vertical End (219)
    hal_GoudaWriteReg(0x0020, 0x0000);
    hal_GoudaWriteReg(0x0021, 0x0000);

    // 5. Turn ON display (GON=1, D=11)
    hal_GoudaWriteReg(0x0007, 0x1017);
    timer_delay_ms(20);

    // 6. Prepare Write GRAM command (0x22)
    hal_GoudaWriteCmd(0x00);
    hal_GoudaWriteCmd(0x22);

    os_log_printf("[LCD] ILI9225G panel wakeup complete.\n");
}

// =============================================================================
//  Singleton Panel Definition
// =============================================================================
static const lcd_panel_t g_ili9225g_panel = {
    .name       = "ILI9225G",
    .width      = ILI9225G_LCD_WIDTH,
    .height     = ILI9225G_LCD_HEIGHT,
    .init       = ili9225g_init,
    .read_id    = ili9225g_read_id,
    .set_window = ili9225g_set_window,
    .sleep      = ili9225g_sleep,
    .wakeup     = ili9225g_wakeup
};

const lcd_panel_t* lcd_ili9225g_get_panel(void)
{
    return &g_ili9225g_panel;
}
