/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Board Configuration: OBTEL B10 (RDA8809 MIPS32r1)
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARDS_OBTEL_B10_BOARD_OBTEL_B10_H
#define BOARDS_OBTEL_B10_BOARD_OBTEL_B10_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_BOARD_NAME "OBTEL B10 (RDA8809)"

/* Display Configuration: 176x220 portrait RGB565 */
#define CONFIG_DISP_HOR_RES 176
#define CONFIG_DISP_VER_RES 220
#define CONFIG_COLOR_DEPTH  16

/* Hardware Capabilities */
#define CONFIG_BOARD_SUPPORT_2G          1
#define CONFIG_BOARD_SUPPORT_3G          0
#define CONFIG_BOARD_SUPPORT_4G          0
#define CONFIG_BOARD_SUPPORT_WIFI        0
#define CONFIG_BOARD_SUPPORT_BLUETOOTH   1
#define CONFIG_BOARD_SUPPORT_FM_RADIO    1
#define CONFIG_BOARD_SUPPORT_SDCARD      1
#define CONFIG_BOARD_SUPPORT_CAMERA      1
#define CONFIG_BOARD_SUPPORT_TORCH       1
#define CONFIG_BOARD_SUPPORT_AUDIO_JACK  1
#define CONFIG_BOARD_SUPPORT_VIBRATOR    0

#define CONFIG_HAS_BATTERY   1
#define CONFIG_HAS_RTC       1
#define CONFIG_HAS_BLUETOOTH 1
#define CONFIG_HAS_CELLULAR  1
#define CONFIG_HAS_SDCARD    1
#define CONFIG_HAS_AUDIO     1

/* Memory Pool */
#define CONFIG_LV_MEM_SIZE (512 * 1024U)

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_OBTEL_B10_BOARD_OBTEL_B10_H */
