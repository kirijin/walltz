// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Debug pattern tile test

#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    setbuf(stdout, NULL);
    fprintf(stderr, "=== Debug pattern test ===\n");
    fprintf(stderr, "About to call wtz_generate_pattern_tile...\n");

    WtzImage *tile = wtz_generate_pattern_tile(0, 0, 60, 0xFF787878U, 1.0);

    fprintf(stderr, "Returned from wtz_generate_pattern_tile, tile=%p\n", (void*)tile);

    if (!tile) { fprintf(stderr, "FAIL: API returned NULL\n"); return 1; }

    fprintf(stderr, "API Tile: %dx%d stride=%d\n", tile->width, tile->height, tile->stride);

    // Count opaque pixels
    int opaque = 0;
    for (int y = 0; y < tile->height; y++) {
        uint8_t *row = tile->pixels + y * tile->stride;
        for (int x = 0; x < tile->width; x++) {
            if (row[x * 4 + 3] > 0) opaque++;
        }
    }
    fprintf(stderr, "API dots: opaque_pixels=%d\n", opaque);

    wtz_image_free(tile);
    return 0;
}
