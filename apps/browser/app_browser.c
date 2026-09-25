/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "app_browser.h"
#include "sdk/include/veebha_connectivity.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_i18n.h"
#include "sdk/include/veebha_log.h"
#include "sdk/include/veebha_status_bar.h"
#include "sdk/text/font_fallback.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TAG "APP_BROWSER"

#define BROWSER_MAX_HISTORY   16
#define BROWSER_MAX_LINKS     32
#define BROWSER_MAX_BOOKMARKS 16

typedef struct {
    char url[128];
    char title[48];
} bookmark_entry_t;

typedef struct {
    lv_obj_t          *screen;
    lv_obj_t          *url_label;
    lv_obj_t          *content_scroll;
    char               current_url[128];
    char               history[BROWSER_MAX_HISTORY][128];
    uint8_t            history_count;
    int8_t             history_idx;
    
    char               links[BROWSER_MAX_LINKS][128];
    lv_obj_t          *link_objs[BROWSER_MAX_LINKS];
    uint8_t            link_count;
    int8_t             focused_link;
} browser_data_t;

static browser_data_t *s_browser_ctx = NULL;

static bookmark_entry_t s_bookmarks[BROWSER_MAX_BOOKMARKS] = {
    { "http://home.veebha",     "Veebha Portal" },
    { "http://frogfind.com",    "FrogFind (Lite Search)" },
    { "http://68k.news",        "68k News (Text News)" },
    { "http://neverssl.com",    "NeverSSL Test" },
    { "http://news.wap",        "Global News WAP" },
    { "http://weather.local",   "Local Weather" },
    { "http://m.wikipedia.org", "Wikipedia Mobile" },
};
static uint8_t s_bookmark_count = 7;

static void browser_render_html(browser_data_t *data, const char *html);
static void on_browser_options_action(void);
static void on_browser_back_action(void);

bool app_browser_is_active(void)
{
    return s_browser_ctx != NULL && s_browser_ctx->screen != NULL && lv_obj_is_valid(s_browser_ctx->screen);
}

static void on_link_clicked_cb(lv_event_t *e)
{
    const char *target_url = (const char *)lv_event_get_user_data(e);
    if (target_url && s_browser_ctx) {
        OS_LOGI(TAG, "Navigating to link: %s", target_url);
        app_browser_load_url(target_url);
    }
}

static void browser_clean_html_text(const char *src, char *out, size_t out_max)
{
    if (!out || out_max == 0) return;
    out[0] = '\0';
    if (!src) return;

    size_t d = 0;
    const char *s = src;
    bool last_was_space = false;

    /* Skip leading whitespace */
    while (*s && isspace((unsigned char)*s)) s++;

    while (*s && d < out_max - 1) {
        if (*s == '<') {
            /* Check if it is a break tag */
            if (strncasecmp(s, "<br>", 4) == 0 || strncasecmp(s, "<br/>", 5) == 0 || strncasecmp(s, "<br />", 6) == 0) {
                if (d > 0 && out[d - 1] != '\n') {
                    out[d++] = '\n';
                }
                const char *close = strchr(s, '>');
                s = close ? (close + 1) : (s + 4);
                last_was_space = false;
                continue;
            }
            /* Strip all other inline tags like <b>, </b>, <i>, </i>, <strong>, <span>, etc. */
            const char *end_tag = strchr(s, '>');
            if (end_tag) {
                s = end_tag + 1;
                continue;
            } else {
                s++;
                continue;
            }
        } else if (*s == '&') {
            /* Decode HTML entities */
            if (strncasecmp(s, "&gt;", 4) == 0) {
                out[d++] = '>';
                s += 4;
            } else if (strncasecmp(s, "&lt;", 4) == 0) {
                out[d++] = '<';
                s += 4;
            } else if (strncasecmp(s, "&amp;", 5) == 0) {
                out[d++] = '&';
                s += 5;
            } else if (strncasecmp(s, "&quot;", 6) == 0) {
                out[d++] = '"';
                s += 6;
            } else if (strncasecmp(s, "&#39;", 5) == 0 || strncasecmp(s, "&apos;", 6) == 0) {
                out[d++] = '\'';
                s += (s[1] == '#') ? 5 : 6;
            } else if (strncasecmp(s, "&nbsp;", 6) == 0) {
                out[d++] = ' ';
                s += 6;
            } else if (strncasecmp(s, "&copy;", 6) == 0) {
                if (d + 3 < out_max - 1) {
                    out[d++] = '(';
                    out[d++] = 'c';
                    out[d++] = ')';
                }
                s += 6;
            } else if (strncasecmp(s, "&ndash;", 7) == 0 || strncasecmp(s, "&mdash;", 7) == 0) {
                out[d++] = '-';
                s += 7;
            } else if (s[1] == '#' && (s[2] >= '0' && s[2] <= '9')) {
                /* Decimal numeric entity &#NNN; */
                const char *p = s + 2;
                int val = 0;
                while (*p >= '0' && *p <= '9') {
                    val = val * 10 + (*p - '0');
                    p++;
                }
                if (*p == ';') {
                    if (val > 0 && val < 128) {
                        out[d++] = (char)val;
                    }
                    s = p + 1;
                } else {
                    out[d++] = *s++;
                }
            } else {
                out[d++] = *s++;
            }
            last_was_space = false;
        } else if (isspace((unsigned char)*s)) {
            if (*s == '\n' || *s == '\r') {
                if (!last_was_space) {
                    out[d++] = ' ';
                    last_was_space = true;
                }
            } else if (!last_was_space) {
                out[d++] = ' ';
                last_was_space = true;
            }
            s++;
        } else {
            out[d++] = *s++;
            last_was_space = false;
        }
    }

    /* Trim trailing space */
    while (d > 0 && (out[d - 1] == ' ' || out[d - 1] == '\r')) {
        d--;
    }
    out[d] = '\0';
}

static void browser_render_html(browser_data_t *data, const char *html)
{
    if (!data || !data->content_scroll) return;

    /* Clean old children */
    lv_obj_clean(data->content_scroll);
    data->link_count = 0;
    data->focused_link = -1;

    if (!html || strlen(html) == 0) {
        lv_obj_t *lbl = lv_label_create(data->content_scroll);
        lv_label_set_text(lbl, "Empty page.");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x8A99AD), 0);
        return;
    }

    lv_group_t *g = win_mgr_get_group();

    const char *p = html;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        /* Skip comments */
        if (strncasecmp(p, "<!--", 4) == 0) {
            const char *end = strstr(p, "-->");
            p = end ? (end + 3) : (p + 4);
            continue;
        }

        /* Skip scripts, styles, head, meta, link */
        if (strncasecmp(p, "<script", 7) == 0) {
            const char *end = strcasestr(p, "</script>");
            p = end ? (end + 9) : (p + 7);
            continue;
        }
        if (strncasecmp(p, "<style", 6) == 0) {
            const char *end = strcasestr(p, "</style>");
            p = end ? (end + 8) : (p + 6);
            continue;
        }
        if (strncasecmp(p, "<head", 5) == 0) {
            const char *end = strcasestr(p, "</head>");
            p = end ? (end + 7) : (p + 5);
            continue;
        }
        if (strncasecmp(p, "<!doctype", 9) == 0 || strncasecmp(p, "<meta", 5) == 0 || strncasecmp(p, "<link", 5) == 0) {
            const char *end = strchr(p, '>');
            p = end ? (end + 1) : (p + 5);
            continue;
        }

        /* Headings: <h1> to <h6> */
        if (p[0] == '<' && (p[1] == 'h' || p[1] == 'H') && (p[2] >= '1' && p[2] <= '6')) {
            char close_tag[8];
            snprintf(close_tag, sizeof(close_tag), "</%c%c>", p[1], p[2]);
            bool is_h1 = (p[2] == '1');

            const char *tag_end = strchr(p, '>');
            if (tag_end) {
                p = tag_end + 1;
            } else {
                p += 4;
            }

            const char *end = strcasestr(p, close_tag);
            size_t raw_len = end ? (size_t)(end - p) : strlen(p);
            char raw[256];
            if (raw_len >= sizeof(raw)) raw_len = sizeof(raw) - 1;
            strncpy(raw, p, raw_len);
            raw[raw_len] = '\0';

            char clean[256];
            browser_clean_html_text(raw, clean, sizeof(clean));

            if (clean[0] != '\0') {
                lv_obj_t *hlbl = lv_label_create(data->content_scroll);
                lv_label_set_text(hlbl, clean);
                lv_label_set_long_mode(hlbl, LV_LABEL_LONG_WRAP);
                lv_obj_set_width(hlbl, lv_pct(100));
                lv_obj_set_style_text_color(hlbl, is_h1 ? lv_color_hex(0x00E5FF) : lv_color_hex(0xFFD600), 0);
                lv_obj_set_style_text_font(hlbl, is_h1 ? &lv_font_montserrat_14 : veebha_font_get_default(), 0);
                lv_obj_set_style_pad_top(hlbl, 4, 0);
                lv_obj_set_style_pad_bottom(hlbl, 2, 0);
            }

            p = end ? (end + strlen(close_tag)) : (p + raw_len);
            continue;
        }

        /* Horizontal rule <hr> */
        if (strncasecmp(p, "<hr", 3) == 0) {
            lv_obj_t *hr = lv_obj_create(data->content_scroll);
            lv_obj_set_size(hr, lv_pct(100), 1);
            lv_obj_set_style_bg_color(hr, lv_color_hex(0x223254), 0);
            lv_obj_set_style_border_width(hr, 0, 0);
            lv_obj_set_style_pad_ver(hr, 2, 0);
            const char *close = strchr(p, '>');
            p = close ? (close + 1) : (p + 3);
            continue;
        }

        /* Hyperlink: <a ... href="...">...</a> */
        if (strncasecmp(p, "<a ", 3) == 0) {
            const char *href_start = strcasestr(p, "href=\"");
            if (!href_start) href_start = strcasestr(p, "href='");
            char href_url[128] = "";
            if (href_start) {
                char quote = href_start[5];
                href_start += 6;
                const char *href_end = strchr(href_start, quote);
                if (href_end) {
                    size_t hlen = href_end - href_start;
                    if (hlen >= sizeof(href_url)) hlen = sizeof(href_url) - 1;
                    strncpy(href_url, href_start, hlen);
                    href_url[hlen] = '\0';
                }
            }

            const char *tag_end = strchr(p, '>');
            const char *a_end = tag_end ? strcasestr(tag_end, "</a>") : NULL;
            if (tag_end && a_end && href_url[0] != '\0') {
                tag_end++;
                size_t link_text_len = a_end - tag_end;
                char raw_link[128];
                if (link_text_len >= sizeof(raw_link)) link_text_len = sizeof(raw_link) - 1;
                strncpy(raw_link, tag_end, link_text_len);
                raw_link[link_text_len] = '\0';

                char clean_link[128];
                browser_clean_html_text(raw_link, clean_link, sizeof(clean_link));

                if (data->link_count < BROWSER_MAX_LINKS) {
                    uint8_t lidx = data->link_count++;
                    strncpy(data->links[lidx], href_url, sizeof(data->links[lidx]) - 1);

                    lv_obj_t *btn = lv_button_create(data->content_scroll);
                    data->link_objs[lidx] = btn;
                    lv_obj_set_size(btn, lv_pct(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_color(btn, lv_color_hex(0x131D33), 0);
                    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
                    lv_obj_set_style_pad_ver(btn, 3, 0);
                    lv_obj_set_style_pad_hor(btn, 6, 0);
                    lv_obj_set_style_radius(btn, 3, 0);
                    lv_obj_set_style_border_color(btn, lv_color_hex(0x2979FF), 0);
                    lv_obj_set_style_border_width(btn, 1, 0);

                    lv_obj_t *lbl = lv_label_create(btn);
                    char lbuf[140];
                    snprintf(lbuf, sizeof(lbuf), LV_SYMBOL_RIGHT " %s", clean_link[0] ? clean_link : href_url);
                    lv_label_set_text(lbl, lbuf);
                    lv_obj_set_style_text_color(lbl, lv_color_hex(0x64B5F6), 0);
                    lv_obj_set_style_text_font(lbl, veebha_font_get_default(), 0);

                    lv_obj_add_event_cb(btn, on_link_clicked_cb, LV_EVENT_CLICKED, data->links[lidx]);

                    if (g) {
                        lv_group_add_obj(g, btn);
                    }
                }
                p = a_end + 4;
                continue;
            }
        }

        /* Block elements: <p...>, <div...>, <li...>, <blockquote...> */
        if (strncasecmp(p, "<p", 2) == 0 || strncasecmp(p, "<div", 4) == 0 ||
            strncasecmp(p, "<li", 3) == 0 || strncasecmp(p, "<blockquote", 11) == 0) {
            const char *tag_end = strchr(p, '>');
            if (tag_end) {
                p = tag_end + 1;
            }
        }

        /* Collect text and inline elements up to next block tag */
        const char *next_tag = strchr(p, '<');
        while (next_tag && (strncasecmp(next_tag, "<b>", 3) == 0 || strncasecmp(next_tag, "</b>", 4) == 0 ||
                            strncasecmp(next_tag, "<i>", 3) == 0 || strncasecmp(next_tag, "</i>", 4) == 0 ||
                            strncasecmp(next_tag, "<strong>", 8) == 0 || strncasecmp(next_tag, "</strong>", 9) == 0 ||
                            strncasecmp(next_tag, "<em>", 4) == 0 || strncasecmp(next_tag, "</em>", 5) == 0 ||
                            strncasecmp(next_tag, "<u>", 3) == 0 || strncasecmp(next_tag, "</u>", 4) == 0 ||
                            strncasecmp(next_tag, "<span", 5) == 0 || strncasecmp(next_tag, "</span>", 7) == 0 ||
                            strncasecmp(next_tag, "<font", 5) == 0 || strncasecmp(next_tag, "</font>", 7) == 0 ||
                            strncasecmp(next_tag, "<small", 6) == 0 || strncasecmp(next_tag, "</small>", 8) == 0 ||
                            strncasecmp(next_tag, "<br", 3) == 0)) {
            /* Skip inline tags and keep accumulating block text */
            const char *after = strchr(next_tag, '>');
            if (after) {
                next_tag = strchr(after + 1, '<');
            } else {
                break;
            }
        }

        size_t raw_len = next_tag ? (size_t)(next_tag - p) : strlen(p);
        if (raw_len > 0) {
            char raw[512];
            if (raw_len >= sizeof(raw)) raw_len = sizeof(raw) - 1;
            strncpy(raw, p, raw_len);
            raw[raw_len] = '\0';

            /* Check if raw contains an embedded <a href="..."> */
            char *a_start = strcasestr(raw, "<a ");
            if (a_start) {
                /* Render pre-link text if any */
                *a_start = '\0';
                char clean_pre[512];
                browser_clean_html_text(raw, clean_pre, sizeof(clean_pre));
                if (clean_pre[0] != '\0') {
                    lv_obj_t *plbl = lv_label_create(data->content_scroll);
                    lv_label_set_long_mode(plbl, LV_LABEL_LONG_WRAP);
                    lv_obj_set_width(plbl, lv_pct(100));
                    lv_label_set_text(plbl, clean_pre);
                    lv_obj_set_style_text_color(plbl, lv_color_hex(0xE0E6ED), 0);
                    lv_obj_set_style_text_font(plbl, veebha_font_get_default(), 0);
                    lv_obj_set_style_pad_ver(plbl, 2, 0);
                }

                /* Render link */
                p = (p + (a_start - raw));
                continue;
            } else {
                char clean[512];
                browser_clean_html_text(raw, clean, sizeof(clean));

                if (clean[0] != '\0') {
                    lv_obj_t *plbl = lv_label_create(data->content_scroll);
                    lv_label_set_long_mode(plbl, LV_LABEL_LONG_WRAP);
                    lv_obj_set_width(plbl, lv_pct(100));
                    lv_label_set_text(plbl, clean);
                    lv_obj_set_style_text_color(plbl, lv_color_hex(0xE0E6ED), 0);
                    lv_obj_set_style_text_font(plbl, veebha_font_get_default(), 0);
                    lv_obj_set_style_pad_ver(plbl, 2, 0);
                }
            }
            p = next_tag ? next_tag : (p + raw_len);
        } else if (*p == '<') {
            const char *next = strchr(p, '>');
            p = next ? (next + 1) : (p + 1);
        } else {
            p++;
        }
    }
}

void app_browser_load_url(const char *url)
{
    if (!s_browser_ctx || !url) return;

    OS_LOGI(TAG, "Loading URL: %s", url);
    if (s_browser_ctx->current_url != url) {
        strncpy(s_browser_ctx->current_url, url, sizeof(s_browser_ctx->current_url) - 1);
        s_browser_ctx->current_url[sizeof(s_browser_ctx->current_url) - 1] = '\0';
    }

    if (s_browser_ctx->url_label) {
        lv_label_set_text(s_browser_ctx->url_label, s_browser_ctx->current_url);
    }

    /* Record history if new */
    if (s_browser_ctx->history_count == 0 || 
        (s_browser_ctx->history_idx >= 0 && strcmp(s_browser_ctx->history[s_browser_ctx->history_idx], s_browser_ctx->current_url) != 0)) {
        if (s_browser_ctx->history_idx + 1 < BROWSER_MAX_HISTORY) {
            s_browser_ctx->history_idx++;
            strncpy(s_browser_ctx->history[s_browser_ctx->history_idx], s_browser_ctx->current_url, sizeof(s_browser_ctx->history[0]) - 1);
            s_browser_ctx->history[s_browser_ctx->history_idx][sizeof(s_browser_ctx->history[0]) - 1] = '\0';
            s_browser_ctx->history_count = s_browser_ctx->history_idx + 1;
        }
    }

    os_http_response_t resp;
    bool ok = connectivity_http_get(url, &resp);
    if (ok && resp.body) {
        browser_render_html(s_browser_ctx, resp.body);
    } else {
        /* If network failed, display offline response */
        if (resp.body) {
            browser_render_html(s_browser_ctx, resp.body);
        } else {
            browser_render_html(s_browser_ctx, 
                "<h1>Connection Failed</h1>\n"
                "<p>Unable to connect to the requested host.</p>\n"
                "<hr>\n"
                "<p><a href=\"http://home.veebha\">Return Home</a></p>");
        }
    }
    connectivity_http_response_free(&resp);

    /* Update softkeys */
    softkey_set_actions(veebha_i18n_str(STR_OPTIONS), on_browser_options_action,
                        veebha_i18n_str(STR_BACK), on_browser_back_action);
}

static void on_browser_back_action(void)
{
    if (!s_browser_ctx) {
        win_mgr_pop();
        return;
    }

    if (s_browser_ctx->history_idx > 0) {
        s_browser_ctx->history_idx--;
        const char *prev_url = s_browser_ctx->history[s_browser_ctx->history_idx];
        OS_LOGI(TAG, "History Back -> %s", prev_url);
        app_browser_load_url(prev_url);
    } else {
        OS_LOGI(TAG, "Exiting Browser");
        win_mgr_pop();
    }
}

static void on_enter_url_confirm(const char *url_input)
{
    if (url_input && strlen(url_input) > 0) {
        char full[128];
        if (strncmp(url_input, "http://", 7) != 0 && strncmp(url_input, "https://", 8) != 0) {
            snprintf(full, sizeof(full), "http://%s", url_input);
        } else {
            strncpy(full, url_input, sizeof(full) - 1);
        }
        app_browser_load_url(full);
    }
}

static void on_bookmark_select(uint16_t index)
{
    win_mgr_pop();
    if (index < s_bookmark_count) {
        app_browser_load_url(s_bookmarks[index].url);
    }
}

static void open_bookmarks_view(void)
{
    static tpl_list_item_t b_items[BROWSER_MAX_BOOKMARKS];
    for (uint8_t i = 0; i < s_bookmark_count; i++) {
        b_items[i].icon = LV_SYMBOL_WIFI;
        b_items[i].title = s_bookmarks[i].title;
        b_items[i].subtext = s_bookmarks[i].url;
    }

    tpl_list_view_t desc = {
        .title = veebha_i18n_str(STR_BOOKMARKS),
        .items = b_items,
        .count = s_bookmark_count,
        .on_select = on_bookmark_select,
        .on_back = NULL,
        .lsk_label = veebha_i18n_str(STR_SELECT),
        .rsk_label = veebha_i18n_str(STR_BACK)
    };
    lv_obj_t *bscr = tpl_list_create(&desc);
    if (bscr) {
        win_mgr_push(bscr, veebha_i18n_str(STR_SELECT), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
    }
}

static void on_editor_save_url(const char *text)
{
    win_mgr_pop();
    on_enter_url_confirm(text);
}

static void on_editor_cancel_url(void)
{
    win_mgr_pop();
}

static void open_url_editor(void)
{
    static char s_url_buf[128];
    strncpy(s_url_buf, s_browser_ctx ? s_browser_ctx->current_url : "http://", sizeof(s_url_buf) - 1);
    tpl_editor_desc_t ed = {
        .title = veebha_i18n_str(STR_ENTER_URL),
        .buffer = s_url_buf,
        .max_len = sizeof(s_url_buf) - 1,
        .on_save = on_editor_save_url,
        .on_cancel = on_editor_cancel_url,
        .lsk_label = "Go",
        .rsk_label = "Cancel"
    };
    lv_obj_t *ed_scr = tpl_editor_create(&ed);
    if (ed_scr) {
        win_mgr_push(ed_scr, "Go", tpl_editor_default_lsk, "Cancel", tpl_editor_default_rsk);
    }
}

static void on_options_menu_select(uint16_t index)
{
    win_mgr_pop();
    if (!s_browser_ctx) return;

    switch (index) {
    case 0: /* Enter URL */
        open_url_editor();
        break;
    case 1: /* Bookmarks */
        open_bookmarks_view();
        break;
    case 2: /* Add Bookmark */
        if (s_bookmark_count < BROWSER_MAX_BOOKMARKS) {
            strncpy(s_bookmarks[s_bookmark_count].url, s_browser_ctx->current_url, sizeof(s_bookmarks[0].url) - 1);
            strncpy(s_bookmarks[s_bookmark_count].title, s_browser_ctx->current_url, sizeof(s_bookmarks[0].title) - 1);
            s_bookmark_count++;
            OS_LOGI(TAG, "Bookmark added: %s", s_browser_ctx->current_url);
        }
        break;
    case 3: /* Reload */
        app_browser_load_url(s_browser_ctx->current_url);
        break;
    case 4: /* Home Portal */
        app_browser_load_url("http://home.veebha");
        break;
    default:
        break;
    }
}

static void on_browser_options_action(void)
{
    static tpl_list_item_t opt_items[5];
    opt_items[0] = (tpl_list_item_t){ .icon = LV_SYMBOL_EDIT,      .title = veebha_i18n_str(STR_ENTER_URL), .subtext = "Type website address" };
    opt_items[1] = (tpl_list_item_t){ .icon = LV_SYMBOL_LIST,      .title = veebha_i18n_str(STR_BOOKMARKS), .subtext = "Saved web pages" };
    opt_items[2] = (tpl_list_item_t){ .icon = LV_SYMBOL_PLUS,      .title = "Add Bookmark",                 .subtext = "Save current page" };
    opt_items[3] = (tpl_list_item_t){ .icon = LV_SYMBOL_REFRESH,   .title = veebha_i18n_str(STR_RELOAD),    .subtext = "Refresh web page" };
    opt_items[4] = (tpl_list_item_t){ .icon = LV_SYMBOL_HOME,      .title = "Home Portal",                  .subtext = "http://home.veebha" };

    tpl_list_view_t desc = {
        .title = veebha_i18n_str(STR_OPTIONS),
        .items = opt_items,
        .count = 5,
        .on_select = on_options_menu_select,
        .on_back = NULL,
        .lsk_label = veebha_i18n_str(STR_SELECT),
        .rsk_label = veebha_i18n_str(STR_BACK)
    };
    lv_obj_t *opt_scr = tpl_list_create(&desc);
    if (opt_scr) {
        win_mgr_push(opt_scr, veebha_i18n_str(STR_SELECT), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
    }
}

static void on_browser_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    browser_data_t *data = (browser_data_t *)lv_event_get_user_data(e);
    if (!data) return;

    if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_DOWN || key == '8') {
            if (data->content_scroll) {
                lv_obj_scroll_by_bounded(data->content_scroll, 0, -30, LV_ANIM_OFF);
            }
        } else if (key == LV_KEY_UP || key == '2') {
            if (data->content_scroll) {
                lv_obj_scroll_by_bounded(data->content_scroll, 0, 30, LV_ANIM_OFF);
            }
        } else if (key == LV_KEY_ENTER || key == '5') {
            lv_group_t *g = win_mgr_get_group();
            lv_obj_t *focused = g ? lv_group_get_focused(g) : NULL;
            if (focused && focused != data->screen && focused != data->content_scroll) {
                /* Handled by focused widget */
            } else if (data->link_count > 0 && data->link_objs[0]) {
                app_browser_load_url(data->links[0]);
            } else {
                on_browser_options_action();
            }
        }
    } else if (code == LV_EVENT_DELETE) {
        win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(data->screen);
        if (hdr) {
            free(hdr);
            lv_obj_set_user_data(data->screen, NULL);
        }
        if (s_browser_ctx == data) {
            s_browser_ctx = NULL;
        }
        free(data);
    }
}

void app_browser_init(void)
{
    OS_LOGI(TAG, "Web Browser application module initialized");
}

void app_browser_open(void)
{
    browser_data_t *data = (browser_data_t *)calloc(1, sizeof(browser_data_t));
    if (!data) return;

    s_browser_ctx = data;
    strncpy(data->current_url, "http://home.veebha", sizeof(data->current_url) - 1);
    data->history_idx = -1;
    data->history_count = 0;

    lv_obj_t *screen = lv_obj_create(NULL);
    data->screen = screen;
    lv_obj_set_size(screen, 176, 220);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x060912), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)calloc(1, sizeof(win_mgr_screen_hdr_t));
    if (hdr) {
        hdr->view_type = VEEBHA_VIEW_TYPE_GENERIC;
        strncpy(hdr->title, veebha_i18n_str(STR_BROWSER), sizeof(hdr->title) - 1);
        lv_obj_set_user_data(screen, hdr);
    }

    lv_obj_add_event_cb(screen, on_browser_event_cb, LV_EVENT_ALL, data);

    /* 1. Zone A: Fixed 18px Top Status Bar */
    status_bar_create(screen, NULL);

    /* 2. URL Navigation Strip (18px) */
    lv_obj_t *url_bar = lv_obj_create(screen);
    lv_obj_set_size(url_bar, lv_pct(100), 18);
    lv_obj_set_style_bg_color(url_bar, lv_color_hex(0x0E1729), 0);
    lv_obj_set_style_border_side(url_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(url_bar, lv_color_hex(0x1B2A4A), 0);
    lv_obj_set_style_border_width(url_bar, 1, 0);
    lv_obj_set_style_pad_hor(url_bar, 4, 0);
    lv_obj_set_style_pad_ver(url_bar, 0, 0);
    lv_obj_set_flex_flow(url_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(url_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *wico = lv_label_create(url_bar);
    lv_label_set_text(wico, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wico, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(wico, &lv_font_montserrat_10, 0);

    data->url_label = lv_label_create(url_bar);
    lv_label_set_long_mode(data->url_label, LV_LABEL_LONG_DOT);
    lv_obj_set_flex_grow(data->url_label, 1);
    lv_label_set_text(data->url_label, data->current_url);
    lv_obj_set_style_text_color(data->url_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(data->url_label, &lv_font_montserrat_10, 0);

    /* 3. Content Scrollable Viewport (176 x 164 px) */
    lv_obj_t *viewport = lv_obj_create(screen);
    data->content_scroll = viewport;
    if (hdr) hdr->first_item = viewport;
    lv_obj_set_size(viewport, lv_pct(100), 0);
    lv_obj_set_flex_grow(viewport, 1);
    lv_obj_set_style_bg_color(viewport, lv_color_hex(0x080D1A), 0);
    lv_obj_set_style_border_width(viewport, 0, 0);
    lv_obj_set_style_pad_hor(viewport, 5, 0);
    lv_obj_set_style_pad_ver(viewport, 4, 0);
    lv_obj_set_flex_flow(viewport, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(viewport, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_add_event_cb(viewport, on_browser_event_cb, LV_EVENT_KEY, data);

    /* 4. Bottom Softkey Bar (20px) */
    lv_obj_t *sk = softkey_bar_create(screen, veebha_i18n_str(STR_OPTIONS), veebha_i18n_str(STR_BACK));
    if (hdr) hdr->softkey_bar = sk;

    win_mgr_push(screen, veebha_i18n_str(STR_OPTIONS), on_browser_options_action,
                 veebha_i18n_str(STR_BACK), on_browser_back_action);

    /* Initial page load */
    app_browser_load_url(data->current_url);
}
