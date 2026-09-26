/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef APPS_VAPPS_VAPP_BRICK_BREAKER_H
#define APPS_VAPPS_VAPP_BRICK_BREAKER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk/include/veebha_vapp.h"

/**
 * Launch Brick Breaker Arcade Game (.vapp).
 */
void vapp_brick_breaker_launch(const vapp_package_t *pkg);

#ifdef __cplusplus
}
#endif

#endif /* APPS_VAPPS_VAPP_BRICK_BREAKER_H */
