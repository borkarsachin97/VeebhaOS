/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Hardware RTC & Calendar Driver Header for RDA8809
 */

#ifndef _HAL_CALENDAR_H_
#define _HAL_CALENDAR_H_

#include "cs_types.h"

// =============================================================================
//  CALENDAR REGISTER MAP (Base: 0x01A0A000)
// =============================================================================
#define REG_CALENDAR_BASE           0x01A0A000

typedef volatile struct
{
    REG32                          Ctrl;                         //0x00000000
    REG32                          Cmd;                          //0x00000004
    REG32                          Status;                       //0x00000008
    REG32                          Calendar_LoadVal_L;           //0x0000000C
    REG32                          Calendar_LoadVal_H;           //0x00000010
    REG32                          Calendar_CurVal_L;            //0x00000014
    REG32                          Calendar_CurVal_H;            //0x00000018
    REG32                          AlarmVal_L;                   //0x0000001C
    REG32                          AlarmVal_H;                   //0x00000020
} HWP_CALENDAR_T;

#define hwp_calendar                ((HWP_CALENDAR_T*) KSEG1(REG_CALENDAR_BASE))

// Ctrl Bits
#define CALENDAR_INTERVAL(n)        (((n)&3)<<0)
#define CALENDAR_INTERVAL_DISABLE   (0<<0)
#define CALENDAR_INTERVAL_PER_SEC   (1<<0)
#define CALENDAR_INTERVAL_PER_MIN   (2<<0)
#define CALENDAR_INTERVAL_PER_HOUR  (3<<0)

// Cmd Bits
#define CALENDAR_CALENDAR_LOAD      (1<<0)
#define CALENDAR_ALARM_LOAD         (1<<4)
#define CALENDAR_ALARM_ENABLE_SET   (1<<5)
#define CALENDAR_ALARM_ENABLE_CLR   (1<<6)
#define CALENDAR_ALARM_CLR          (1<<8)
#define CALENDAR_ITV_IRQ_CLR        (1<<9)
#define CALENDAR_ITV_IRQ_MASK_SET   (1<<16)
#define CALENDAR_ITV_IRQ_MASK_CLR   (1<<17)
#define CALENDAR_CALENDAR_NOT_VALID (1<<31)

// Status Bits
#define CALENDAR_ITV_IRQ_CAUSE      (1<<0)
#define CALENDAR_ALARM_IRQ_CAUSE    (1<<1)
#define CALENDAR_FORCE_WAKEUP       (1<<8)
#define CALENDAR_ITV_IRQ_STATUS     (1<<16)
#define CALENDAR_ALARM_ENABLE       (1<<20)
#define CALENDAR_CALENDAR_NOT_PROG  (1<<31)

// Load / Cur Val Bitfields
#define CALENDAR_SEC(n)             (((n)&0x3F)<<0)
#define CALENDAR_MIN(n)             (((n)&0x3F)<<8)
#define CALENDAR_HOUR(n)            (((n)&31)<<16)

#define CALENDAR_DAY(n)             (((n)&31)<<0)
#define CALENDAR_MON(n)             (((n)&15)<<8)
#define CALENDAR_YEAR(n)            (((n)&0x7F)<<16)
#define CALENDAR_WEEKDAY(n)         (((n)&7)<<24)

// =============================================================================
//  DATA STRUCTURES
// =============================================================================
typedef struct
{
    uint8_t sec;        /* 0..59 */
    uint8_t min;        /* 0..59 */
    uint8_t hour;       /* 0..23 */
    uint8_t day;        /* 1..31 */
    uint8_t month;      /* 1..12 */
    uint16_t year;      /* 2000..2127 (offset from 2000) */
    uint8_t wDay;       /* 0..6 (0=Sunday, 1=Monday... 6=Saturday) */
} hal_calendar_time_t;

// =============================================================================
//  PUBLIC RTC / CALENDAR API
// =============================================================================

/**
 * @brief Initialize the hardware Calendar/RTC peripheral.
 */
void hal_CalendarInit(void);

/**
 * @brief Check if the hardware calendar has been programmed.
 * @return true if valid/programmed, false otherwise.
 */
bool hal_CalendarIsValid(void);

/**
 * @brief Read the current date and time from the hardware calendar.
 * @param tm Pointer to structure to populate.
 * @return true on success.
 */
bool hal_CalendarGetTime(hal_calendar_time_t *tm);

/**
 * @brief Program a new date and time into the hardware calendar.
 * @param tm Pointer to desired date and time.
 * @return true on success.
 */
bool hal_CalendarSetTime(const hal_calendar_time_t *tm);

/**
 * @brief Format date and time to string (YYYY-MM-DD HH:MM:SS Day).
 */
int hal_CalendarFormat(char *buf, uint32_t size, const hal_calendar_time_t *tm);

/**
 * @brief Get human-readable weekday name (e.g. "Mon", "Tue").
 */
const char* hal_CalendarGetWeekdayName(uint8_t wDay);

#endif // _HAL_CALENDAR_H_
