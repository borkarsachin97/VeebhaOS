/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Hardware RTC & Calendar Driver Implementation for RDA8809
 */

#include "cs_types.h"
#include "global_macros.h"
#include "hal_calendar.h"
#include "sys_ctrl.h"
#include "timer.h"

extern void os_log_printf(const char *fmt, ...);

static const char * const g_weekday_names[7] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

void hal_CalendarInit(void)
{
    // Clear any pending interval interrupt
    hwp_calendar->Cmd = CALENDAR_ITV_IRQ_CLR;

    if (!hal_CalendarIsValid())
    {
        // Program default initial timestamp (2026-09-07 14:30:00 Monday)
        hal_calendar_time_t init_time = {
            .sec   = 0,
            .min   = 30,
            .hour  = 14,
            .day   = 7,
            .month = 9,
            .year  = 2026,
            .wDay  = 1
        };
        hal_CalendarSetTime(&init_time);
        os_log_printf("[RTC] Hardware Calendar initialized with default time: 2026-09-07 14:30:00 Mon\n");
    }
    else
    {
        hal_calendar_time_t cur_tm;
        if (hal_CalendarGetTime(&cur_tm))
        {
            char buf[32];
            hal_CalendarFormat(buf, sizeof(buf), &cur_tm);
            os_log_printf("[RTC] Hardware Calendar active. Current Time: %s\n", buf);
        }
    }
}

bool hal_CalendarIsValid(void)
{
    return (hwp_calendar->Status & CALENDAR_CALENDAR_NOT_PROG) == 0;
}

bool hal_CalendarGetTime(hal_calendar_time_t *tm)
{
    if (!tm) return false;

    // Read high and low registers
    uint32_t val_h = hwp_calendar->Calendar_CurVal_H;
    uint32_t val_l = hwp_calendar->Calendar_CurVal_L;

    // Guard against midnight roll-over during multi-register read
    if (val_h != hwp_calendar->Calendar_CurVal_H)
    {
        val_l = hwp_calendar->Calendar_CurVal_L;
        val_h = hwp_calendar->Calendar_CurVal_H;
    }

    tm->sec   = (uint8_t)((val_l >> 0)  & 0x3F);
    tm->min   = (uint8_t)((val_l >> 8)  & 0x3F);
    tm->hour  = (uint8_t)((val_l >> 16) & 0x1F);

    tm->day   = (uint8_t)((val_h >> 0)  & 0x1F);
    tm->month = (uint8_t)((val_h >> 8)  & 0x0F);
    uint8_t y_off = (uint8_t)((val_h >> 16) & 0x7F);
    tm->year  = (uint16_t)(2000 + y_off);
    tm->wDay  = (uint8_t)((val_h >> 24) & 0x07);
    if (tm->wDay > 6) tm->wDay = 0;

    return true;
}

bool hal_CalendarSetTime(const hal_calendar_time_t *tm)
{
    if (!tm) return false;

    uint8_t y_off = (tm->year >= 2000) ? (uint8_t)(tm->year - 2000) : (uint8_t)tm->year;
    uint8_t wday = (tm->wDay <= 6) ? tm->wDay : 0;

    uint32_t load_l = CALENDAR_SEC(tm->sec)  |
                      CALENDAR_MIN(tm->min)  |
                      CALENDAR_HOUR(tm->hour);

    uint32_t load_h = CALENDAR_DAY(tm->day)   |
                      CALENDAR_MON(tm->month) |
                      CALENDAR_YEAR(y_off)    |
                      CALENDAR_WEEKDAY(wday);

    hwp_calendar->Calendar_LoadVal_L = load_l;
    hwp_calendar->Calendar_LoadVal_H = load_h;

    // Trigger load
    hwp_calendar->Cmd = CALENDAR_CALENDAR_LOAD;

    // Wait for hardware to acknowledge programming
    uint32_t timeout = 100000;
    while ((hwp_calendar->Status & CALENDAR_CALENDAR_NOT_PROG) && --timeout);

    return (timeout > 0);
}

int hal_CalendarFormat(char *buf, uint32_t size, const hal_calendar_time_t *tm)
{
    if (!buf || size < 24 || !tm) return 0;

    const char *wname = hal_CalendarGetWeekdayName(tm->wDay);

    char tmp[32];
    uint32_t y = tm->year;
    uint8_t mo = tm->month;
    uint8_t d = tm->day;
    uint8_t h = tm->hour;
    uint8_t mi = tm->min;
    uint8_t s = tm->sec;

    tmp[0] = '0' + (y / 1000) % 10;
    tmp[1] = '0' + (y / 100) % 10;
    tmp[2] = '0' + (y / 10) % 10;
    tmp[3] = '0' + y % 10;
    tmp[4] = '-';
    tmp[5] = '0' + (mo / 10) % 10;
    tmp[6] = '0' + mo % 10;
    tmp[7] = '-';
    tmp[8] = '0' + (d / 10) % 10;
    tmp[9] = '0' + d % 10;
    tmp[10] = ' ';
    tmp[11] = '0' + (h / 10) % 10;
    tmp[12] = '0' + h % 10;
    tmp[13] = ':';
    tmp[14] = '0' + (mi / 10) % 10;
    tmp[15] = '0' + mi % 10;
    tmp[16] = ':';
    tmp[17] = '0' + (s / 10) % 10;
    tmp[18] = '0' + s % 10;
    tmp[19] = ' ';
    tmp[20] = wname[0];
    tmp[21] = wname[1];
    tmp[22] = wname[2];
    tmp[23] = '\0';

    uint32_t len = 0;
    while (tmp[len] && len < size - 1)
    {
        buf[len] = tmp[len];
        len++;
    }
    buf[len] = '\0';
    return (int)len;
}

const char* hal_CalendarGetWeekdayName(uint8_t wDay)
{
    if (wDay > 6) return "???";
    return g_weekday_names[wDay];
}
