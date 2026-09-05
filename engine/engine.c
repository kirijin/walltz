// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ── Factory defaults (single source of truth) ─────────────────────────────
#define WTZ_BLUR_RADIUS       90
#define WTZ_SATURATION        1.8
#define WTZ_BLUR_BRIGHTNESS   1.0
#define WTZ_COLOR_GAMMA       1.0
#define WTZ_COLOR_WARMTH      0.0
#define WTZ_COLOR_BLACK_LIFT  0.0
#define WTZ_TEXTURE_BLEND_MODE 13  // Multiply
#define WTZ_OVERLAY_OPACITY   0.0
#define WTZ_BG_ZOOM           1.0
#define WTZ_BG_BLUR_ANGLE     0.0
#define WTZ_GRADIENT_ANGLE    45.0
#define WTZ_FG_ZOOM           0.8
#define WTZ_PIP_ZOOM          1.0
#define WTZ_VIGNETTE          0.0
#define WTZ_GRAIN             0.0
#define WTZ_CA                0.0
#define WTZ_PHOTO_FRAME_WIDTH 0
#define WTZ_BLUR_RADIUS_MAX   120
#define WTZ_CANVAS_MARGIN     0.05
#define WTZ_MIN_ZOOM          0.5

// ── Image I/O (stubs — will use libpng/libjpeg in production) ─────────────

WtzImage* wtz_image_new(int width, int height) {
    WtzImage *img = g_new0(WtzImage, 1);
    img->width = width;
    img->height = height;
    img->stride = width * 4;
    img->pixels = g_new0(uint8_t, img->stride * height);
    return img;
}

void wtz_image_free(WtzImage *img) {
    if (!img) return;
    g_free(img->pixels);
    g_free(img);
}

WtzImage* wtz_image_load(const char *path) {
    // TODO: implement with GdkPixbuf or libpng
    // For now, return NULL
    return NULL;
}

int wtz_image_save_png(const WtzImage *img, const char *path) {
    // TODO: implement with libpng
    return 0;
}

// ── Core render (stub — full implementation in subsequent files) ──────────

WtzImage* wtz_render(const WtzImage *src, const WtzRenderParams *params) {
    if (!src || !params) return NULL;
    
    int W = params->target_width;
    int H = params->target_height;
    if (W < 1 || H < 1) return NULL;
    
    WtzImage *output = wtz_image_new(W, H);
    if (!output) return NULL;
    
    // TODO: full render pipeline
    // For now, fill with background color
    uint8_t a = (params->bg_color >> 24) & 0xFF;
    uint8_t r = (params->bg_color >> 16) & 0xFF;
    uint8_t g = (params->bg_color >> 8) & 0xFF;
    uint8_t b = params->bg_color & 0xFF;
    
    for (int y = 0; y < H; y++) {
        uint8_t *row = output->pixels + y * output->stride;
        for (int x = 0; x < W; x++) {
            row[x * 4 + 0] = b;
            row[x * 4 + 1] = g;
            row[x * 4 + 2] = r;
            row[x * 4 + 3] = a;
        }
    }
    
    return output;
}

// ── Mood palettes (stub) ──────────────────────────────────────────────────

WtzMoodPalettes* wtz_extract_mood_palettes(const WtzImage *img) {
    WtzMoodPalettes *palettes = g_new0(WtzMoodPalettes, 1);
    // TODO: implement hue/RGB histogram extraction
    for (int i = 0; i < 6; i++) {
        palettes->moods[i].color_a = 0xFF808080;
        palettes->moods[i].color_b = 0xFFB4B4B4;
        palettes->v2_moods[i].color_a = 0xFF808080;
        palettes->v2_moods[i].color_b = 0xFFB4B4B4;
    }
    return palettes;
}

void wtz_mood_palettes_free(WtzMoodPalettes *palettes) {
    g_free(palettes);
}

// ── Smart auto (stub) ─────────────────────────────────────────────────────

WtzSmartAutoParams wtz_compute_smart_auto(const WtzImage *img) {
    WtzSmartAutoParams p = {
        .sigma = 15,
        .sat_boost = 0.85,
        .brightness = 0.92,
        .overlay_opacity = 0.35,
        .overlay_color = 0xFF1A1A1A,
        .vignette = 0,
        .grain = 0,
    };
    return p;
}

// ── Pattern tile (stub) ───────────────────────────────────────────────────

WtzImage* wtz_generate_pattern_tile(int kind, int index, int tile_size,
                                    uint32_t fg_color, double scale) {
    WtzImage *tile = wtz_image_new(tile_size, tile_size);
    if (!tile) return NULL;
    
    // TODO: implement pattern generation
    // For now, fill with transparent
    memset(tile->pixels, 0, tile->stride * tile_size);
    
    return tile;
}

// ── Blur presets ──────────────────────────────────────────────────────────

static const WtzBlurPreset s_presets[] = {
    // id, display, sigma, sat, bright, ovlOp, ovlColor, vig, grain, frameEn, frameW, gamma, warmth, lift, texKind, texOp, texBlend, texOver, texColor, photo
    { "default", "Default", WTZ_BLUR_RADIUS, WTZ_SATURATION, WTZ_BLUR_BRIGHTNESS,
      WTZ_OVERLAY_OPACITY, 0x000000, WTZ_VIGNETTE, WTZ_GRAIN, FALSE, 0,
      WTZ_COLOR_GAMMA, WTZ_COLOR_WARMTH, WTZ_COLOR_BLACK_LIFT,
      0, 0.0, WTZ_TEXTURE_BLEND_MODE, FALSE, 0, FALSE },
    { "auto", "Auto", 0, 1.0, 1.0, 0.0, 0x000000, 0.0, 0.0, FALSE, 0,
      1.0, 0.0, 0.0, 0, 0.0, 13, FALSE, 0, FALSE },
    { "serenity", "Serenity", 25, 0.30, 0.65, 0.55, 0x181824, 0.15, 0.0,
      FALSE, 0, 1.0, 0.0, 0.0, 0, 0.0, 13, FALSE, 0, FALSE },
    { "focus", "Focus", 8, 0.55, 0.90, 0.15, 0x1C1C1C, 0.0, 0.0,
      FALSE, 0, 1.0, 0.0, 0.0, 0, 0.0, 13, FALSE, 0, FALSE },
    { "comfort", "Comfort", 20, 0.80, 0.78, 0.35, 0x2A1F14, 0.25, 0.02,
      FALSE, 0, 1.0, 0.0, 0.0, 0, 0.0, 13, FALSE, 0, FALSE },
    { "apple", "Apple", 30, 1.8, 1.02, 0.50, 0x1A1A1A, 0.0, 0.0,
      FALSE, 0, 1.0, 0.0, 0.0, 0, 0.0, 13, FALSE, 0, FALSE },
    { "gnome", "GNOME", 30, 1.0, 0.60, 0.0, 0x000000, 0.0, 0.0,
      FALSE, 0, 1.0, 0.0, 0.0, 0, 0.0, 13, FALSE, 0, FALSE },
    { "mica", "Mica", 10, 0.9, 0.85, 0.30, 0x323232, 0.0, 0.0,
      FALSE, 0, 1.0, 0.0, 0.0, 0, 0.0, 13, FALSE, 0, FALSE },
    { "acrylic", "Acrylic", 30, 1.0, 0.90, 0.60, 0x202020, 0.0, 0.03,
      FALSE, 0, 1.0, 0.0, 0.0, 0, 0.0, 13, FALSE, 0, FALSE },
    { "reddit", "Reddit", 24, 1.0, 1.0, 0.70, 0x0B0E0F, 0.0, 0.0,
      TRUE, 1, 1.0, 0.0, 0.0, 0, 0.0, 13, FALSE, 0, FALSE },
    { "kodachrome", "Kodachrome", 90, 1.5, 1.0, 0.0, 0x000000, 0.0, 0.02,
      FALSE, 0, 1.08, 0.22, 0.05, 1, 0.20, 13, TRUE, 0xE8A050, TRUE },
    { "polaroid", "Polaroid", 90, 0.9, 1.0, 0.0, 0x000000, 0.15, 0.03,
      TRUE, 3, 0.94, -0.04, 0.18, 0, 0.0, 13, FALSE, 0x000000, TRUE },
    { "vintage", "Vintage", 90, 1.1, 1.0, 0.0, 0x000000, 0.20, 0.04,
      TRUE, 1, 0.96, 0.25, 0.12, 1, 0.28, 13, TRUE, 0xC87030, TRUE },
    { "trix", "Tri-X", 90, 0.0, 1.0, 0.0, 0x000000, 0.25, 0.08,
      TRUE, 1, 1.10, 0.0, 0.02, 3, 0.90, 13, TRUE, 0x000000, TRUE },
    { "coolfilm", "Cool Film", 90, 1.1, 1.0, 0.0, 0x000000, 0.10, 0.02,
      FALSE, 0, 1.03, -0.20, 0.08, 1, 0.18, 13, TRUE, 0x70A8D0, TRUE },
};

int wtz_blur_preset_count(void) {
    return sizeof(s_presets) / sizeof(s_presets[0]);
}

const WtzBlurPreset* wtz_blur_preset(int index) {
    if (index < 0 || index >= wtz_blur_preset_count()) return &s_presets[0];
    return &s_presets[index];
}

const WtzBlurPreset* wtz_blur_preset_for_id(const char *id) {
    for (int i = 0; i < wtz_blur_preset_count(); i++) {
        if (strcmp(s_presets[i].id, id) == 0) return &s_presets[i];
    }
    return &s_presets[0];
}

// ── Gradient presets ──────────────────────────────────────────────────────

static const WtzGradientPreset s_gradient_presets[] = {
    { "Sunset Warmth", 0xFFFF6B6B, 0xFFFECA57 },
    { "Coral Reef", 0xFFFF6B6B, 0xFF48DFB },
    { "Lemonade", 0xFFFDCB6E, 0xFF00CEC9 },
    { "Ocean Depths", 0xFF0ABDE3, 0xFF48DFB },
    { "Tokyo Night", 0xFF1A1B26, 0xFF7AA2F7 },
    { "Arctic", 0xFF2E3440, 0xFF88C0D0 },
    { "Catppuccin", 0xFF1E1E2E, 0xFFCBA6F7 },
    { "Gruvbox", 0xFF282828, 0xFF8F3F1A },
    { "Solarized", 0xFF073642, 0xFF268BD2 },
    { "Dusk", 0xFF6C5CE7, 0xFFFD79A8 },
    { "Everforest", 0xFF2B3339, 0xFFA7C080 },
    { "Grayscale", 0xFF444444, 0xFFCCCCCC },
};

int wtz_gradient_preset_count(void) {
    return sizeof(s_gradient_presets) / sizeof(s_gradient_presets[0]);
}

const WtzGradientPreset* wtz_gradient_preset(int index) {
    if (index < 0 || index >= wtz_gradient_preset_count()) return &s_gradient_presets[0];
    return &s_gradient_presets[index];
}
