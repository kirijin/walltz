// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Standalone test for mood extraction - compiles directly with engine sources

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Minimal types from engine.h
typedef struct {
    uint8_t *pixels;
    int width;
    int height;
    int stride;
} WtzImage;

typedef struct {
    uint32_t color_a;
    uint32_t color_b;
} WtzMoodPair;

typedef struct {
    WtzMoodPair moods[6];
    WtzMoodPair v2_moods[6];
} WtzMoodPalettes;

// Forward declare the function we're testing
WtzMoodPalettes* wtz_extract_mood_palettes(const WtzImage *img);

int main() {
    setlinebuf(stdout);
    printf("=== Standalone mood test ===\n");

    WtzImage img;
    img.width = 100;
    img.height = 100;
    img.stride = 400;
    img.pixels = malloc(400 * 100);

    for (int y = 0; y < 100; y++) {
        uint8_t *row = img.pixels + y * img.stride;
        for (int x = 0; x < 100; x++) {
            if (y < 50) {
                row[x * 4 + 0] = 200; // B
                row[x * 4 + 1] = 150; // G
                row[x * 4 + 2] = 50;  // R
            } else {
                row[x * 4 + 0] = 50;
                row[x * 4 + 1] = 180;
                row[x * 4 + 2] = 50;
            }
            row[x * 4 + 3] = 255;
        }
    }

    printf("Calling wtz_extract_mood_palettes...\n");
    WtzMoodPalettes *palettes = wtz_extract_mood_palettes(&img);
    printf("Returned from wtz_extract_mood_palettes\n");

    if (!palettes) {
        printf("FAIL: returned NULL\n");
        free(img.pixels);
        return 1;
    }

    printf("\nV1 Moods:\n");
    for (int i = 0; i < 6; i++) {
        printf("  Mood %d: A=#%08X B=#%08X\n", i, palettes->moods[i].color_a, palettes->moods[i].color_b);
    }

    uint32_t auto_a = palettes->moods[0].color_a;
    if (auto_a == 0xFF808080) {
        printf("\nFAIL: Mood 0 (Auto) is default gray\n");
        free(img.pixels);
        return 1;
    }

    printf("\nOK: Mood extraction working (Auto A=#%08X)\n", auto_a);
    free(img.pixels);
    return 0;
}
