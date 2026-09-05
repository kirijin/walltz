// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Test pattern tile generation

#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("=== Pattern tile test ===\n");

    // Test geometric patterns
    for (int i = 0; i < 8; i++) {
        WtzImage *tile = wtz_generate_pattern_tile(0, i, 60, 0xFF787878U, 1.0);
        if (!tile) {
            printf("FAIL: pattern %d returned NULL\n", i);
            return 1;
        }

        int opaque = 0;
        for (int y = 0; y < tile->height; y++) {
            uint8_t *row = tile->pixels + y * tile->stride;
            for (int x = 0; x < tile->width; x++) {
                if (row[x * 4 + 3] > 0) opaque++;
            }
        }
        printf("Geometric %d: %dx%d, opaque_pixels=%d\n", i, tile->width, tile->height, opaque);

        if (opaque == 0) {
            printf("FAIL: pattern %d is fully transparent\n", i);
            wtz_image_free(tile);
            return 1;
        }

        wtz_image_free(tile);
    }

    // Test SVG geo patterns
    for (int i = 0; i < 16; i++) {
        WtzImage *tile = wtz_generate_pattern_tile(1, i, 60, 0xFF787878U, 1.0);
        if (!tile) {
            printf("FAIL: svg geo %d returned NULL\n", i);
            return 1;
        }

        int opaque = 0;
        for (int y = 0; y < tile->height; y++) {
            uint8_t *row = tile->pixels + y * tile->stride;
            for (int x = 0; x < tile->width; x++) {
                if (row[x * 4 + 3] > 0) opaque++;
            }
        }
        printf("SVG Geo %d: %dx%d, opaque_pixels=%d\n", i, tile->width, tile->height, opaque);

        if (opaque == 0) {
            printf("FAIL: svg geo %d is fully transparent\n", i);
            wtz_image_free(tile);
            return 1;
        }

        wtz_image_free(tile);
    }

    printf("\nOK: All pattern tiles generated\n");
    return 0;
}
