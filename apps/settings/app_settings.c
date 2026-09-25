/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "apps/settings/app_settings.h"
#include "apps/settings/app_bt.h"
#include "apps/settings/app_tethering.h"
#include "apps/home/app_idle.h"
#include "sdk/core/wallpaper.h"
#include "sdk/vfs/os_vfs.h"
#include "sdk/storage/os_nvram.h"
#include "sdk/include/veebha_i18n.h"
#include "veebha_templates.h"
#include "veebha_win_mgr.h"
#include "veebha_status_bar.h"
#include "veebha_theme.h"
#include "boards/board_config.h"
#include "boards/board_info.h"
#include "sdk/include/veebha_hardware.h"
#include "apps/home/app_launcher.h"
#include "apps/tools/app_tools.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static sound_profile_t s_active_profile = SOUND_PROFILE_GENERAL;
static uint8_t s_backlight_timeout_sec = 30;
static uint8_t s_volume_level = 7;
static uint8_t s_clock_hours = 12;
static uint8_t s_clock_mins = 0;
static bool s_settings_initialized = false;

void app_settings_init(void)
{
    if (s_settings_initialized) return;
    s_settings_initialized = true;

    os_nvram_init();
    os_nvram_data_t *nv = os_nvram_get();
    if (nv) {
        if (nv->theme_id < THEME_COUNT) {
            theme_set_palette((os_theme_id_t)nv->theme_id);
        } else {
            theme_set_palette(THEME_DARK_CYAN);
        }
        veebha_i18n_init((language_id_t)nv->language_id);
        wallpaper_init();
        s_volume_level = (nv->volume_level >= 1 && nv->volume_level <= 7) ? nv->volume_level : 7;
        s_active_profile = (nv->active_profile < SOUND_PROFILE_COUNT) ? (sound_profile_t)nv->active_profile : SOUND_PROFILE_GENERAL;
        s_backlight_timeout_sec = nv->backlight_timeout ? nv->backlight_timeout : 30;
        if (nv->clock_hour < 24) s_clock_hours = nv->clock_hour;
        if (nv->clock_min < 60) s_clock_mins = nv->clock_min;
        status_bar_set_rtc_time(s_clock_hours, s_clock_mins);
        uint16_t cy = (nv->clock_year >= 2020 && nv->clock_year <= 2099) ? nv->clock_year : 2026;
        uint8_t cm = (nv->clock_month >= 1 && nv->clock_month <= 12) ? nv->clock_month : 9;
        uint8_t cd = (nv->clock_day >= 1 && nv->clock_day <= 31) ? nv->clock_day : 23;
        status_bar_set_rtc_date(cy, cm, cd);

        status_bar_set_silent(s_active_profile == SOUND_PROFILE_SILENT);
        status_bar_set_bt_state(nv->bt_enabled, false);
        status_bar_set_usb_mode(nv->usb_mode);
        status_bar_set_tethering(nv->tethering_usb || nv->tethering_bt);
        status_bar_set_alarm(nv->alarm_enabled);
    }
}

sound_profile_t app_settings_get_profile(void)
{
    return s_active_profile;
}

void app_settings_set_profile(sound_profile_t profile)
{
    if (profile >= SOUND_PROFILE_COUNT) profile = SOUND_PROFILE_GENERAL;
    s_active_profile = profile;

    status_bar_set_silent(s_active_profile == SOUND_PROFILE_SILENT);

    os_nvram_data_t *nv = os_nvram_get();
    if (nv) {
        nv->active_profile = (uint8_t)s_active_profile;
        os_nvram_save();
    }
    printf("[SETTINGS] Audio profile set to %d (%s)\n",
           (int)s_active_profile,
           s_active_profile == SOUND_PROFILE_SILENT ? "Silent" : (s_active_profile == SOUND_PROFILE_OUTDOOR ? "Outdoor" : "General"));
}

void app_settings_toggle_silent(void)
{
    if (s_active_profile == SOUND_PROFILE_SILENT) {
        app_settings_set_profile(SOUND_PROFILE_GENERAL);
    } else {
        app_settings_set_profile(SOUND_PROFILE_SILENT);
    }
}

bool app_settings_is_silent(void)
{
    return s_active_profile == SOUND_PROFILE_SILENT;
}

uint8_t app_settings_get_volume(void)
{
    return s_volume_level;
}

void app_settings_set_volume(uint8_t vol)
{
    if (vol < 1) vol = 1;
    if (vol > 7) vol = 7;
    s_volume_level = vol;

    os_nvram_data_t *nv = os_nvram_get();
    if (nv) {
        nv->volume_level = s_volume_level;
        os_nvram_save();
    }
}

/* Forward declarations */
static void open_display_settings(void);
static void open_theme_selection(void);
static void open_sound_settings(void);
static void open_sound_profiles(void);
static void open_connectivity_settings(void);
static void open_datetime_settings(void);
static void open_about_phone(void);

/* ============================================================================
 * 1. Display Settings & Themes
 * ============================================================================ */
static void on_theme_select(uint16_t index)
{
    if (index < THEME_COUNT) {
        theme_set_palette((os_theme_id_t)index);
        os_nvram_data_t *nv = os_nvram_get();
        if (nv) {
            nv->theme_id = (uint8_t)index;
            os_nvram_save();
        }
        printf("[SETTINGS] Theme switched to: %s\n", theme_get_name((os_theme_id_t)index));
    }
    win_mgr_pop();
    open_theme_selection();
}

static void open_theme_selection(void)
{
    static tpl_list_item_t s_theme_items[THEME_COUNT];
    static char s_theme_sub[THEME_COUNT][32];

    os_theme_id_t current = theme_get_palette();

    for (int i = 0; i < THEME_COUNT; i++) {
        s_theme_items[i].icon = LV_SYMBOL_IMAGE;
        s_theme_items[i].title = theme_get_name((os_theme_id_t)i);
        if (i == (int)current) {
            snprintf(s_theme_sub[i], sizeof(s_theme_sub[i]), "Active Theme");
        } else {
            snprintf(s_theme_sub[i], sizeof(s_theme_sub[i]), "Select to Apply");
        }
        s_theme_items[i].subtext = s_theme_sub[i];
    }

    tpl_list_view_t desc = {
        .title = "Themes",
        .items = s_theme_items,
        .count = THEME_COUNT,
        .on_select = on_theme_select,
        .on_back = NULL,
        .lsk_label = "Select",
        .rsk_label = "Back"
    };

    lv_obj_t *scr = tpl_list_create(&desc);
    if (scr) {
        win_mgr_push(scr, "Select", tpl_list_default_lsk, "Back", tpl_list_default_rsk);
    }
}

#define MAX_WP_SETTINGS_FILES 16
static char s_wp_filenames[MAX_WP_SETTINGS_FILES][32];
static char s_wp_subtexts[MAX_WP_SETTINGS_FILES + 1][32];
static tpl_list_item_t s_wp_list_items[MAX_WP_SETTINGS_FILES + 1];
static uint16_t s_wp_item_count = 0;

static void on_wallpaper_item_select(uint16_t index)
{
    if (index >= s_wp_item_count) return;

    os_nvram_data_t *nv = os_nvram_get();

    if (index == 0) {
        /* Theme Solid */
        wallpaper_set_solid(theme_get()->bg_color);
        if (nv) {
            nv->wallpaper_mode = (uint8_t)WALLPAPER_MODE_THEME_SOLID;
            nv->wallpaper_path[0] = '\0';
            os_nvram_save();
        }
        static tpl_dialog_desc_t dlg = {
            .title = "Wallpaper",
            .message = "Applied: Theme Solid",
            .icon = LV_SYMBOL_IMAGE,
            .on_confirm = NULL,
            .on_cancel = NULL,
            .lsk_label = "OK",
            .rsk_label = "OK"
        };
        tpl_dialog_show(&dlg);
    } else {
        char full_path[64];
        snprintf(full_path, sizeof(full_path), "/sdcard/Wallpapers/%s", s_wp_filenames[index - 1]);
        wallpaper_set_image(full_path);
        if (nv) {
            nv->wallpaper_mode = (uint8_t)WALLPAPER_MODE_IMAGE_BMP;
            strncpy(nv->wallpaper_path, full_path, sizeof(nv->wallpaper_path) - 1);
            nv->wallpaper_path[sizeof(nv->wallpaper_path) - 1] = '\0';
            os_nvram_save();
        }
        static char msg_buf[64];
        snprintf(msg_buf, sizeof(msg_buf), "Applied: %s", s_wp_filenames[index - 1]);
        static tpl_dialog_desc_t dlg = {
            .title = "Wallpaper",
            .message = msg_buf,
            .icon = LV_SYMBOL_IMAGE,
            .on_confirm = NULL,
            .on_cancel = NULL,
            .lsk_label = "OK",
            .rsk_label = "OK"
        };
        dlg.message = msg_buf;
        tpl_dialog_show(&dlg);
    }
}

static void open_wallpaper_settings(void)
{
    s_wp_item_count = 0;

    wallpaper_mode_t cur_mode = wallpaper_get_mode();
    const char *cur_path = wallpaper_get_current_path();

    /* Row 0: Theme Solid (Default) */
    s_wp_list_items[0].icon = LV_SYMBOL_IMAGE;
    s_wp_list_items[0].title = "Theme Solid (Default)";
    if (cur_mode == WALLPAPER_MODE_THEME_SOLID) {
        snprintf(s_wp_subtexts[0], sizeof(s_wp_subtexts[0]), "Active");
    } else {
        snprintf(s_wp_subtexts[0], sizeof(s_wp_subtexts[0]), "Solid Color");
    }
    s_wp_list_items[0].subtext = s_wp_subtexts[0];
    s_wp_item_count = 1;

    /* Scan /sdcard/Wallpapers */
    void *dir = NULL;
    if (vfs_opendir(WALLPAPER_DIR, &dir) && dir != NULL) {
        vfs_dirent_t ent;
        while (vfs_readdir(dir, &ent) && s_wp_item_count < (MAX_WP_SETTINGS_FILES + 1)) {
            if (ent.type == VFS_NODE_FILE) {
                uint16_t file_idx = s_wp_item_count - 1;
                strncpy(s_wp_filenames[file_idx], ent.name, sizeof(s_wp_filenames[file_idx]) - 1);
                s_wp_filenames[file_idx][sizeof(s_wp_filenames[file_idx]) - 1] = '\0';

                s_wp_list_items[s_wp_item_count].icon = LV_SYMBOL_IMAGE;
                s_wp_list_items[s_wp_item_count].title = s_wp_filenames[file_idx];

                char full_path[64];
                snprintf(full_path, sizeof(full_path), "/sdcard/Wallpapers/%s", ent.name);
                bool is_active = (cur_mode == WALLPAPER_MODE_IMAGE_BMP && strcasecmp(cur_path, full_path) == 0);

                if (is_active) {
                    snprintf(s_wp_subtexts[s_wp_item_count], sizeof(s_wp_subtexts[s_wp_item_count]), "Active");
                } else {
                    snprintf(s_wp_subtexts[s_wp_item_count], sizeof(s_wp_subtexts[s_wp_item_count]), "%u KB | BMP Image", (ent.size_bytes + 1023) / 1024);
                }
                s_wp_list_items[s_wp_item_count].subtext = s_wp_subtexts[s_wp_item_count];
                s_wp_item_count++;
            }
        }
        vfs_closedir(dir);
    }

    tpl_list_view_t desc = {
        .title = "Wallpaper",
        .items = s_wp_list_items,
        .count = s_wp_item_count,
        .on_select = on_wallpaper_item_select,
        .on_back = NULL,
        .lsk_label = "Apply",
        .rsk_label = "Back"
    };

    lv_obj_t *scr = tpl_list_create(&desc);
    if (scr) {
        win_mgr_push(scr, "Apply", tpl_list_default_lsk, "Back", tpl_list_default_rsk);
    }
}

static void on_display_select(uint16_t index)
{
    if (index == 0) {
        open_theme_selection();
    } else if (index == 1) {
        /* Backlight timeout step */
        if (s_backlight_timeout_sec == 10) s_backlight_timeout_sec = 30;
        else if (s_backlight_timeout_sec == 30) s_backlight_timeout_sec = 60;
        else s_backlight_timeout_sec = 10;

        os_nvram_data_t *nv = os_nvram_get();
        if (nv) {
            nv->backlight_timeout = s_backlight_timeout_sec;
            os_nvram_save();
        }

        static char msg_buf[64];
        snprintf(msg_buf, sizeof(msg_buf), "Timeout set to %u seconds", s_backlight_timeout_sec);
        static tpl_dialog_desc_t dlg = {
            .title = "Backlight Timeout",
            .message = msg_buf,
            .icon = LV_SYMBOL_REFRESH,
            .on_confirm = NULL,
            .on_cancel = NULL,
            .lsk_label = "OK",
            .rsk_label = "OK"
        };
        tpl_dialog_show(&dlg);
    } else if (index == 2) {
        open_wallpaper_settings();
    }
}

static void open_display_settings(void)
{
    static char theme_sub[32];
    snprintf(theme_sub, sizeof(theme_sub), "%s (Active)", theme_get_name(theme_get_palette()));

    static char bl_sub[32];
    snprintf(bl_sub, sizeof(bl_sub), "%u seconds", s_backlight_timeout_sec);

    static char wp_sub[32];
    if (wallpaper_get_mode() == WALLPAPER_MODE_THEME_SOLID) {
        snprintf(wp_sub, sizeof(wp_sub), "Theme Solid");
    } else {
        const char *cur = wallpaper_get_current_path();
        const char *slash = strrchr(cur, '/');
        snprintf(wp_sub, sizeof(wp_sub), "%s", slash ? (slash + 1) : cur);
    }

    static tpl_list_item_t s_disp_items[3];
    s_disp_items[0].icon = LV_SYMBOL_IMAGE;
    s_disp_items[0].title = veebha_i18n_str(STR_THEME);
    s_disp_items[0].subtext = theme_sub;

    s_disp_items[1].icon = LV_SYMBOL_REFRESH;
    s_disp_items[1].title = "Backlight Timeout";
    s_disp_items[1].subtext = bl_sub;

    s_disp_items[2].icon = LV_SYMBOL_IMAGE;
    s_disp_items[2].title = veebha_i18n_str(STR_WALLPAPER);
    s_disp_items[2].subtext = wp_sub;

    tpl_list_view_t desc = {
        .title = veebha_i18n_str(STR_DISPLAY),
        .items = s_disp_items,
        .count = 3,
        .on_select = on_display_select,
        .on_back = NULL,
        .lsk_label = veebha_i18n_str(STR_SELECT),
        .rsk_label = veebha_i18n_str(STR_BACK)
    };

    lv_obj_t *scr = tpl_list_create(&desc);
    if (scr) {
        win_mgr_push(scr, veebha_i18n_str(STR_SELECT), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
    }
}

/* ============================================================================
 * 2. Sound Settings & Profiles
 * ============================================================================ */
static void on_profile_select(uint16_t index)
{
    if (index == 0) {
        app_settings_set_profile(SOUND_PROFILE_GENERAL);
    } else if (index == 1) {
        app_settings_set_profile(SOUND_PROFILE_SILENT);
    } else if (index == 2) {
        app_settings_set_profile(SOUND_PROFILE_OUTDOOR);
    }

    const char *pnames[] = { "General", "Silent", "Outdoor" };
    static char msg_buf[64];
    snprintf(msg_buf, sizeof(msg_buf), "Profile '%s' activated", pnames[s_active_profile % 3]);
    static tpl_dialog_desc_t dlg = {
        .title = "Sound Profile",
        .message = msg_buf,
        .icon = LV_SYMBOL_AUDIO,
        .on_confirm = NULL,
        .on_cancel = NULL,
        .lsk_label = "OK",
        .rsk_label = "OK"
    };
    tpl_dialog_show(&dlg);
}

static void open_sound_profiles(void)
{
    static tpl_list_item_t s_prof_items[3];
    s_prof_items[0].icon = LV_SYMBOL_AUDIO;
    s_prof_items[0].title = "General";
    s_prof_items[0].subtext = (s_active_profile == SOUND_PROFILE_GENERAL) ? "Active Profile" : "Standard volume & rings";

    s_prof_items[1].icon = LV_SYMBOL_MUTE;
    s_prof_items[1].title = "Silent";
    s_prof_items[1].subtext = (s_active_profile == SOUND_PROFILE_SILENT) ? "Active Profile" : "Mute ringtones & alerts";

    s_prof_items[2].icon = LV_SYMBOL_VOLUME_MAX;
    s_prof_items[2].title = "Outdoor";
    s_prof_items[2].subtext = (s_active_profile == SOUND_PROFILE_OUTDOOR) ? "Active Profile" : "Maximum ring volume";

    tpl_list_view_t desc = {
        .title = "Profiles",
        .items = s_prof_items,
        .count = 3,
        .on_select = on_profile_select,
        .on_back = NULL,
        .lsk_label = "Select",
        .rsk_label = "Back"
    };

    lv_obj_t *scr = tpl_list_create(&desc);
    if (scr) {
        win_mgr_push(scr, "Select", tpl_list_default_lsk, "Back", tpl_list_default_rsk);
    }
}

static void on_sound_select(uint16_t index)
{
    if (index == 0) {
        open_sound_profiles();
    } else if (index == 1) {
        /* Adjust Volume */
        uint8_t new_vol = (s_volume_level >= 7) ? 1 : (s_volume_level + 1);
        app_settings_set_volume(new_vol);

        static char vol_buf[64];
        snprintf(vol_buf, sizeof(vol_buf), "Volume set to Level %u / 7", s_volume_level);
        static tpl_dialog_desc_t dlg = {
            .title = "Volume",
            .message = vol_buf,
            .icon = LV_SYMBOL_VOLUME_MAX,
            .on_confirm = NULL,
            .on_cancel = NULL,
            .lsk_label = "OK",
            .rsk_label = "OK"
        };
        tpl_dialog_show(&dlg);
        printf("[SETTINGS] Volume set to Level %u\n", s_volume_level);
    }
}

static void open_sound_settings(void)
{
    static char prof_sub[32];
    const char *pnames[] = { "General", "Silent", "Outdoor" };
    snprintf(prof_sub, sizeof(prof_sub), "Active: %s", pnames[s_active_profile % 3]);

    static char vol_sub[32];
    snprintf(vol_sub, sizeof(vol_sub), "Level %u of 7", s_volume_level);

    static tpl_list_item_t s_sound_items[2];
    s_sound_items[0].icon = LV_SYMBOL_AUDIO;
    s_sound_items[0].title = "Profiles";
    s_sound_items[0].subtext = prof_sub;

    s_sound_items[1].icon = LV_SYMBOL_VOLUME_MAX;
    s_sound_items[1].title = "Volume";
    s_sound_items[1].subtext = vol_sub;

    tpl_list_view_t desc = {
        .title = veebha_i18n_str(STR_AUDIO_PROFILES),
        .items = s_sound_items,
        .count = 2,
        .on_select = on_sound_select,
        .on_back = NULL,
        .lsk_label = veebha_i18n_str(STR_SELECT),
        .rsk_label = veebha_i18n_str(STR_BACK)
    };

    lv_obj_t *scr = tpl_list_create(&desc);
    if (scr) {
        win_mgr_push(scr, veebha_i18n_str(STR_SELECT), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
    }
}

/* ============================================================================
 * 3. Date & Time Settings & Set Time Screen
 * ============================================================================ */

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;

    uint8_t            hour;    /* 1..12 */
    uint8_t            minute;  /* 0..59 */
    bool               is_pm;

    uint8_t            focus_col; /* 0: hour, 1: minute, 2: am_pm */

    lv_obj_t          *hour_lbl;
    lv_obj_t          *minute_lbl;
    lv_obj_t          *ampm_lbl;
} time_set_screen_data_t;

static void update_time_picker_ui(time_set_screen_data_t *data)
{
    if (!data) return;

    char hbuf[8], mbuf[8];
    snprintf(hbuf, sizeof(hbuf), "%02u", (unsigned int)data->hour);
    snprintf(mbuf, sizeof(mbuf), "%02u", (unsigned int)data->minute);

    lv_label_set_text(data->hour_lbl, hbuf);
    lv_label_set_text(data->minute_lbl, mbuf);
    lv_label_set_text(data->ampm_lbl, data->is_pm ? "PM" : "AM");

    /* Focus highlight on active column */
    lv_obj_set_style_text_color(data->hour_lbl, (data->focus_col == 0) ? theme_get()->accent : theme_get()->text_primary, 0);
    lv_obj_set_style_text_color(data->minute_lbl, (data->focus_col == 1) ? theme_get()->accent : theme_get()->text_primary, 0);
    lv_obj_set_style_text_color(data->ampm_lbl, (data->focus_col == 2) ? theme_get()->accent : theme_get()->text_primary, 0);
}

static void on_time_picker_save(void)
{
    lv_obj_t *top = lv_scr_act();
    time_set_screen_data_t *data = (time_set_screen_data_t *)lv_obj_get_user_data(top);
    if (!data) return;

    uint8_t h24 = (data->hour % 12) + (data->is_pm ? 12 : 0);
    s_clock_hours = h24;
    s_clock_mins = data->minute;

    status_bar_set_rtc_time(s_clock_hours, s_clock_mins);
    veebha_hw_rtc_set_time(s_clock_hours, s_clock_mins, 0);

    os_nvram_data_t *nv = os_nvram_get();
    if (nv) {
        nv->clock_hour = s_clock_hours;
        nv->clock_min = s_clock_mins;
        os_nvram_save();
    }

    printf("[SETTINGS] Time saved: %02u:%02u %s (%02u:%02u 24-Hour)\n",
           data->hour, data->minute, data->is_pm ? "PM" : "AM", s_clock_hours, s_clock_mins);

    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    win_mgr_pop();
}

static void on_time_picker_back(void)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    win_mgr_pop();
}

static void on_time_picker_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    lv_obj_t *scr = lv_obj_get_screen(lv_event_get_target(e));
    time_set_screen_data_t *data = (time_set_screen_data_t *)lv_obj_get_user_data(scr);
    if (!data) return;

    if (key == LV_KEY_RIGHT || key == '6') {
        data->focus_col = (data->focus_col + 1) % 3;
        update_time_picker_ui(data);
    } else if (key == LV_KEY_LEFT || key == '4') {
        data->focus_col = (data->focus_col + 2) % 3;
        update_time_picker_ui(data);
    } else if (key == LV_KEY_UP || key == LV_KEY_PREV || key == '2') {
        if (data->focus_col == 0) {
            data->hour = (data->hour % 12) + 1;
        } else if (data->focus_col == 1) {
            data->minute = (data->minute + 1) % 60;
        } else {
            data->is_pm = !data->is_pm;
        }
        update_time_picker_ui(data);
    } else if (key == LV_KEY_DOWN || key == LV_KEY_NEXT || key == '8') {
        if (data->focus_col == 0) {
            data->hour = (data->hour <= 1) ? 12 : (data->hour - 1);
        } else if (data->focus_col == 1) {
            data->minute = (data->minute == 0) ? 59 : (data->minute - 1);
        } else {
            data->is_pm = !data->is_pm;
        }
        update_time_picker_ui(data);
    } else if (key >= '0' && key <= '9') {
        uint8_t d = (uint8_t)(key - '0');
        if (data->focus_col == 0) {
            if (d >= 1 && d <= 12) data->hour = d;
            else if (d == 0) data->hour = 12;
        } else if (data->focus_col == 1) {
            data->minute = (data->minute * 10 + d) % 60;
        }
        update_time_picker_ui(data);
    }
}

static void on_time_set_delete_cb(lv_event_t *e)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    lv_obj_t *scr = lv_event_get_target(e);
    time_set_screen_data_t *data = (time_set_screen_data_t *)lv_obj_get_user_data(scr);
    if (data) {
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

static lv_obj_t * app_time_set_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_set_style_bg_color(screen, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    time_set_screen_data_t *data = (time_set_screen_data_t *)calloc(1, sizeof(time_set_screen_data_t));
    if (!data) {
        lv_obj_del(screen);
        return NULL;
    }

    data->view_type = VEEBHA_VIEW_TYPE_GENERIC;
    strncpy(data->title, "Set Time", sizeof(data->title) - 1);

    uint8_t cur_h = 12, cur_m = 0;
    status_bar_get_rtc_time(&cur_h, &cur_m);
    data->hour = (cur_h % 12 == 0) ? 12 : (cur_h % 12);
    data->minute = cur_m;
    data->is_pm = (cur_h >= 12);
    data->focus_col = 0;

    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_time_set_delete_cb, LV_EVENT_DELETE, NULL);

    /* 1. Zone A: Fixed 18px Top Status Bar */
    status_bar_create(screen, NULL);

    /* 2. Header Strip (18px) */
    lv_obj_t *hdr = lv_obj_create(screen);
    lv_obj_set_size(hdr, lv_pct(100), 18);
    lv_obj_set_style_bg_color(hdr, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_color_t hdr_border = (theme_get_palette() == THEME_HIGH_CONTRAST_BW)
                            ? lv_color_hex(0xFFFFFF)
                            : (theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x282C35));
    lv_obj_set_style_border_color(hdr, hdr_border, 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hlbl = lv_label_create(hdr);
    lv_label_set_text(hlbl, "SET TIME");
    lv_obj_set_style_text_color(hlbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(hlbl, &lv_font_montserrat_12, 0);

    /* 3. Viewport */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 8, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    /* Time Picker Card */
    lv_obj_t *card = lv_button_create(content);
    data->first_item = card;
    lv_obj_set_size(card, lv_pct(100), 56);
    lv_obj_set_style_bg_color(card, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_color_t card_border = (theme_get_palette() == THEME_HIGH_CONTRAST_BW)
                             ? lv_color_hex(0xFFFFFF)
                             : (theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x30363D));
    lv_obj_set_style_border_color(card, card_border, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_pad_all(card, 4, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_border_color(card, theme_get()->accent, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(card, 2, LV_STATE_FOCUSED);

    data->hour_lbl = lv_label_create(card);
    char hbuf[8];
    snprintf(hbuf, sizeof(hbuf), "%02u", (unsigned int)data->hour);
    lv_label_set_text(data->hour_lbl, hbuf);
    lv_obj_set_style_text_font(data->hour_lbl, &lv_font_montserrat_16, 0);

    lv_obj_t *colon = lv_label_create(card);
    lv_label_set_text(colon, ":");
    lv_obj_set_style_text_font(colon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(colon, theme_get()->text_primary, 0);

    data->minute_lbl = lv_label_create(card);
    char mbuf[8];
    snprintf(mbuf, sizeof(mbuf), "%02u", (unsigned int)data->minute);
    lv_label_set_text(data->minute_lbl, mbuf);
    lv_obj_set_style_text_font(data->minute_lbl, &lv_font_montserrat_16, 0);

    lv_obj_t *sp = lv_label_create(card);
    lv_label_set_text(sp, " ");

    data->ampm_lbl = lv_label_create(card);
    lv_label_set_text(data->ampm_lbl, data->is_pm ? "PM" : "AM");
    lv_obj_set_style_text_font(data->ampm_lbl, &lv_font_montserrat_14, 0);

    lv_obj_add_event_cb(card, on_time_picker_key_cb, LV_EVENT_KEY, NULL);

    lv_group_t *grp = win_mgr_get_group();
    if (grp) {
        lv_group_add_obj(grp, card);
        lv_group_focus_obj(card);
    }

    /* Hint Label */
    lv_obj_t *hint = lv_label_create(content);
    lv_label_set_text(hint, "Left/Right: Select\nUp/Down: Adjust");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);

    /* 4. Bottom Softkey Bar */
    data->softkey_bar = softkey_bar_create(screen, "Save", "Back");
    softkey_set_actions("Save", on_time_picker_save, "Back", on_time_picker_back);

    update_time_picker_ui(data);
    return screen;
}

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;

    uint16_t           year;      /* 2020..2099 */
    uint8_t            month;     /* 1..12 */
    uint8_t            day;       /* 1..31 */

    uint8_t            focus_col; /* 0: day, 1: month, 2: year */

    lv_obj_t          *day_lbl;
    lv_obj_t          *month_lbl;
    lv_obj_t          *year_lbl;
} date_set_screen_data_t;

static const char *s_settings_month_names[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static bool is_leap_year_val(uint16_t year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static uint8_t get_days_in_month_val(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2 && is_leap_year_val(year)) return 29;
    if (month >= 1 && month <= 12) return days[month - 1];
    return 30;
}

static void update_date_picker_ui(date_set_screen_data_t *data)
{
    if (!data) return;

    uint8_t max_d = get_days_in_month_val(data->year, data->month);
    if (data->day > max_d) data->day = max_d;
    if (data->day < 1) data->day = 1;

    char dbuf[8], mbuf[8], ybuf[8];
    snprintf(dbuf, sizeof(dbuf), "%02u", (unsigned int)data->day);
    const char *mname = (data->month >= 1 && data->month <= 12) ? s_settings_month_names[data->month - 1] : "Sep";
    snprintf(mbuf, sizeof(mbuf), "%s", mname);
    snprintf(ybuf, sizeof(ybuf), "%04u", (unsigned int)data->year);

    lv_label_set_text(data->day_lbl, dbuf);
    lv_label_set_text(data->month_lbl, mbuf);
    lv_label_set_text(data->year_lbl, ybuf);

    /* Focus highlight on active column */
    lv_obj_set_style_text_color(data->day_lbl, (data->focus_col == 0) ? theme_get()->accent : theme_get()->text_primary, 0);
    lv_obj_set_style_text_color(data->month_lbl, (data->focus_col == 1) ? theme_get()->accent : theme_get()->text_primary, 0);
    lv_obj_set_style_text_color(data->year_lbl, (data->focus_col == 2) ? theme_get()->accent : theme_get()->text_primary, 0);
}

static void on_date_picker_save(void)
{
    lv_obj_t *top = lv_scr_act();
    date_set_screen_data_t *data = (date_set_screen_data_t *)lv_obj_get_user_data(top);
    if (!data) return;

    uint8_t max_d = get_days_in_month_val(data->year, data->month);
    if (data->day > max_d) data->day = max_d;
    if (data->day < 1) data->day = 1;

    status_bar_set_rtc_date(data->year, data->month, data->day);
    veebha_hw_rtc_set_date(data->year, data->month, data->day);

    os_nvram_data_t *nv = os_nvram_get();
    if (nv) {
        nv->clock_year = data->year;
        nv->clock_month = data->month;
        nv->clock_day = data->day;
        os_nvram_save();
    }

    printf("[SETTINGS] Date saved: %04u-%02u-%02u\n",
           (unsigned int)data->year, (unsigned int)data->month, (unsigned int)data->day);

    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    win_mgr_pop();
}

static void on_date_picker_back(void)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    win_mgr_pop();
}

static void on_date_picker_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    lv_obj_t *scr = lv_obj_get_screen(lv_event_get_target(e));
    date_set_screen_data_t *data = (date_set_screen_data_t *)lv_obj_get_user_data(scr);
    if (!data) return;

    uint8_t max_days = get_days_in_month_val(data->year, data->month);

    if (key == LV_KEY_RIGHT || key == '6') {
        data->focus_col = (data->focus_col + 1) % 3;
        update_date_picker_ui(data);
    } else if (key == LV_KEY_LEFT || key == '4') {
        data->focus_col = (data->focus_col + 2) % 3;
        update_date_picker_ui(data);
    } else if (key == LV_KEY_UP || key == LV_KEY_PREV || key == '2') {
        if (data->focus_col == 0) {
            data->day = (data->day >= max_days) ? 1 : (data->day + 1);
        } else if (data->focus_col == 1) {
            data->month = (data->month >= 12) ? 1 : (data->month + 1);
        } else {
            data->year = (data->year >= 2099) ? 2020 : (data->year + 1);
        }
        update_date_picker_ui(data);
    } else if (key == LV_KEY_DOWN || key == LV_KEY_NEXT || key == '8') {
        if (data->focus_col == 0) {
            data->day = (data->day <= 1) ? max_days : (data->day - 1);
        } else if (data->focus_col == 1) {
            data->month = (data->month <= 1) ? 12 : (data->month - 1);
        } else {
            data->year = (data->year <= 2020) ? 2099 : (data->year - 1);
        }
        update_date_picker_ui(data);
    } else if (key >= '0' && key <= '9') {
        uint8_t d = (uint8_t)(key - '0');
        if (data->focus_col == 0) {
            uint8_t new_d = (data->day * 10 + d) % 100;
            if (new_d >= 1 && new_d <= max_days) data->day = new_d;
            else if (d >= 1 && d <= max_days) data->day = d;
        } else if (data->focus_col == 1) {
            uint8_t new_m = (data->month * 10 + d) % 100;
            if (new_m >= 1 && new_m <= 12) data->month = new_m;
            else if (d >= 1 && d <= 12) data->month = d;
        } else {
            uint32_t new_y = (data->year * 10 + d) % 10000;
            if (new_y >= 2020 && new_y <= 2099) data->year = (uint16_t)new_y;
        }
        update_date_picker_ui(data);
    }
}

static void on_date_set_delete_cb(lv_event_t *e)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    lv_obj_t *scr = lv_event_get_target(e);
    date_set_screen_data_t *data = (date_set_screen_data_t *)lv_obj_get_user_data(scr);
    if (data) {
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

static lv_obj_t * app_date_set_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_set_style_bg_color(screen, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    date_set_screen_data_t *data = (date_set_screen_data_t *)calloc(1, sizeof(date_set_screen_data_t));
    if (!data) {
        lv_obj_del(screen);
        return NULL;
    }

    data->view_type = VEEBHA_VIEW_TYPE_GENERIC;
    strncpy(data->title, "Set Date", sizeof(data->title) - 1);

    uint16_t cur_y = 2026;
    uint8_t cur_m = 9, cur_d = 23;
    status_bar_get_rtc_date(&cur_y, &cur_m, &cur_d);
    data->year = cur_y;
    data->month = cur_m;
    data->day = cur_d;
    data->focus_col = 0;

    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_date_set_delete_cb, LV_EVENT_DELETE, NULL);

    /* 1. Zone A: Fixed 18px Top Status Bar */
    status_bar_create(screen, NULL);

    /* 2. Header Strip (18px) */
    lv_obj_t *hdr = lv_obj_create(screen);
    lv_obj_set_size(hdr, lv_pct(100), 18);
    lv_obj_set_style_bg_color(hdr, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_color_t hdr_border = (theme_get_palette() == THEME_HIGH_CONTRAST_BW)
                            ? lv_color_hex(0xFFFFFF)
                            : (theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x282C35));
    lv_obj_set_style_border_color(hdr, hdr_border, 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hlbl = lv_label_create(hdr);
    lv_label_set_text(hlbl, "SET DATE");
    lv_obj_set_style_text_color(hlbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(hlbl, &lv_font_montserrat_12, 0);

    /* 3. Viewport */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 8, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    /* Date Picker Card */
    lv_obj_t *card = lv_button_create(content);
    data->first_item = card;
    lv_obj_set_size(card, lv_pct(100), 56);
    lv_obj_set_style_bg_color(card, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_color_t card_border = (theme_get_palette() == THEME_HIGH_CONTRAST_BW)
                             ? lv_color_hex(0xFFFFFF)
                             : (theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x30363D));
    lv_obj_set_style_border_color(card, card_border, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_pad_all(card, 4, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_border_color(card, theme_get()->accent, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(card, 2, LV_STATE_FOCUSED);

    /* Day */
    data->day_lbl = lv_label_create(card);
    char dbuf[8];
    snprintf(dbuf, sizeof(dbuf), "%02u", (unsigned int)data->day);
    lv_label_set_text(data->day_lbl, dbuf);
    lv_obj_set_style_text_font(data->day_lbl, &lv_font_montserrat_16, 0);

    lv_obj_t *dash1 = lv_label_create(card);
    lv_label_set_text(dash1, " - ");
    lv_obj_set_style_text_font(dash1, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(dash1, theme_get()->text_muted, 0);

    /* Month */
    data->month_lbl = lv_label_create(card);
    const char *mname = (data->month >= 1 && data->month <= 12) ? s_settings_month_names[data->month - 1] : "Sep";
    lv_label_set_text(data->month_lbl, mname);
    lv_obj_set_style_text_font(data->month_lbl, &lv_font_montserrat_16, 0);

    lv_obj_t *dash2 = lv_label_create(card);
    lv_label_set_text(dash2, " - ");
    lv_obj_set_style_text_font(dash2, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(dash2, theme_get()->text_muted, 0);

    /* Year */
    data->year_lbl = lv_label_create(card);
    char ybuf[8];
    snprintf(ybuf, sizeof(ybuf), "%04u", (unsigned int)data->year);
    lv_label_set_text(data->year_lbl, ybuf);
    lv_obj_set_style_text_font(data->year_lbl, &lv_font_montserrat_16, 0);

    lv_obj_add_event_cb(card, on_date_picker_key_cb, LV_EVENT_KEY, NULL);

    lv_group_t *grp = win_mgr_get_group();
    if (grp) {
        lv_group_add_obj(grp, card);
        lv_group_focus_obj(card);
    }

    /* Hint Label */
    lv_obj_t *hint = lv_label_create(content);
    lv_label_set_text(hint, "Left/Right: Select\nUp/Down: Adjust");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);

    /* 4. Bottom Softkey Bar */
    data->softkey_bar = softkey_bar_create(screen, "Save", "Back");
    softkey_set_actions("Save", on_date_picker_save, "Back", on_date_picker_back);

    update_date_picker_ui(data);
    return screen;
}

static void open_set_time_dialog(void)
{
    lv_obj_t *scr = app_time_set_create();
    if (scr) {
        win_mgr_push(scr, "Save", on_time_picker_save, "Back", on_time_picker_back);
        lv_group_t *g = win_mgr_get_group();
        if (g) {
            time_set_screen_data_t *d = (time_set_screen_data_t *)lv_obj_get_user_data(scr);
            if (d && d->first_item) {
                lv_group_focus_obj(d->first_item);
            }
            lv_group_set_editing(g, true);
        }
    }
}

static void open_set_date_dialog(void)
{
    lv_obj_t *scr = app_date_set_create();
    if (scr) {
        win_mgr_push(scr, "Save", on_date_picker_save, "Back", on_date_picker_back);
        lv_group_t *g = win_mgr_get_group();
        if (g) {
            date_set_screen_data_t *d = (date_set_screen_data_t *)lv_obj_get_user_data(scr);
            if (d && d->first_item) {
                lv_group_focus_obj(d->first_item);
            }
            lv_group_set_editing(g, true);
        }
    }
}

static void on_datetime_select(uint16_t index)
{
    if (index == 0) {
        open_set_time_dialog();
    } else if (index == 1) {
        open_set_date_dialog();
    }
}

static void open_datetime_settings(void)
{
    static char clock_sub[32];
    uint8_t h = 12, m = 0;
    status_bar_get_rtc_time(&h, &m);
    uint8_t h12 = (h % 12 == 0) ? 12 : (h % 12);
    snprintf(clock_sub, sizeof(clock_sub), "%02u:%02u %s (%02u:%02u)", h12, m, (h >= 12) ? "PM" : "AM", h, m);

    static char date_sub[32];
    uint16_t y = 2026;
    uint8_t mo = 9, d = 23;
    status_bar_get_rtc_date(&y, &mo, &d);
    static const char *dow_names[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char *mon_names[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    int calc_y = (int)y;
    if (mo < 3) calc_y -= 1;
    int dow = (calc_y + calc_y / 4 - calc_y / 100 + calc_y / 400 + t[(mo >= 1 && mo <= 12) ? (mo - 1) : 0] + d) % 7;
    if (dow < 0 || dow > 6) dow = 0;
    const char *mstr = (mo >= 1 && mo <= 12) ? mon_names[mo - 1] : "Sep";
    snprintf(date_sub, sizeof(date_sub), "%02u %s %04u (%s)", (unsigned int)d, mstr, (unsigned int)y, dow_names[dow]);

    static tpl_list_item_t s_dt_items[3];
    s_dt_items[0].icon = LV_SYMBOL_BELL;
    s_dt_items[0].title = "Set Time";
    s_dt_items[0].subtext = clock_sub;

    s_dt_items[1].icon = LV_SYMBOL_DIRECTORY;
    s_dt_items[1].title = "Set Date";
    s_dt_items[1].subtext = date_sub;

    s_dt_items[2].icon = LV_SYMBOL_SETTINGS;
    s_dt_items[2].title = "Time Format";
    s_dt_items[2].subtext = "12-Hour / 24-Hour";

    tpl_list_view_t desc = {
        .title = veebha_i18n_str(STR_DATE_TIME),
        .items = s_dt_items,
        .count = 3,
        .on_select = on_datetime_select,
        .on_back = NULL,
        .lsk_label = veebha_i18n_str(STR_SELECT),
        .rsk_label = veebha_i18n_str(STR_BACK)
    };

    lv_obj_t *scr = tpl_list_create(&desc);
    if (scr) {
        win_mgr_push(scr, veebha_i18n_str(STR_SELECT), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
    }
}

/* ============================================================================
 * 4. About Phone Info Screen
 * ============================================================================ */
static void open_about_phone(void)
{
    const board_info_t *info = board_get_info();

    static char s_device_str[64];
    static char s_cpu_str[64];
    static char s_arch_str[64];
    static char s_disp_str[64];
    static char s_gpu_str[64];
    static char s_ram_str[64];
    static char s_flash_str[64];
    static char s_ver_str[64];
    static char s_build_str[64];

    if (info) {
        snprintf(s_device_str, sizeof(s_device_str), "%s %s", info->brand, info->model);
        snprintf(s_cpu_str, sizeof(s_cpu_str), "%s", info->cpu_name);
        snprintf(s_arch_str, sizeof(s_arch_str), "%s", info->arch);
        snprintf(s_disp_str, sizeof(s_disp_str), "%ux%u %s", info->display_width, info->display_height, info->color_format ? info->color_format : "RGB565");
        snprintf(s_gpu_str, sizeof(s_gpu_str), "%s / %s", info->display_controller ? info->display_controller : "LCD", info->gpu_blitter ? info->gpu_blitter : "CPU");
        snprintf(s_ram_str, sizeof(s_ram_str), "%u KB Total", info->ram_size_kb);
        snprintf(s_flash_str, sizeof(s_flash_str), "%u KB Flash", info->flash_size_kb);
        snprintf(s_ver_str, sizeof(s_ver_str), "VeebhaOS v%s", info->os_version);
        snprintf(s_build_str, sizeof(s_build_str), "%s", info->build_timestamp ? info->build_timestamp : "");
    }


    static char s_about_lines[9][64];
    snprintf(s_about_lines[0], sizeof(s_about_lines[0]), "Device: %s", s_device_str);
    snprintf(s_about_lines[1], sizeof(s_about_lines[1]), "Version: %s", s_ver_str);
    snprintf(s_about_lines[2], sizeof(s_about_lines[2]), "CPU: %s", s_cpu_str);
    snprintf(s_about_lines[3], sizeof(s_about_lines[3]), "Arch: %s", s_arch_str);
    snprintf(s_about_lines[4], sizeof(s_about_lines[4]), "Display: %s", s_disp_str);
    snprintf(s_about_lines[5], sizeof(s_about_lines[5]), "GPU: %s", s_gpu_str);
    snprintf(s_about_lines[6], sizeof(s_about_lines[6]), "RAM: %s", s_ram_str);
    snprintf(s_about_lines[7], sizeof(s_about_lines[7]), "Flash: %s", s_flash_str);
    snprintf(s_about_lines[8], sizeof(s_about_lines[8]), "Build: %s", s_build_str);

    static tpl_list_item_t s_about_items[9];
    for (int i = 0; i < 9; i++) {
        s_about_items[i] = (tpl_list_item_t){ .icon = NULL, .title = s_about_lines[i], .subtext = NULL };
    }

    tpl_list_view_t desc = {
        .title = veebha_i18n_str(STR_ABOUT),
        .items = s_about_items,
        .count = sizeof(s_about_items) / sizeof(s_about_items[0]),
        .on_select = NULL,
        .on_back = NULL,
        .lsk_label = veebha_i18n_str(STR_OK),
        .rsk_label = veebha_i18n_str(STR_BACK)
    };

    lv_obj_t *scr = tpl_list_create(&desc);
    if (scr) {
        win_mgr_push(scr, veebha_i18n_str(STR_OK), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
    }
}

/* ============================================================================
 * Connectivity Settings
 * ============================================================================ */
static void on_connectivity_select(uint16_t index)
{
    if (index == 0) {
        app_bt_open();
    } else if (index == 1) {
        app_tethering_open();
    } else if (index == 2) {
        bool cur = veebha_hw_usb_console_is_enabled();
        veebha_hw_usb_console_set_enabled(!cur);
        win_mgr_pop();
        open_connectivity_settings();
    }
}

static void open_connectivity_settings(void)
{
    bool usb_log_on = veebha_hw_usb_console_is_enabled();
    static tpl_list_item_t s_conn_items[3];
    s_conn_items[0] = (tpl_list_item_t){ .icon = LV_SYMBOL_BLUETOOTH, .title = "Bluetooth",   .subtext = "Devices, Pairing" };
    s_conn_items[1] = (tpl_list_item_t){ .icon = LV_SYMBOL_WIFI,      .title = "Tethering",   .subtext = "USB & BT Hotspot" };
    s_conn_items[2] = (tpl_list_item_t){ .icon = LV_SYMBOL_USB,       .title = "USB Console", .subtext = usb_log_on ? "Status: Enabled" : "Status: Disabled" };

    tpl_list_view_t desc = {
        .title = veebha_i18n_str(STR_CONNECTIVITY),
        .items = s_conn_items,
        .count = 3,
        .on_select = on_connectivity_select,
        .on_back = NULL,
        .lsk_label = veebha_i18n_str(STR_SELECT),
        .rsk_label = veebha_i18n_str(STR_BACK)
    };

    lv_obj_t *scr = tpl_list_create(&desc);
    if (scr) {
        win_mgr_push(scr, veebha_i18n_str(STR_SELECT), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
    }
}

/* ============================================================================
 * Language Settings
 * ============================================================================ */
static void on_language_select(uint16_t index)
{
    if (index < LANG_COUNT) {
        veebha_i18n_set_language((language_id_t)index);
        app_launcher_invalidate();
        os_nvram_data_t *nv = os_nvram_get();
        if (nv) {
            nv->language_id = (uint8_t)index;
            os_nvram_save();
        }
        win_mgr_pop();
    }
}

static void open_language_settings(void)
{
    static const tpl_list_item_t s_lang_items[] = {
        { .icon = LV_SYMBOL_EDIT, .title = "English", .subtext = "Default" },
        { .icon = LV_SYMBOL_EDIT, .title = "हिन्दी",    .subtext = "Hindi" },
        { .icon = LV_SYMBOL_EDIT, .title = "Русский",  .subtext = "Russian" },
    };

    tpl_list_view_t desc = {
        .title = veebha_i18n_str(STR_LANGUAGE),
        .items = s_lang_items,
        .count = sizeof(s_lang_items) / sizeof(s_lang_items[0]),
        .on_select = on_language_select,
        .on_back = NULL,
        .lsk_label = veebha_i18n_str(STR_SELECT),
        .rsk_label = veebha_i18n_str(STR_BACK)
    };

    lv_obj_t *scr = tpl_list_create(&desc);
    if (scr) {
        win_mgr_push(scr, veebha_i18n_str(STR_SELECT), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
    }
}

/* ============================================================================
 * Main Settings Menu
 * ============================================================================ */
static void on_main_settings_select(uint16_t index)
{
    printf("[SETTINGS] Main Settings Item %u Selected\n", index);
    if (index == 0) {
        open_display_settings();
    } else if (index == 1) {
        open_sound_settings();
    } else if (index == 2) {
        open_connectivity_settings();
    } else if (index == 3) {
        open_datetime_settings();
    } else if (index == 4) {
        open_language_settings();
    } else if (index == 5) {
        open_about_phone();
    }
}

void app_settings_open(void)
{
    app_settings_init();

    static tpl_list_item_t s_main_items[6];
    s_main_items[0] = (tpl_list_item_t){ .icon = LV_SYMBOL_EYE_OPEN,   .title = veebha_i18n_str(STR_DISPLAY),        .subtext = "Theme, Backlight" };
    s_main_items[1] = (tpl_list_item_t){ .icon = LV_SYMBOL_VOLUME_MAX, .title = veebha_i18n_str(STR_AUDIO_PROFILES), .subtext = "Profiles, Volume" };
    s_main_items[2] = (tpl_list_item_t){ .icon = LV_SYMBOL_SETTINGS,   .title = veebha_i18n_str(STR_CONNECTIVITY),   .subtext = "Bluetooth, Tethering" };
    s_main_items[3] = (tpl_list_item_t){ .icon = LV_SYMBOL_BELL,       .title = veebha_i18n_str(STR_DATE_TIME),      .subtext = "Clock, Timezone" };
    s_main_items[4] = (tpl_list_item_t){ .icon = LV_SYMBOL_EDIT,       .title = veebha_i18n_str(STR_LANGUAGE),       .subtext = "English, हिन्दी, Русский" };
    s_main_items[5] = (tpl_list_item_t){ .icon = LV_SYMBOL_SETTINGS,   .title = veebha_i18n_str(STR_ABOUT),          .subtext = "Hardware & Build info" };

    tpl_list_view_t desc = {
        .title = veebha_i18n_str(STR_SETTINGS),
        .items = s_main_items,
        .count = sizeof(s_main_items) / sizeof(s_main_items[0]),
        .on_select = on_main_settings_select,
        .on_back = NULL,
        .lsk_label = veebha_i18n_str(STR_SELECT),
        .rsk_label = veebha_i18n_str(STR_BACK)
    };

    lv_obj_t *scr = tpl_list_create(&desc);
    if (scr) {
        win_mgr_push(scr, veebha_i18n_str(STR_SELECT), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
        printf("[SETTINGS] Main Settings menu opened\n");
    }
}
