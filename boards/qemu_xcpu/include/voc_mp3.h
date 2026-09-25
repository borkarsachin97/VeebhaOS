/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * VoC Hardware MP3 Decoder Interface for RDA8809
 */

#ifndef _VOC_MP3_H_
#define _VOC_MP3_H_

#include "cs_types.h"

typedef struct
{
    INT32 sample_rate;
    INT16 channels;
    INT16 bitrate_kbps;
    INT16 consumed_bytes;
    INT32 output_pcm_bytes;
    INT16 error_status;
} voc_mp3_frame_info_t;

/**
 * @brief Initialize VoC coprocessor, load common microcode and register MP3 DMA overlay tables.
 * @return 0 on success, negative on error.
 */
int voc_mp3_open(void);

/**
 * @brief Close VoC session and power down VoC clock.
 */
void voc_mp3_close(void);

/**
 * @brief Decode one MP3 frame using hardware VoC DSP engine.
 * @param in_buf Pointer to input MP3 bitstream (must be in KSEG0/KSEG1 memory).
 * @param in_len Length of input data available in in_buf.
 * @param out_pcm Pointer to output buffer for decoded 16-bit stereo PCM (at least 4608 bytes).
 * @param info Pointer to voc_mp3_frame_info_t to receive frame metadata.
 * @return 0 on success, negative on error or EOF.
 */
int voc_mp3_decode_frame(const void *in_buf, int in_len, void *out_pcm, voc_mp3_frame_info_t *info);

/**
 * @brief Check if VoC MP3 decoder session is currently open.
 */
BOOL voc_mp3_is_open(void);

#endif // _VOC_MP3_H_
