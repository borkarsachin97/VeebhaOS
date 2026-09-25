/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Hardware Torch / Flashlight LED Driver Header for RDA8809
 * Controls Torch / Flashlight LED via PMU (RDA1203/ISPI) Current Sink Channels.
 */

#ifndef _HAL_TORCH_H_
#define _HAL_TORCH_H_

#include "cs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
//  PMU LED CHANNEL MASKS
// =============================================================================
#define TORCH_CHAN_R            (1 << 0)
#define TORCH_CHAN_G            (1 << 1)
#define TORCH_CHAN_B            (1 << 2)
#define TORCH_CHAN_ALL          (TORCH_CHAN_R | TORCH_CHAN_G | TORCH_CHAN_B)

// =============================================================================
//  PUBLIC TORCH / FLASHLIGHT API
// =============================================================================

/**
 * @brief Initialize Torch / Flashlight LED driver.
 */
void hal_TorchInit(void);

/**
 * @brief Turn the Torch LED ON or OFF at default maximum brightness.
 * @param on true for ON, false for OFF.
 */
void hal_TorchSet(bool on);

/**
 * @brief Toggle the Torch LED state.
 * @return New state (true = ON, false = OFF).
 */
bool hal_TorchToggle(void);

/**
 * @brief Check if Torch is currently turned ON.
 */
bool hal_TorchIsOn(void);

/**
 * @brief Set brightness level (0 = OFF, 1..7 = Dim to Maximum).
 * @param level Brightness level (0..7).
 */
void hal_TorchSetLevel(uint8_t level);

/**
 * @brief Get current brightness level (0..7).
 */
uint8_t hal_TorchGetLevel(void);

/**
 * @brief Test individual PMU current-sink channels (R, G, B, or combinations).
 * @param channel_mask Bitmask of TORCH_CHAN_R, TORCH_CHAN_G, TORCH_CHAN_B.
 * @param level Brightness level (0..7).
 */
void hal_TorchSetChannels(uint8_t channel_mask, uint8_t level);

/**
 * @brief Get the active PMU channel mask.
 */
uint8_t hal_TorchGetChannels(void);

#ifdef __cplusplus
}
#endif

#endif // _HAL_TORCH_H_
