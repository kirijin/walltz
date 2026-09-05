// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#define _GNU_SOURCE

// Render composition — the full pipeline that turns a source image into a wallpaper.
// This is the heart of walltz: background → pattern → effects → shadow → frame → foreground → CA.

#include "engine.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ── Factory defaults ──────────────────────────────────────────────────────
#define WTZ_BLUR_RADIUS       90
#define WTZ_SATURATION        1.8
#define WTZ_BLUR_BRIGHTNESS   1.0
#define WTZ_COLOR_GAMMA       1.0
#define WTZ_COLOR_WARMTH      0.0
#define WTZ_COLOR_BLACK_LIFT  0.0
#define WTZ_TEXTURE_BLEND_MODE 13
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

// ── Gradient presets ──────────────────────────────────────────────────────

typedef struct {
    uint32_t color1;
    uint32_t color2;
} GradientPreset;

static const GradientPreset s_gradient_presets[] = {
    { 0xFFFF6B6B, 0xFFFECA57 }, // Sunset Warmth
    { 0xFFFF6B6B, 0xFF48DFB },  // Coral Reef
    { 0xFFFDCB6E, 0xFF00CEC9 }, // Lemonade
    { 0xFF0ABDE3, 0xFF48DFB },  // Ocean Depths
    { 0xFF1A1B26, 0xFF7AA2F7 }, // Tokyo Night
    { 0xFF2E3440, 0xFF88C0D0 }, // Arctic
    { 0xFF1E1E2E, 0xFFCBA6F7 }, // Catppuccin
    { 0xFF282828, 0xFF8F3F1A }, // Gruvbox
    { 0xFF073642, 0xFF268BD2 }, // Solarized
    { 0xFF6C5CE7, 0xFFFD79A8 }, // Dusk
    { 0xFF2B3339, 0xFFA7C080 }, // Everforest
    { 0xFF444444, 0xFFCCCCCC }, // Grayscale
};

// ── Helper: fill rect with color ──────────────────────────────────────────

static void fill_rect(WtzImage *img, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > img->width) ? img->width : x + w;
    int y1 = (y + h > img->height) ? img->height : y + h;
    for (int row = y0; row < y1; row++) {
        uint8_t *line = img->pixels + row * img->stride;
        for (int col = x0; col < x1; col++) {
            line[col * 4 + 0] = b;
            line[col * 4 + 1] = g;
            line[col * 4 + 2] = r;
            line[col * 4 + 3] = a;
        }
    }
}

// ── Helper: draw rounded rect (simple, no antialiasing for now) ────────────

static void draw_rounded_rect(WtzImage *img, int x, int y, int w, int h, int radius,
                               uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    // Fill main body
    fill_rect(img, x + radius, y, w - 2 * radius, h, r, g, b, a);
    fill_rect(img, x, y + radius, w, h - 2 * radius, r, g, b, a);
    // Corners (simple circles)
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                int px, py;
                // Top-left
                px = x + radius + dx; py = y + radius + dy;
                if (px >= 0 && px < img->width && py >= 0 && py < img->height) {
                    uint8_t *p = img->pixels + py * img->stride + px * 4;
                    p[0] = b; p[1] = g; p[2] = r; p[3] = a;
                }
                // Top-right
                px = x + w - radius + dx; py = y + radius + dy;
                if (px >= 0 && px < img->width && py >= 0 && py < img->height) {
                    uint8_t *p = img->pixels + py * img->stride + px * 4;
                    p[0] = b; p[1] = g; p[2] = r; p[3] = a;
                }
                // Bottom-left
                px = x + radius + dx; py = y + h - radius + dy;
                if (px >= 0 && px < img->width && py >= 0 && py < img->height) {
                    uint8_t *p = img->pixels + py * img->stride + px * 4;
                    p[0] = b; p[1] = g; p[2] = r; p[3] = a;
                }
                // Bottom-right
                px = x + w - radius + dx; py = y + h - radius + dy;
                if (px >= 0 && px < img->width && py >= 0 && py < img->height) {
                    uint8_t *p = img->pixels + py * img->stride + px * 4;
                    p[0] = b; p[1] = g; p[2] = r; p[3] = a;
                }
            }
        }
    }
}

// ── Helper: copy image region ─────────────────────────────────────────────

static void copy_image(WtzImage *dst, int dst_x, int dst_y,
                        const WtzImage *src, int src_x, int src_y, int w, int h) {
    for (int row = 0; row < h; row++) {
        int sy = src_y + row;
        int dy = dst_y + row;
        if (sy < 0 || sy >= src->height || dy < 0 || dy >= dst->height) continue;
        const uint8_t *src_row = src->pixels + sy * src->stride;
        uint8_t *dst_row = dst->pixels + dy * dst->stride;
        for (int col = 0; col < w; col++) {
            int sx = src_x + col;
            int dx = dst_x + col;
            if (sx < 0 || sx >= src->width || dx < 0 || dx >= dst->width) continue;
            uint8_t sa = src_row[sx * 4 + 3];
            if (sa == 0) continue;
            if (sa == 255) {
                dst_row[dx * 4 + 0] = src_row[sx * 4 + 0];
                dst_row[dx * 4 + 1] = src_row[sx * 4 + 1];
                dst_row[dx * 4 + 2] = src_row[sx * 4 + 2];
                dst_row[dx * 4 + 3] = 255;
            } else {
                uint8_t da = dst_row[dx * 4 + 3];
                uint8_t out_a = sa + da * (255 - sa) / 255;
                if (out_a == 0) continue;
                for (int c = 0; c < 3; c++) {
                    dst_row[dx * 4 + c] = (src_row[sx * 4 + c] * sa + dst_row[dx * 4 + c] * da * (255 - sa) / 255) / out_a;
                }
                dst_row[dx * 4 + 3] = out_a;
            }
        }
    }
}

// ── Background: blur mode ─────────────────────────────────────────────────

static void render_background_blur(WtzImage *output, const WtzImage *src,
                                    const WtzRenderParams *params) {
    int W = output->width, H = output->height;
    double fill_zoom = (double)W / src->width;
    if ((double)H / src->height > fill_zoom) fill_zoom = (double)H / src->height;
    fill_zoom *= params->bg_zoom;

    // Fill with white or mood color
    if (params->bg_zoom < 1.0) {
        uint8_t a = (params->mood_color_a >> 24) & 0xFF;
        uint8_t r = (params->mood_color_a >> 16) & 0xFF;
        uint8_t g = (params->mood_color_a >> 8) & 0xFF;
        uint8_t b = params->mood_color_a & 0xFF;
        fill_rect(output, 0, 0, W, H, r, g, b, a);
    } else {
        fill_rect(output, 0, 0, W, H, 255, 255, 255, 255);
    }

    // Draw scaled source image centered
    int bg_w = (int)(src->width * fill_zoom);
    int bg_h = (int)(src->height * fill_zoom);
    int bg_x = (W - bg_w) / 2;
    int bg_y = (H - bg_h) / 2;

    // Simple bilinear scale
    for (int y = 0; y < bg_h; y++) {
        int sy = (int)((double)y / bg_h * src->height);
        if (sy >= src->height) sy = src->height - 1;
        uint8_t *dst_row = output->pixels + (bg_y + y) * output->stride;
        const uint8_t *src_row = src->pixels + sy * src->stride;
        for (int x = 0; x < bg_w; x++) {
            int sx = (int)((double)x / bg_w * src->width);
            if (sx >= src->width) sx = src->width - 1;
            int dx = bg_x + x;
            if (dx < 0 || dx >= W) continue;
            dst_row[dx * 4 + 0] = src_row[sx * 4 + 0];
            dst_row[dx * 4 + 1] = src_row[sx * 4 + 1];
            dst_row[dx * 4 + 2] = src_row[sx * 4 + 2];
            dst_row[dx * 4 + 3] = 255;
        }
    }

    // Darken overlay
    for (int y = 0; y < H; y++) {
        uint8_t *row = output->pixels + y * output->stride;
        for (int x = 0; x < W; x++) {
            for (int c = 0; c < 3; c++) {
                row[x * 4 + c] = (uint8_t)(row[x * 4 + c] * 0.9); // 10% darken
            }
        }
    }

    // Apply blur via float pipeline
    double sigma = params->blur_radius > 0 ? params->blur_radius : 0.017 * H;
    wtz_blur_image(output, sigma, params->saturation_factor,
                    params->overlay_opacity, params->overlay_color, params->blur_brightness,
                    params->color_gamma, params->color_warmth, params->color_black_lift);
}

// ── Background: solid color ───────────────────────────────────────────────

static void render_background_solid(WtzImage *output, const WtzRenderParams *params) {
    uint8_t a, r, g, b;
    if (params->auto_color) {
        a = (params->mood_color_a >> 24) & 0xFF;
        r = (params->mood_color_a >> 16) & 0xFF;
        g = (params->mood_color_a >> 8) & 0xFF;
        b = params->mood_color_a & 0xFF;
    } else {
        a = (params->bg_color >> 24) & 0xFF;
        r = (params->bg_color >> 16) & 0xFF;
        g = (params->bg_color >> 8) & 0xFF;
        b = params->bg_color & 0xFF;
    }
    fill_rect(output, 0, 0, output->width, output->height, r, g, b, a);
}

// ── Background: gradient ──────────────────────────────────────────────────

static void render_background_gradient(WtzImage *output, const WtzRenderParams *params) {
    int W = output->width, H = output->height;
    uint32_t c1, c2;

    if (params->bg_gradient_style == 1) {
        // Preset gradient
        int idx = params->bg_gradient_preset;
        if (idx < 0) idx = 0;
        if (idx >= 12) idx = 11;
        c1 = s_gradient_presets[idx].color1;
        c2 = s_gradient_presets[idx].color2;
    } else {
        // Mood gradient
        c1 = params->mood_color_a;
        c2 = params->mood_color_b;
    }

    uint8_t r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    uint8_t r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;

    double angle = params->gradient_angle * M_PI / 180.0;
    double dx = cos(angle), dy = sin(angle);
    double half_len = fabs(W / 2.0 * dx) + fabs(H / 2.0 * dy);

    for (int y = 0; y < H; y++) {
        uint8_t *row = output->pixels + y * output->stride;
        for (int x = 0; x < W; x++) {
            double t = ((x - W / 2.0) * dx + (y - H / 2.0) * dy) / half_len + 0.5;
            if (t < 0) t = 0;
            if (t > 1) t = 1;
            row[x * 4 + 0] = (uint8_t)(b1 + (b2 - b1) * t);
            row[x * 4 + 1] = (uint8_t)(g1 + (g2 - g1) * t);
            row[x * 4 + 2] = (uint8_t)(r1 + (r2 - r1) * t);
            row[x * 4 + 3] = 255;
        }
    }
}

// ── Vignette effect ───────────────────────────────────────────────────────

static void apply_vignette(WtzImage *img, double strength) {
    if (strength < 0.001) return;
    int W = img->width, H = img->height;
    double cx = W / 2.0, cy = H / 2.0;
    double max_dist = sqrt(cx * cx + cy * cy);

    for (int y = 0; y < H; y++) {
        uint8_t *row = img->pixels + y * img->stride;
        for (int x = 0; x < W; x++) {
            double dx = x - cx, dy = y - cy;
            double dist = sqrt(dx * dx + dy * dy) / max_dist;
            double factor = 1.0 - strength * dist * dist;
            if (factor < 0) factor = 0;
            for (int c = 0; c < 3; c++) {
                row[x * 4 + c] = (uint8_t)(row[x * 4 + c] * factor);
            }
        }
    }
}

// ── Grain effect ──────────────────────────────────────────────────────────

static void apply_grain(WtzImage *img, double strength) {
    if (strength < 0.001) return;
    int W = img->width, H = img->height;
    int intensity = (int)(15 * strength);

    // Use a simple LCG for thread safety
    static uint32_t seed = 12345;
    seed = seed * 1103515245 + 12345;

    for (int y = 0; y < H; y++) {
        uint8_t *row = img->pixels + y * img->stride;
        for (int x = 0; x < W; x++) {
            seed = seed * 1103515245 + 12345;
            int noise = (int)((seed >> 16) % (intensity * 2 + 1)) - intensity;
            for (int c = 0; c < 3; c++) {
                int val = row[x * 4 + c] + noise;
                row[x * 4 + c] = (val < 0) ? 0 : (val > 255) ? 255 : val;
            }
        }
    }
}

// ── Chromatic aberration ──────────────────────────────────────────────────

static void apply_chromatic_aberration(WtzImage *img, double strength) {
    if (strength < 0.001) return;
    int W = img->width, H = img->height;
    double max_shift = strength * (W < H ? W : H) * 0.05;
    double cx = W / 2.0, cy = H / 2.0;
    double max_dist = sqrt(cx * cx + cy * cy);

    WtzImage *temp = wtz_image_new(W, H);
    if (!temp) return;

    for (int y = 0; y < H; y++) {
        uint8_t *dst_row = temp->pixels + y * temp->stride;
        for (int x = 0; x < W; x++) {
            double dx = (x - cx) / max_dist;
            double dy = (y - cy) / max_dist;
            int shift = (int)(sqrt(dx * dx + dy * dy) * max_shift);
            int sx = (int)(dx * shift), sy = (int)(dy * shift);

            int rx = x + sx, ry = y + sy;
            int bx = x - sx, by = y - sy;
            rx = (rx < 0) ? 0 : (rx >= W) ? W - 1 : rx;
            ry = (ry < 0) ? 0 : (ry >= H) ? H - 1 : ry;
            bx = (bx < 0) ? 0 : (bx >= W) ? W - 1 : bx;
            by = (by < 0) ? 0 : (by >= H) ? H - 1 : by;

            const uint8_t *rPx = img->pixels + ry * img->stride + rx * 4;
            const uint8_t *bPx = img->pixels + by * img->stride + bx * 4;
            const uint8_t *srcPx = img->pixels + y * img->stride + x * 4;

            dst_row[x * 4 + 0] = bPx[0];  // B shifts inward
            dst_row[x * 4 + 1] = srcPx[1]; // G unchanged
            dst_row[x * 4 + 2] = rPx[2];  // R shifts outward
            dst_row[x * 4 + 3] = srcPx[3]; // A unchanged
        }
    }

    memcpy(img->pixels, temp->pixels, img->stride * H);
    wtz_image_free(temp);
}

// ── Shadow ────────────────────────────────────────────────────────────────

static void apply_shadow(WtzImage *img, int cx, int cy, int w, int h, int radius) {
    // Simple shadow: darken area around the rect
    int shadow_size = radius * 3;
    for (int dy = -shadow_size; dy < h + shadow_size; dy++) {
        for (int dx = -shadow_size; dx < w + shadow_size; dx++) {
            int x = cx + dx;
            int y = cy + dy;
            if (x < 0 || x >= img->width || y < 0 || y >= img->height) continue;
            if (dx >= 0 && dx < w && dy >= 0 && dy < h) continue; // Skip the rect itself

            double dist = 0;
            if (dx < 0) dist = -dx;
            else if (dx >= w) dist = dx - w + 1;
            if (dy < 0) dist = (dist > -dy) ? dist : -dy;
            else if (dy >= h) dist = (dist > dy - h + 1) ? dist : dy - h + 1;

            double alpha = 0.3 * (1.0 - dist / shadow_size);
            if (alpha <= 0) continue;

            uint8_t *p = img->pixels + y * img->stride + x * 4;
            for (int c = 0; c < 3; c++) {
                p[c] = (uint8_t)(p[c] * (1.0 - alpha));
            }
        }
    }
}

// ── Photo frame ───────────────────────────────────────────────────────────

static void apply_photo_frame(WtzImage *img, int cx, int cy, int w, int h, int frame_w) {
    if (frame_w <= 0) return;
    // White frame
    draw_rounded_rect(img, cx - frame_w, cy - frame_w, w + 2 * frame_w, h + 2 * frame_w, 2, 255, 255, 255, 255);
    // Gray outline
    draw_rounded_rect(img, cx - frame_w, cy - frame_w, w + 2 * frame_w, h + 2 * frame_w, 2, 200, 200, 200, 0); // Outline only
}

// ── Foreground image (scaled, centered) ──────────────────────────────────

static void render_foreground(WtzImage *output, const WtzImage *src,
                               int cx, int cy, int w, int h) {
    // Simple bilinear scale and draw
    for (int y = 0; y < h; y++) {
        int sy = (int)((double)y / h * src->height);
        if (sy >= src->height) sy = src->height - 1;
        uint8_t *dst_row = output->pixels + (cy + y) * output->stride;
        const uint8_t *src_row = src->pixels + sy * src->stride;
        for (int x = 0; x < w; x++) {
            int sx = (int)((double)x / w * src->width);
            if (sx >= src->width) sx = src->width - 1;
            int dx = cx + x;
            if (dx < 0 || dx >= output->width) continue;
            dst_row[dx * 4 + 0] = src_row[sx * 4 + 0];
            dst_row[dx * 4 + 1] = src_row[sx * 4 + 1];
            dst_row[dx * 4 + 2] = src_row[sx * 4 + 2];
            dst_row[dx * 4 + 3] = 255;
        }
    }
}

// ── Main render entry point ───────────────────────────────────────────────

WtzImage* wtz_render(const WtzImage *src, const WtzRenderParams *params) {
    if (!src || !params) return NULL;
    int W = params->target_width;
    int H = params->target_height;
    if (W < 1 || H < 1) return NULL;

    WtzImage *output = wtz_image_new(W, H);
    if (!output) return NULL;

    // ── Background ──
    if (params->blur_mode) {
        render_background_blur(output, src, params);
    } else if (params->bg_gradient_style >= 1) {
        render_background_gradient(output, params);
    } else {
        render_background_solid(output, params);
    }

    // ── Vignette ──
    apply_vignette(output, params->vignette_strength);

    // ── Grain ──
    apply_grain(output, params->grain_strength);

    // ── Foreground composition ──
    // Self-similar composition: same ratio at each nesting level
    double RHO = WTZ_CANVAS_MARGIN;
    int margin_w = (int)(W * RHO);
    int margin_h = (int)(H * RHO);
    int eff_w = W - 2 * margin_w;
    int eff_h = H - 2 * margin_h;

    double frame_ratio = params->photo_frame ? params->photo_frame_width / 100.0 : 0.0;
    if (frame_ratio > 0.25) frame_ratio = 0.25;
    double img_budget_w = eff_w / (1.0 + 2.0 * frame_ratio);
    double img_budget_h = eff_h / (1.0 + 2.0 * frame_ratio);

    double scale_f = img_budget_w / src->width;
    if (img_budget_h / src->height < scale_f) scale_f = img_budget_h / src->height;

    double g_img_w = src->width * scale_f;
    double g_img_h = src->height * scale_f;
    double g_fw = params->photo_frame ? g_img_w * frame_ratio : 0.0;
    double g_visual_w = g_img_w + 2 * g_fw;
    double g_visual_h = g_img_h + 2 * g_fw;

    double max_zoom = 1.0;
    if (W / g_visual_w < max_zoom) max_zoom = W / g_visual_w;
    if (H / g_visual_h < max_zoom) max_zoom = H / g_visual_h;

    double zoom = params->fg_zoom;
    if (zoom < WTZ_MIN_ZOOM) zoom = WTZ_MIN_ZOOM;
    if (zoom > max_zoom) zoom = max_zoom;

    int img_w = (int)(g_img_w * zoom);
    int img_h = (int)(g_img_h * zoom);
    int frame_w = params->photo_frame ? (int)(img_w * frame_ratio) : 0;
    int total_visual_w = img_w + 2 * frame_w;
    int total_visual_h = img_h + 2 * frame_w;

    int fg_cx = (W - total_visual_w) / 2 + frame_w;
    int fg_cy = (H - total_visual_h) / 2 + frame_w;

    // Shadow
    apply_shadow(output, fg_cx, fg_cy + 2, img_w, img_h, 3);

    // Photo frame
    if (params->photo_frame) {
        apply_photo_frame(output, fg_cx, fg_cy, img_w, img_h, frame_w);
    }

    // Foreground image
    render_foreground(output, src, fg_cx, fg_cy, img_w, img_h);

    // ── Chromatic aberration ──
    apply_chromatic_aberration(output, params->ca_strength);

    return output;
}
