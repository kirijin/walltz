// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WALLTZ_ENGINE_H
#define WALLTZ_ENGINE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque image type
typedef struct {
    uint8_t *pixels;     // RGBA, premultiplied alpha
    int width;
    int height;
    int stride;          // bytes per row
} WtzImage;

// Render parameters (mirrors RenderSnapshot from Qt version)
typedef struct {
    // Target canvas
    int target_width;
    int target_height;

    // Background mode
    int blur_mode;           // 1 = blur, 0 = solid/gradient
    int bg_gradient_style;   // 0 = solid, 1 = preset, 2 = mood

    // Blur
    double bg_zoom;
    double bg_blur_angle;
    int blur_radius;         // 0 = auto
    double saturation_factor;
    double overlay_opacity;
    uint32_t overlay_color;  // 0xAARRGGBB
    double blur_brightness;

    // Solid / gradient background
    int auto_color;
    uint32_t bg_color;
    int bg_gradient_preset;
    double gradient_angle;

    // Pre-resolved gradient endpoints
    uint32_t mood_color_a;
    uint32_t mood_color_b;

    // Patterns
    int bg_pattern_enabled;
    int bg_pattern_type;
    uint32_t bg_pattern_color;
    double bg_pattern_scale;
    double bg_pattern_rotation;
    double bg_pattern_spacing;
    int bg_pattern_random_rotate;
    int bg_pattern_jitter;
    double bg_pattern_grid_amplitude;
    int bg_pattern_mix_enabled;
    int *bg_pattern_mix_motifs;
    int bg_pattern_mix_count;

    // Effects
    double vignette_strength;
    double grain_strength;
    double ca_strength;
    double color_gamma;
    double color_warmth;
    double color_black_lift;
    double texture_opacity;
    int texture_blend_mode;
    int texture_over_photo;
    int texture_kind;
    uint32_t texture_color;
    int photo_frame;
    int photo_frame_width;
    double fg_zoom;
    double pip_zoom;
    int photo_grade;

    // Mood
    int auto_mood;
    int use_v2;
} WtzRenderParams;

// Image I/O
WtzImage* wtz_image_load(const char *path);
WtzImage* wtz_image_new(int width, int height);
void wtz_image_free(WtzImage *img);
int wtz_image_save_png(const WtzImage *img, const char *path);

// Core render
WtzImage* wtz_render(const WtzImage *src, const WtzRenderParams *params);

// Blur (adaptive dispatch: Gaussian for σ≤50, box cascade for σ>50)
void wtz_blur_image(WtzImage *image, double sigma, double saturationFactor,
                     double overlayOpacity, uint32_t overlayColor, double brightness,
                     double colorGamma, double colorWarmth, double colorBlackLift);

// Mood palette extraction
typedef struct {
    uint32_t color_a;
    uint32_t color_b;
} WtzMoodPair;

typedef struct {
    WtzMoodPair moods[6];  // Auto, Soft, Vivid, Warm, Cool, Deep
    WtzMoodPair v2_moods[6]; // Dynamic, Tonal, Vibrant, Ember, Glacier, Shadow
} WtzMoodPalettes;

WtzMoodPalettes* wtz_extract_mood_palettes(const WtzImage *img);
void wtz_mood_palettes_free(WtzMoodPalettes *palettes);

// Smart auto params
typedef struct {
    double sigma;
    double sat_boost;
    double brightness;
    double overlay_opacity;
    uint32_t overlay_color;
    double vignette;
    double grain;
} WtzSmartAutoParams;

void wtz_compute_smart_auto(const WtzImage *img, WtzSmartAutoParams *result);

// Pattern tile generation
WtzImage* wtz_generate_pattern_tile(int kind, int index, int tile_size,
                                    uint32_t fg_color, double scale);

// Preset definitions
typedef struct {
    const char *id;
    const char *display_name;
    double sigma;
    double sat_boost;
    double brightness;
    double overlay_opacity;
    uint32_t overlay_color;
    double vignette;
    double grain;
    int frame_enabled;
    int frame_width_pct;
    double gamma;
    double warmth;
    double black_lift;
    int texture_kind;
    double texture_opacity;
    int texture_blend_mode;
    int texture_over_photo;
    uint32_t texture_color;
    int photo_grade;
} WtzBlurPreset;

int wtz_blur_preset_count(void);
const WtzBlurPreset* wtz_blur_preset(int index);
const WtzBlurPreset* wtz_blur_preset_for_id(const char *id);

// Gradient presets
typedef struct {
    const char *name;
    uint32_t color1;
    uint32_t color2;
} WtzGradientPreset;

int wtz_gradient_preset_count(void);
const WtzGradientPreset* wtz_gradient_preset(int index);

#ifdef __cplusplus
}
#endif

#endif // WALLTZ_ENGINE_H
