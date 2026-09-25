/**
 * @file lv_conf.h
 * Configuration file for LVGL v9 on RDA8809 MIPS XCPU
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_CONF_SKIP 0
#define LV_ATTRIBUTE_DEPRECATED __attribute__((deprecated))
#define LV_DEPRECATED(msg) __attribute__((deprecated))
#define LV_DEPRECATED_MACRO_WARN(msg) ((void)0)
#define LV_DEPRECATIONS_IGNORE_BEGIN
#define LV_DEPRECATIONS_IGNORE_END

/*============================================================================
 * MEMORY AND STANDARD LIBRARY
 *============================================================================*/

#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (256 * 1024)
#define LV_MEM_ADR 0x0
#endif

#define LV_STDINT_INCLUDE "stdint.h"
#define LV_STDDEF_INCLUDE "stddef.h"
#define LV_STDBOOL_INCLUDE "stdbool.h"
#define LV_INTTYPES_INCLUDE "inttypes.h"
#define LV_LIMITS_INCLUDE "limits.h"
#define LV_STDARG_INCLUDE "stdarg.h"

/*============================================================================
 * HAL SETTINGS
 *============================================================================*/

#define LV_COLOR_DEPTH 16

#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "timer.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (timer_get_ms())
#endif

#define LV_DEF_REFR_PERIOD 33 /* 30 FPS */
#define LV_DPI_DEF 130

/*============================================================================
 * FEATURE CONFIGURATION
 *============================================================================*/

#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL 0
#define LV_USE_ASSERT_MALLOC 0
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0

#define LV_USE_USER_DATA 1
#define LV_ENABLE_GLOBAL_CUSTOM 0
#define LV_USE_OBSERVER 0
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/* Software Draw Optimizations & Anti-Aliasing Tuning */
#define LV_USE_DRAW_SW 1
#define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
#define LV_DRAW_SW_CIRCLE_CACHE_SIZE 0

#define LV_USE_SYSMON 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

/*============================================================================
 * WIDGET CONFIGURATION (Trimmed for physical keypad feature phone)
 *============================================================================*/

#define LV_USE_ANIMIMG 0
#define LV_USE_ARC 0
#define LV_USE_BAR 1
#define LV_USE_BUTTON 1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR 0
#define LV_USE_CANVAS 0
#define LV_USE_CHART 0
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 0
#define LV_USE_IMAGE 1
#define LV_USE_IMAGEBUTTON 1
#define LV_USE_KEYBOARD 0
#define LV_USE_LABEL 1
#define LV_LABEL_TEXT_SELECTION 0
#define LV_LABEL_LONG_TXT_HINT 1
#define LV_USE_LED 1
#define LV_USE_LINE 0
#define LV_USE_LIST 1
#define LV_USE_LOTTIE 0
#define LV_USE_MENU 0
#define LV_USE_MSGBOX 0
#define LV_USE_ROLLER 0
#define LV_USE_SCALE 0
#define LV_USE_SLIDER 1
#define LV_USE_SPAN 0
#define LV_USE_SPINBOX 0
#define LV_USE_SPINNER 0
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 0
#define LV_USE_TABVIEW 0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0

/*============================================================================
 * THEMES & FONTS
 *============================================================================*/

#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK 1
    #define LV_THEME_DEFAULT_GROW 0
    #define LV_THEME_DEFAULT_TRANSITION_TIME 0
#endif

#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#endif /* LV_CONF_H */
