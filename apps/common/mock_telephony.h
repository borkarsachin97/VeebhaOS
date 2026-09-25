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

#ifndef APPS_COMMON_MOCK_TELEPHONY_H
#define APPS_COMMON_MOCK_TELEPHONY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define TELEPHONY_MAX_CONTACTS 64
#define TELEPHONY_MAX_MESSAGES 64

/**
 * Contact Record Structure
 */
typedef struct {
    uint32_t id;
    char     name[32];
    char     number[20];
} contact_record_t;

/**
 * SMS Message Structure
 */
typedef struct {
    uint32_t id;
    char     sender[32];
    char     timestamp[16];
    char     preview[64];
    char     body[256];
    bool     is_read;
} sms_message_t;

/**
 * Initialize mock telephony store with default data if not already done.
 */
void telephony_store_init(void);

/**
 * Retrieve the current contacts array.
 *
 * @param out_count Output pointer to write total number of contacts.
 * @return Pointer to internal contacts array.
 */
contact_record_t * telephony_get_contacts(uint16_t *out_count);

/**
 * Add a new contact to the store.
 *
 * @param name   Contact name.
 * @param number Phone number.
 * @return true on success, false if storage full or invalid inputs.
 */
bool telephony_add_contact(const char *name, const char *number);

/**
 * Search contacts for exact or partial phone number match.
 *
 * @param number Phone number query string.
 * @return Pointer to matching contact record, or NULL if not found.
 */
const contact_record_t * telephony_find_contact_by_number(const char *number);

/**
 * Retrieve the current messages array.
 *
 * @param out_count Output pointer to write total number of messages.
 * @return Pointer to internal messages array.
 */
sms_message_t * telephony_get_messages(uint16_t *out_count);

/**
 * Send and store a new SMS message.
 *
 * @param recipient Recipient phone number or name.
 * @param body      SMS body content.
 * @return true on success, false if storage full or invalid inputs.
 */
bool telephony_send_sms(const char *recipient, const char *body);

/**
 * Simulate receiving an incoming SMS message (stored as unread).
 *
 * @param sender Sender phone number or name.
 * @param body   SMS body content.
 * @return true on success, false if storage full.
 */
bool telephony_receive_sms(const char *sender, const char *body);

/**
 * Retrieve current unread SMS count.
 */
uint16_t telephony_get_unread_sms_count(void);

/**
 * Retrieve current missed call count.
 */
uint16_t telephony_get_missed_call_count(void);

/**
 * Call Log Entry Types
 */
typedef enum {
    CALL_TYPE_INCOMING,
    CALL_TYPE_OUTGOING,
    CALL_TYPE_MISSED
} call_type_t;

/**
 * Call Log Entry Structure
 */
typedef struct {
    uint32_t    id;
    char        name[32];
    char        number[20];
    char        timestamp[32];
    uint32_t    duration; /* in seconds */
    call_type_t type;
} call_log_entry_t;

#define TELEPHONY_MAX_CALL_LOGS 32

/**
 * Retrieve the current call logs array.
 *
 * @param out_count Output pointer to write total number of call logs.
 * @return Pointer to internal call logs array (ordered newest first).
 */
call_log_entry_t * telephony_get_call_logs(uint16_t *out_count);

/**
 * Append a new call log entry (placed at index 0).
 *
 * @param name      Contact name (or number if unknown).
 * @param number    Phone number.
 * @param type      CALL_TYPE_INCOMING, CALL_TYPE_OUTGOING, or CALL_TYPE_MISSED.
 * @param duration  Call duration in seconds.
 * @param timestamp Display string for date/time (e.g. "Today 10:15", "Just now").
 * @return true on success, false on error.
 */
bool telephony_add_call_log(const char *name, const char *number, call_type_t type, uint32_t duration, const char *timestamp);

/**
 * Persist the entire telephony store (contacts, messages, call logs) to binary file.
 *
 * @return true on success, false on error.
 */
bool telephony_store_save(void);

/**
 * Load the telephony store from binary file.
 *
 * @return true on success, false on error or file missing.
 */
bool telephony_store_load(void);

#ifdef __cplusplus
}
#endif

#endif /* APPS_COMMON_MOCK_TELEPHONY_H */
