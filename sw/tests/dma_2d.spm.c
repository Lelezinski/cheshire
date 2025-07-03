// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Paul Scheffler <paulsc@iis.ee.ethz.ch>
//
// Simple test for iDMA. This is intended *only for execution from SPM*.

#include "util.h"
#include "dif/clint.h"
#include "dif/dma.h"

#define DMA_2D_ROWS         5
#define DMA_2D_COLS         2
#define DMA_2D_SRC_STRIDE   2
#define DMA_2D_DST_STRIDE   3

#define DEBUG_THIS_TEST 0
#if DEBUG_THIS_TEST
#include "printf.h"
#endif

int main(void)
{
    // Immediately return an error if DMA is not present
    CHECK_ASSERT(-1, chs_hw_feature_present(CHESHIRE_HW_FEATURES_DMA_BIT));

    // This test takes 0-9 numbers and copies them, spacing them in blocks of twos, through the 2D DMA.
    volatile char src_cached[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '\0'};
    volatile char gold[]       = {'0', '1', 0, '2', '3', 0, '4', '5', 0, '6', '7', 0, '8', '9', '\0'};

    // Allocate destination memory in SPM
    volatile char dst_cached[sizeof(gold)];

    // Get pointer to uncached SPM source and destination
    volatile char *src = src_cached + 0x04000000;
    volatile char *dst = dst_cached + 0x04000000;

    // Copy from cached to uncached source to ensure it is DMA-accessible
    for (unsigned i = 0; i < sizeof(src_cached); ++i)
        src[i] = src_cached[i];

    // Pre-write finishing "!\0" to guard against overlength transfers
    dst[sizeof(gold) - 1] = '\0';

    // Issue blocking 2D memcpy
    sys_dma_2d_blk_memcpy(
        (uintptr_t)(void *)dst,
        (uintptr_t)(void *)src,
        DMA_2D_COLS,
        DMA_2D_DST_STRIDE,
        DMA_2D_SRC_STRIDE,
        DMA_2D_ROWS
    );

    // Check destination string
    int errors = sizeof(gold);
    for (unsigned i = 0; i < sizeof(gold); ++i) {
        if (dst[i] == gold[i]) {
            errors--;
#if DEBUG_THIS_TEST
            printf("dst[%u]\t= '%c'\t(correct)\n", i, dst[i]);
#endif
        } else {
#if DEBUG_THIS_TEST
            printf("dst[%u]\t= '%c'\t(expected '%c')\n", i, dst[i], gold[i]);
#endif
        }
    }
    return errors;
}
