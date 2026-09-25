/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#include "apps/common/mock_telephony.h"
#include "sdk/storage/os_nvram.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TELEPHONY_MAGIC         0x54454C50U  /* "TELP" in little endian */
#define TELEPHONY_VERSION       1U
#define TELEPHONY_FILE_PATH_BLD "./build/telephony_store.bin"
#define TELEPHONY_FILE_PATH_LOC "./telephony_store.bin"

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t crc16;
    uint16_t contact_count;
    uint16_t message_count;
    uint16_t call_log_count;
} __attribute__((packed)) telephony_store_hdr_t;

static contact_record_t s_contacts[TELEPHONY_MAX_CONTACTS];
static uint16_t s_contact_count = 0;

static sms_message_t s_messages[TELEPHONY_MAX_MESSAGES];
static uint16_t s_message_count = 0;

static call_log_entry_t s_call_logs[TELEPHONY_MAX_CALL_LOGS];
static uint16_t s_call_log_count = 0;

static bool s_initialized = false;

static void telephony_seed_defaults(void)
{
    s_contact_count = 0;
    telephony_add_contact("Alice Smith", "+15550101");
    telephony_add_contact("Bob Jones", "+15550102");
    telephony_add_contact("Emergency 112", "112");
    telephony_add_contact("Customer Care", "611");

    s_message_count = 0;

    /* Message 1: Alice Smith */
    s_messages[s_message_count].id = 1;
    strncpy(s_messages[s_message_count].sender, "Alice Smith", sizeof(s_messages[0].sender) - 1);
    strncpy(s_messages[s_message_count].timestamp, "10:45 AM", sizeof(s_messages[0].timestamp) - 1);
    strncpy(s_messages[s_message_count].body, "Hey, are you free this afternoon for the RDA8809 review?", sizeof(s_messages[0].body) - 1);
    strncpy(s_messages[s_message_count].preview, "Hey, are you free this afternoon...", sizeof(s_messages[0].preview) - 1);
    s_messages[s_message_count].is_read = false;
    s_message_count++;

    /* Message 2: Bob Jones */
    s_messages[s_message_count].id = 2;
    strncpy(s_messages[s_message_count].sender, "Bob Jones", sizeof(s_messages[0].sender) - 1);
    strncpy(s_messages[s_message_count].timestamp, "Yesterday", sizeof(s_messages[0].timestamp) - 1);
    strncpy(s_messages[s_message_count].body, "Board hardware review starts at 2 PM in Lab 3.", sizeof(s_messages[0].body) - 1);
    strncpy(s_messages[s_message_count].preview, "Board hardware review starts at 2 PM...", sizeof(s_messages[0].preview) - 1);
    s_messages[s_message_count].is_read = true;
    s_message_count++;

    /* Message 3: Operator */
    s_messages[s_message_count].id = 3;
    strncpy(s_messages[s_message_count].sender, "Operator", sizeof(s_messages[0].sender) - 1);
    strncpy(s_messages[s_message_count].timestamp, "Sep 20", sizeof(s_messages[0].timestamp) - 1);
    strncpy(s_messages[s_message_count].body, "Welcome to VeebhaOS! Zero-coordinate embedded operating system.", sizeof(s_messages[0].body) - 1);
    strncpy(s_messages[s_message_count].preview, "Welcome to VeebhaOS! Zero-coordinate...", sizeof(s_messages[0].preview) - 1);
    s_messages[s_message_count].is_read = true;
    s_message_count++;

    s_call_log_count = 0;
    telephony_add_call_log("Alice Smith", "+15550101", CALL_TYPE_OUTGOING, 84, "Today 10:15");
    telephony_add_call_log("Bob Jones", "+15550102", CALL_TYPE_INCOMING, 195, "Today 09:30");
    telephony_add_call_log("Unknown", "+15550199", CALL_TYPE_MISSED, 0, "Yesterday 18:40");
    telephony_add_call_log("Customer Care", "611", CALL_TYPE_OUTGOING, 45, "Sep 20");
    telephony_add_call_log("Alice Smith", "+15550101", CALL_TYPE_MISSED, 0, "Sep 19");

    telephony_store_save();
}

#if defined(CONFIG_SIMULATOR) && !defined(CONFIG_IS_RAMRUN_ONLY)
static bool load_telephony_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    telephony_store_hdr_t hdr;
    if (fread(&hdr, 1, sizeof(hdr), f) != sizeof(hdr)) {
        fclose(f);
        return false;
    }

    if (hdr.magic != TELEPHONY_MAGIC || hdr.version != TELEPHONY_VERSION) {
        fclose(f);
        return false;
    }

    if (hdr.contact_count > TELEPHONY_MAX_CONTACTS ||
        hdr.message_count > TELEPHONY_MAX_MESSAGES ||
        hdr.call_log_count > TELEPHONY_MAX_CALL_LOGS) {
        fclose(f);
        return false;
    }

    contact_record_t c_buf[TELEPHONY_MAX_CONTACTS];
    sms_message_t m_buf[TELEPHONY_MAX_MESSAGES];
    call_log_entry_t l_buf[TELEPHONY_MAX_CALL_LOGS];

    size_t c_bytes = hdr.contact_count * sizeof(contact_record_t);
    size_t m_bytes = hdr.message_count * sizeof(sms_message_t);
    size_t l_bytes = hdr.call_log_count * sizeof(call_log_entry_t);

    if (c_bytes > 0 && fread(c_buf, 1, c_bytes, f) != c_bytes) {
        fclose(f);
        return false;
    }
    if (m_bytes > 0 && fread(m_buf, 1, m_bytes, f) != m_bytes) {
        fclose(f);
        return false;
    }
    if (l_bytes > 0 && fread(l_buf, 1, l_bytes, f) != l_bytes) {
        fclose(f);
        return false;
    }
    fclose(f);

    /* Verify CRC16 */
    uint16_t calc_crc = 0xFFFF;
    const uint8_t *cnt_hdr = ((const uint8_t *)&hdr) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t);
    size_t cnt_hdr_len = sizeof(hdr) - (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t));
    for (size_t i = 0; i < cnt_hdr_len; i++) {
        calc_crc ^= (uint16_t)cnt_hdr[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (calc_crc & 0x8000) calc_crc = (calc_crc << 1) ^ 0x1021;
            else calc_crc = calc_crc << 1;
        }
    }
    if (c_bytes > 0) {
        const uint8_t *p = (const uint8_t *)c_buf;
        for (size_t i = 0; i < c_bytes; i++) {
            calc_crc ^= (uint16_t)p[i] << 8;
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (calc_crc & 0x8000) calc_crc = (calc_crc << 1) ^ 0x1021;
                else calc_crc = calc_crc << 1;
            }
        }
    }
    if (m_bytes > 0) {
        const uint8_t *p = (const uint8_t *)m_buf;
        for (size_t i = 0; i < m_bytes; i++) {
            calc_crc ^= (uint16_t)p[i] << 8;
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (calc_crc & 0x8000) calc_crc = (calc_crc << 1) ^ 0x1021;
                else calc_crc = calc_crc << 1;
            }
        }
    }
    if (l_bytes > 0) {
        const uint8_t *p = (const uint8_t *)l_buf;
        for (size_t i = 0; i < l_bytes; i++) {
            calc_crc ^= (uint16_t)p[i] << 8;
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (calc_crc & 0x8000) calc_crc = (calc_crc << 1) ^ 0x1021;
                else calc_crc = calc_crc << 1;
            }
        }
    }

    if (calc_crc != hdr.crc16) {
        printf("[TELEPHONY] Store CRC mismatch (0x%04X vs 0x%04X)\n", (unsigned int)hdr.crc16, (unsigned int)calc_crc);
        return false;
    }

    s_contact_count = hdr.contact_count;
    if (c_bytes > 0) memcpy(s_contacts, c_buf, c_bytes);

    s_message_count = hdr.message_count;
    if (m_bytes > 0) memcpy(s_messages, m_buf, m_bytes);

    s_call_log_count = hdr.call_log_count;
    if (l_bytes > 0) memcpy(s_call_logs, l_buf, l_bytes);

    return true;
}
#endif

bool telephony_store_load(void)
{
#if defined(CONFIG_SIMULATOR) && !defined(CONFIG_IS_RAMRUN_ONLY)
    if (load_telephony_file(TELEPHONY_FILE_PATH_BLD)) {
        printf("[TELEPHONY] Loaded store from %s (%u contacts, %u msgs, %u calls)\n",
               TELEPHONY_FILE_PATH_BLD, s_contact_count, s_message_count, s_call_log_count);
        return true;
    }
    if (load_telephony_file(TELEPHONY_FILE_PATH_LOC)) {
        printf("[TELEPHONY] Loaded store from %s (%u contacts, %u msgs, %u calls)\n",
               TELEPHONY_FILE_PATH_LOC, s_contact_count, s_message_count, s_call_log_count);
        return true;
    }
#endif
    return false;
}

bool telephony_store_save(void)
{
    telephony_store_hdr_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = TELEPHONY_MAGIC;
    hdr.version = TELEPHONY_VERSION;
    hdr.contact_count = s_contact_count;
    hdr.message_count = s_message_count;
    hdr.call_log_count = s_call_log_count;

    size_t c_bytes = s_contact_count * sizeof(contact_record_t);
    size_t m_bytes = s_message_count * sizeof(sms_message_t);
    size_t l_bytes = s_call_log_count * sizeof(call_log_entry_t);

    uint16_t calc_crc = 0xFFFF;
    const uint8_t *cnt_hdr = ((const uint8_t *)&hdr) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t);
    size_t cnt_hdr_len = sizeof(hdr) - (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t));
    for (size_t i = 0; i < cnt_hdr_len; i++) {
        calc_crc ^= (uint16_t)cnt_hdr[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (calc_crc & 0x8000) calc_crc = (calc_crc << 1) ^ 0x1021;
            else calc_crc = calc_crc << 1;
        }
    }
    if (c_bytes > 0) {
        const uint8_t *p = (const uint8_t *)s_contacts;
        for (size_t i = 0; i < c_bytes; i++) {
            calc_crc ^= (uint16_t)p[i] << 8;
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (calc_crc & 0x8000) calc_crc = (calc_crc << 1) ^ 0x1021;
                else calc_crc = calc_crc << 1;
            }
        }
    }
    if (m_bytes > 0) {
        const uint8_t *p = (const uint8_t *)s_messages;
        for (size_t i = 0; i < m_bytes; i++) {
            calc_crc ^= (uint16_t)p[i] << 8;
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (calc_crc & 0x8000) calc_crc = (calc_crc << 1) ^ 0x1021;
                else calc_crc = calc_crc << 1;
            }
        }
    }
    if (l_bytes > 0) {
        const uint8_t *p = (const uint8_t *)s_call_logs;
        for (size_t i = 0; i < l_bytes; i++) {
            calc_crc ^= (uint16_t)p[i] << 8;
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (calc_crc & 0x8000) calc_crc = (calc_crc << 1) ^ 0x1021;
                else calc_crc = calc_crc << 1;
            }
        }
    }

    hdr.crc16 = calc_crc;

#if defined(CONFIG_SIMULATOR) && !defined(CONFIG_IS_RAMRUN_ONLY)
    FILE *f = fopen(TELEPHONY_FILE_PATH_BLD, "wb");
    if (!f) {
        f = fopen(TELEPHONY_FILE_PATH_LOC, "wb");
    }
    if (!f) {
        fprintf(stderr, "[TELEPHONY ERROR] Failed to open telephony store for writing\n");
        return false;
    }

    fwrite(&hdr, 1, sizeof(hdr), f);
    if (c_bytes > 0) fwrite(s_contacts, 1, c_bytes, f);
    if (m_bytes > 0) fwrite(s_messages, 1, m_bytes, f);
    if (l_bytes > 0) fwrite(s_call_logs, 1, l_bytes, f);
    fclose(f);
#endif

    return true;
}

void telephony_store_init(void)
{
    if (s_initialized) return;
    s_initialized = true;

    if (telephony_store_load()) {
        printf("[TELEPHONY] Store loaded from disk successfully\n");
        return;
    }

    printf("[TELEPHONY] Initializing default mock telephony dataset\n");
    telephony_seed_defaults();
}

contact_record_t * telephony_get_contacts(uint16_t *out_count)
{
    telephony_store_init();
    if (out_count) *out_count = s_contact_count;
    return s_contacts;
}

bool telephony_add_contact(const char *name, const char *number)
{
    if (!name || !number || name[0] == '\0' || number[0] == '\0') {
        return false;
    }
    if (s_contact_count >= TELEPHONY_MAX_CONTACTS) {
        return false;
    }

    contact_record_t *rec = &s_contacts[s_contact_count];
    rec->id = s_contact_count + 1;
    strncpy(rec->name, name, sizeof(rec->name) - 1);
    rec->name[sizeof(rec->name) - 1] = '\0';
    strncpy(rec->number, number, sizeof(rec->number) - 1);
    rec->number[sizeof(rec->number) - 1] = '\0';

    s_contact_count++;
    printf("[TELEPHONY] Added contact #%u: '%s' (%s)\n", rec->id, rec->name, rec->number);
    telephony_store_save();
    return true;
}

static bool numbers_match(const char *query, const char *target)
{
    if (!query || !target || query[0] == '\0' || target[0] == '\0') return false;

    /* Direct substring match */
    if (strstr(target, query) != NULL) return true;

    /* Clean non-digits for comparison */
    char q_clean[32] = {0};
    char t_clean[32] = {0};
    size_t qi = 0, ti = 0;

    for (size_t i = 0; query[i] && qi < sizeof(q_clean) - 1; i++) {
        if (query[i] >= '0' && query[i] <= '9') q_clean[qi++] = query[i];
    }
    for (size_t i = 0; target[i] && ti < sizeof(t_clean) - 1; i++) {
        if (target[i] >= '0' && target[i] <= '9') t_clean[ti++] = target[i];
    }

    if (qi > 0 && strstr(t_clean, q_clean) != NULL) return true;
    return false;
}

const contact_record_t * telephony_find_contact_by_number(const char *number)
{
    telephony_store_init();
    if (!number || number[0] == '\0') return NULL;

    for (uint16_t i = 0; i < s_contact_count; i++) {
        if (numbers_match(number, s_contacts[i].number)) {
            return &s_contacts[i];
        }
    }
    return NULL;
}

sms_message_t * telephony_get_messages(uint16_t *out_count)
{
    telephony_store_init();
    if (out_count) *out_count = s_message_count;
    return s_messages;
}

bool telephony_send_sms(const char *recipient, const char *body)
{
    telephony_store_init();
    if (!recipient || !body || recipient[0] == '\0' || body[0] == '\0') {
        return false;
    }
    if (s_message_count >= TELEPHONY_MAX_MESSAGES) {
        return false;
    }

    /* Shift messages down so newest appears at index 0 */
    for (int i = s_message_count; i > 0; i--) {
        s_messages[i] = s_messages[i - 1];
    }

    sms_message_t *msg = &s_messages[0];
    msg->id = s_message_count + 1;

    /* Lookup recipient name if available */
    const contact_record_t *contact = telephony_find_contact_by_number(recipient);
    if (contact) {
        strncpy(msg->sender, contact->name, sizeof(msg->sender) - 1);
    } else {
        strncpy(msg->sender, recipient, sizeof(msg->sender) - 1);
    }
    msg->sender[sizeof(msg->sender) - 1] = '\0';

    strncpy(msg->timestamp, "Just now", sizeof(msg->timestamp) - 1);
    msg->timestamp[sizeof(msg->timestamp) - 1] = '\0';

    strncpy(msg->body, body, sizeof(msg->body) - 1);
    msg->body[sizeof(msg->body) - 1] = '\0';

    /* Generate preview */
    strncpy(msg->preview, body, sizeof(msg->preview) - 1);
    if (strlen(body) >= sizeof(msg->preview) - 4) {
        msg->preview[sizeof(msg->preview) - 4] = '.';
        msg->preview[sizeof(msg->preview) - 3] = '.';
        msg->preview[sizeof(msg->preview) - 2] = '.';
        msg->preview[sizeof(msg->preview) - 1] = '\0';
    } else {
        msg->preview[sizeof(msg->preview) - 1] = '\0';
    }

    msg->is_read = true;
    s_message_count++;

    printf("[TELEPHONY] Sent SMS to '%s': \"%s\"\n", msg->sender, msg->preview);
    telephony_store_save();
    return true;
}

bool telephony_receive_sms(const char *sender, const char *body)
{
    telephony_store_init();
    if (!sender || !body || sender[0] == '\0' || body[0] == '\0') {
        return false;
    }
    if (s_message_count >= TELEPHONY_MAX_MESSAGES) {
        return false;
    }

    /* Shift messages down so newest appears at index 0 */
    for (int i = s_message_count; i > 0; i--) {
        s_messages[i] = s_messages[i - 1];
    }

    sms_message_t *msg = &s_messages[0];
    msg->id = s_message_count + 1;

    const contact_record_t *contact = telephony_find_contact_by_number(sender);
    if (contact) {
        strncpy(msg->sender, contact->name, sizeof(msg->sender) - 1);
    } else {
        strncpy(msg->sender, sender, sizeof(msg->sender) - 1);
    }
    msg->sender[sizeof(msg->sender) - 1] = '\0';

    strncpy(msg->timestamp, "Just now", sizeof(msg->timestamp) - 1);
    msg->timestamp[sizeof(msg->timestamp) - 1] = '\0';

    strncpy(msg->body, body, sizeof(msg->body) - 1);
    msg->body[sizeof(msg->body) - 1] = '\0';

    strncpy(msg->preview, body, sizeof(msg->preview) - 1);
    if (strlen(body) >= sizeof(msg->preview) - 4) {
        msg->preview[sizeof(msg->preview) - 4] = '.';
        msg->preview[sizeof(msg->preview) - 3] = '.';
        msg->preview[sizeof(msg->preview) - 2] = '.';
        msg->preview[sizeof(msg->preview) - 1] = '\0';
    } else {
        msg->preview[sizeof(msg->preview) - 1] = '\0';
    }

    msg->is_read = false; /* Unread */
    s_message_count++;

    printf("[TELEPHONY] Received incoming SMS from '%s': \"%s\"\n", msg->sender, msg->preview);
    telephony_store_save();
    return true;
}

uint16_t telephony_get_unread_sms_count(void)
{
    telephony_store_init();
    uint16_t unread = 0;
    for (uint16_t i = 0; i < s_message_count; i++) {
        if (!s_messages[i].is_read) unread++;
    }
    return unread;
}

uint16_t telephony_get_missed_call_count(void)
{
    telephony_store_init();
    uint16_t missed = 0;
    for (uint16_t i = 0; i < s_call_log_count; i++) {
        if (s_call_logs[i].type == CALL_TYPE_MISSED) missed++;
    }
    return missed;
}

call_log_entry_t * telephony_get_call_logs(uint16_t *out_count)
{
    telephony_store_init();
    if (out_count) *out_count = s_call_log_count;
    return s_call_logs;
}

bool telephony_add_call_log(const char *name, const char *number, call_type_t type, uint32_t duration, const char *timestamp)
{
    telephony_store_init();
    if ((!name && !number) || (name && name[0] == '\0' && number && number[0] == '\0')) {
        return false;
    }

    /* Shift entries down to make room at index 0 */
    if (s_call_log_count < TELEPHONY_MAX_CALL_LOGS) {
        for (int i = s_call_log_count; i > 0; i--) {
            s_call_logs[i] = s_call_logs[i - 1];
        }
        s_call_log_count++;
    } else {
        for (int i = TELEPHONY_MAX_CALL_LOGS - 1; i > 0; i--) {
            s_call_logs[i] = s_call_logs[i - 1];
        }
    }

    call_log_entry_t *entry = &s_call_logs[0];
    entry->id = s_call_log_count;
    entry->type = type;
    entry->duration = duration;

    const char *resolved_name = name;
    if ((!resolved_name || resolved_name[0] == '\0') && number) {
        const contact_record_t *c = telephony_find_contact_by_number(number);
        if (c) resolved_name = c->name;
    }
    if (!resolved_name || resolved_name[0] == '\0') {
        resolved_name = (number && number[0] != '\0') ? number : "Unknown";
    }

    strncpy(entry->name, resolved_name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';

    if (number) {
        strncpy(entry->number, number, sizeof(entry->number) - 1);
        entry->number[sizeof(entry->number) - 1] = '\0';
    } else {
        entry->number[0] = '\0';
    }

    if (timestamp) {
        strncpy(entry->timestamp, timestamp, sizeof(entry->timestamp) - 1);
        entry->timestamp[sizeof(entry->timestamp) - 1] = '\0';
    } else {
        strncpy(entry->timestamp, "Just now", sizeof(entry->timestamp) - 1);
        entry->timestamp[sizeof(entry->timestamp) - 1] = '\0';
    }

    printf("[TELEPHONY] Added call log: %s '%s' (%s), duration=%us\n",
           type == CALL_TYPE_OUTGOING ? "OUTGOING" : (type == CALL_TYPE_INCOMING ? "INCOMING" : "MISSED"),
           entry->name, entry->number, duration);
    telephony_store_save();
    return true;
}
