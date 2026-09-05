// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Mood palette extraction — hue histogram + 3D RGB histogram

#include "engine.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ── V1: Hue histogram mood palettes ───────────────────────────────────────

static double lum(double r, double g, double b) {
    return 0.299 * r + 0.587 * g + 0.114 * b;
}

WtzMoodPalettes* wtz_extract_mood_palettes(const WtzImage *img) {
    WtzMoodPalettes *palettes = g_new0(WtzMoodPalettes, 1);
    
    // Initialize defaults
    for (int i = 0; i < 6; i++) {
        palettes->moods[i].color_a = 0xFF808080;
        palettes->moods[i].color_b = 0xFFB4B4B4;
        palettes->v2_moods[i].color_a = 0xFF808080;
        palettes->v2_moods[i].color_b = 0xFFB4B4B4;
    }
    
    if (!img || !img->pixels) return palettes;
    
    int w = img->width, h = img->height;
    int step = MAX(1, MAX(w, h) / 64);
    int hue_bins = 24;
    double hue_weight[24] = {0};
    double hue_sat[24] = {0};
    double hue_light[24] = {0};
    int hue_count[24] = {0};
    float max_sat = 0.0f;
    int key_r = 128, key_g = 128, key_b = 128;
    
    for (int y = 0; y < h; y += step) {
        const uint8_t *row = img->pixels + y * img->stride;
        for (int x = 0; x < w; x += step) {
            uint8_t r = row[x * 4 + 2];
            uint8_t g = row[x * 4 + 1];
            uint8_t b = row[x * 4 + 0];
            int mn = MIN(MIN(r, g), b);
            int mx = MAX(MAX(r, g), b);
            float sat = (mx == 0) ? 0.0f : (mx - mn) / (float)mx;
            float lgt = (mx + mn) / 510.0f;
            if (sat < 0.15f || lgt < 0.15f || lgt > 0.90f) continue;
            
            float hue = 0.0f;
            float delta = mx - mn;
            if (delta > 0) {
                if (mx == r) hue = (g - b) / delta;
                else if (mx == g) hue = 2.0f + (b - r) / delta;
                else hue = 4.0f + (r - g) / delta;
                hue /= 6.0f;
                if (hue < 0) hue += 1.0f;
            }
            int bin = MIN((int)(hue * hue_bins), hue_bins - 1);
            double wgt = MAX(sat * 100.0, 1.0);
            hue_weight[bin] += wgt;
            hue_sat[bin] += sat * wgt;
            hue_light[bin] += lgt * wgt;
            hue_count[bin]++;
            
            float score = sat * MAX(0.0f, lgt - 0.15f) * 1.5f;
            if (score > max_sat) { max_sat = score; key_r = r; key_g = g; key_b = b; }
        }
    }
    
    // Find best two bins
    int best1 = 0, best2 = 0;
    double best_w1 = 0, best_w2 = 0;
    for (int i = 0; i < hue_bins; i++) {
        if (hue_weight[i] > best_w1) { best_w2 = best_w1; best2 = best1; best_w1 = hue_weight[i]; best1 = i; }
        else if (hue_weight[i] > best_w2) { best_w2 = hue_weight[i]; best2 = i; }
    }
    
    if (best_w1 < 1.0) return palettes;
    
    // Mood 0: Auto
    float h1 = (best1 + 0.5f) / hue_bins;
    float h2 = (best_w2 > 1.0f) ? (best2 + 0.5f) / hue_bins : fmodf(h1 + 0.38197f, 1.0f);
    palettes->moods[0].color_a = ((uint32_t)(h1 * 360) << 16) | 0xFF000000;
    palettes->moods[0].color_b = ((uint32_t)(h2 * 360) << 16) | 0xFF000000;
    
    // Mood 5: Deep (lowest lightness)
    int db = best1;
    double ml = 1.0;
    for (int i = 0; i < hue_bins; i++) {
        if (hue_weight[i] <= 0) continue;
        float l = hue_light[i] / hue_weight[i];
        if (l < ml) { ml = l; db = i; }
    }
    float hd = (db + 0.5f) / hue_bins;
    palettes->moods[5].color_a = ((uint32_t)(hd * 360) << 16) | 0xFF000000;
    palettes->moods[5].color_b = ((uint32_t)(fmodf(hd + 0.5f, 1.0f) * 360) << 16) | 0xFF000000;
    
    // TODO: fill remaining moods (Soft/Vivid/Warm/Cool) with proper extraction
    
    return palettes;
}

void wtz_mood_palettes_free(WtzMoodPalettes *palettes) {
    g_free(palettes);
}
