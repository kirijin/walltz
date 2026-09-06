// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Test mood palette extraction

#include "engine.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Create a test image: blue sky with green grass (top half blue, bottom half green)
    WtzImage *img = wtz_image_new(100, 100);
    if (!img) { printf("FAIL: wtz_image_new\n"); return 1; }

    for (int y = 0; y < 100; y++) {
        uint8_t *row = img->pixels + y * img->stride;
        for (int x = 0; x < 100; x++) {
            if (y < 50) {
                // Sky blue
                row[x * 4 + 0] = 200; // B
                row[x * 4 + 1] = 150; // G
                row[x * 4 + 2] = 50;  // R
            } else {
                // Grass green
                row[x * 4 + 0] = 50;  // B
                row[x * 4 + 1] = 180; // G
                row[x * 4 + 2] = 50;  // R
            }
            row[x * 4 + 3] = 255; // A
        }
    }

    printf("Calling wtz_extract_mood_palettes...\n");
    fflush(stdout);
    
    fprintf(stderr, "TEST: about to call wtz_extract_mood_palettes\n");
    WtzMoodPalettes *palettes = wtz_extract_mood_palettes(img);
    fprintf(stderr, "TEST: returned from wtz_extract_mood_palettes, palettes=%p\n", (void*)palettes);
    fflush(stderr);
    
    printf("Returned from wtz_extract_mood_palettes\n");
    fflush(stdout);
    
    if (!palettes) { printf("FAIL: wtz_extract_mood_palettes returned NULL\n"); wtz_image_free(img); return 1; }

    printf("V1 Moods:\n");
    for (int i = 0; i < 6; i++) {
        uint32_t a = palettes->moods[i].color_a;
        uint32_t b = palettes->moods[i].color_b;
        printf("  Mood %d: A=#%08X B=#%08X\n", i, a, b);
    }

    printf("\nV2 Moods:\n");
    for (int i = 0; i < 6; i++) {
        uint32_t a = palettes->v2_moods[i].color_a;
        uint32_t b = palettes->v2_moods[i].color_b;
        printf("  Mood %d: A=#%08X B=#%08X\n", i, a, b);
    }

    // Verify mood 0 (Auto) is not default gray
    uint32_t auto_a = palettes->moods[0].color_a;
    if (auto_a == 0xFF808080) {
        printf("\nFAIL: Mood 0 (Auto) is default gray — extraction failed\n");
        wtz_image_free(img);
        return 1;
    }

    printf("\nOK: Mood extraction working (Auto A=#%08X)\n", auto_a);

    // Test Smart Auto
    WtzSmartAutoParams smart;
    wtz_compute_smart_auto (img, &smart);
    printf("\nSmart Auto: sigma=%.1f sat=%.2f bright=%.2f overlay=%.2f\n",
           smart.sigma, smart.sat_boost, smart.brightness, smart.overlay_opacity);

    wtz_image_free(img);
    return 0;
}
