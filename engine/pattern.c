// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Pattern tile generation — geometric primitives drawn directly to pixel buffer
// No Cairo dependency needed for these simple shapes.

#include "engine.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI G_PI
#endif

// ── Helper: set pixel with bounds check ───────────────────────────────────

static void set_pixel(WtzImage *img, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || x >= img->width || y < 0 || y >= img->height) return;
    uint8_t *p = img->pixels + y * img->stride + x * 4;
    p[0] = b; p[1] = g; p[2] = r; p[3] = a;
}

// ── Helper: fill rect ─────────────────────────────────────────────────────

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

// ── Helper: draw filled circle ────────────────────────────────────────────

static void draw_circle(WtzImage *img, int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                set_pixel(img, cx + dx, cy + dy, r, g, b, a);
            }
        }
    }
}

// ── Helper: draw line (Bresenham) ─────────────────────────────────────────

static void draw_line(WtzImage *img, int x0, int y0, int x1, int y1, int width, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        for (int w = -width / 2; w <= width / 2; w++) {
            set_pixel(img, x0 + w, y0, r, g, b, a);
            set_pixel(img, x0, y0 + w, r, g, b, a);
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// ── Geometric tile generators ─────────────────────────────────────────────

static void draw_dots(WtzImage *tile, int S, uint32_t fg, double scale) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    double spacing = S / 4.0;
    int radius = (int)fmax(1.0, S * 0.08 * scale);
    fprintf(stderr, "DEBUG draw_dots: S=%d spacing=%.1f radius=%d color=%02x%02x%02x\n", S, spacing, radius, r, g, b);
    for (double y = 0; y <= S; y += spacing) {
        for (double x = 0; x <= S; x += spacing) {
            fprintf(stderr, "  drawing dot at (%.0f, %.0f)\n", x, y);
            draw_circle(tile, (int)x, (int)y, radius, r, g, b, 255);
        }
    }
}

static void draw_stripes_h(WtzImage *tile, int S, uint32_t fg, double scale) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    double spacing = fmax(2.0, S / 5.0);
    int h = (int)fmax(1.0, spacing * 0.4);
    for (double y = 0; y < S; y += spacing) {
        fill_rect(tile, 0, (int)y, S, h, r, g, b, 255);
    }
}

static void draw_stripes_v(WtzImage *tile, int S, uint32_t fg, double scale) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    double spacing = fmax(2.0, S / 5.0);
    int w = (int)fmax(1.0, spacing * 0.4);
    for (double x = 0; x < S; x += spacing) {
        fill_rect(tile, (int)x, 0, w, S, r, g, b, 255);
    }
}

static void draw_stripes_d(WtzImage *tile, int S, uint32_t fg, double scale) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    double spacing = fmax(2.0, S * 0.3);
    int sw = (int)fmax(1.0, spacing * 0.25);
    int len = (int)(S * 1.5);
    for (int d = -len; d < len; d += (int)spacing) {
        for (int w = -sw / 2; w <= sw / 2; w++) {
            draw_line(tile, d + w, 0, d + len + w, len, 1, r, g, b, 255);
        }
    }
}

static void draw_checkerboard(WtzImage *tile, int S, uint32_t fg, double scale) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int cells = 4;
    double cs = S / (double)cells;
    for (int row = 0; row < cells; row++) {
        for (int col = 0; col < cells; col++) {
            if ((row + col) % 2 == 1) {
                fill_rect(tile, (int)(col * cs), (int)(row * cs), (int)cs, (int)cs, r, g, b, 255);
            }
        }
    }
}

static void draw_chevron(WtzImage *tile, int S, uint32_t fg, double scale) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    double step = S / 4.0;
    double sw = step * 0.3;
    for (double y = -step; y < S + step; y += step) {
        for (int x = 0; x < S; x++) {
            double dist_from_apex = fabs((x - S / 2.0) / (S / 2.0));
            double chevron_y = y + dist_from_apex * sw;
            if (fabs(chevron_y - y) < 0.5) {
                set_pixel(tile, x, (int)y, r, g, b, 255);
            }
        }
    }
}

static void draw_diamonds(WtzImage *tile, int S, uint32_t fg, double scale) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    double spacing = S / 3.0;
    double hs = spacing * 0.6;
    for (double y = -spacing; y < S + spacing; y += spacing) {
        for (double x = -spacing; x < S + spacing; x += spacing) {
            draw_line(tile, (int)x, (int)(y - hs), (int)(x + hs), (int)y, 1, r, g, b, 255);
            draw_line(tile, (int)(x + hs), (int)y, (int)x, (int)(y + hs), 1, r, g, b, 255);
            draw_line(tile, (int)x, (int)(y + hs), (int)(x - hs), (int)y, 1, r, g, b, 255);
            draw_line(tile, (int)(x - hs), (int)y, (int)x, (int)(y - hs), 1, r, g, b, 255);
        }
    }
}

static void draw_crosshatch(WtzImage *tile, int S, uint32_t fg, double scale) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    double spacing = fmax(2.0, S * 0.2);
    int sw = (int)fmax(1.0, spacing * 0.15);
    int len = (int)(S * 1.5);
    for (int d = -len; d < len; d += (int)spacing) {
        for (int w = -sw / 2; w <= sw / 2; w++) {
            draw_line(tile, d + w, 0, d + len + w, len, 1, r, g, b, 255);
            draw_line(tile, d + w, len, d + len + w, 0, 1, r, g, b, 255);
        }
    }
}

// ── SVG geometric icon generators ──────────────────────────────────────────

static void draw_svg_squares(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int margin = S / 10;
    for (int x = margin; x < S - margin; x++) {
        set_pixel(tile, x, margin, r, g, b, 255);
        set_pixel(tile, x, S - margin - 1, r, g, b, 255);
    }
    for (int y = margin; y < S - margin; y++) {
        set_pixel(tile, margin, y, r, g, b, 255);
        set_pixel(tile, S - margin - 1, y, r, g, b, 255);
    }
    int im = S / 4;
    for (int x = im; x < S - im; x++) {
        set_pixel(tile, x, im, r, g, b, 255);
        set_pixel(tile, x, S - im - 1, r, g, b, 255);
    }
    for (int y = im; y < S - im; y++) {
        set_pixel(tile, im, y, r, g, b, 255);
        set_pixel(tile, S - im - 1, y, r, g, b, 255);
    }
}

static void draw_svg_triangles(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int margin = S / 10;
    draw_line(tile, S / 2, margin, margin, S - margin, 1, r, g, b, 255);
    draw_line(tile, S / 2, margin, S - margin, S - margin, 1, r, g, b, 255);
    draw_line(tile, margin, S - margin, S - margin, S - margin, 1, r, g, b, 255);
    int im = S / 4;
    draw_line(tile, S / 2, S - im - 1, im, im, 1, r, g, b, 255);
    draw_line(tile, S / 2, S - im - 1, S - im - 1, im, 1, r, g, b, 255);
    draw_line(tile, im, im, S - im - 1, im, 1, r, g, b, 255);
}

static void draw_svg_circles(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int cx = S / 2, cy = S / 2;
    int outer_r = S / 2 - S / 10;
    int inner_r = S / 4;
    for (int angle = 0; angle < 360; angle++) {
        double rad = angle * M_PI / 180.0;
        for (int w = -1; w <= 1; w++) {
            set_pixel(tile, (int)(cx + (outer_r + w) * cos(rad)), (int)(cy + (outer_r + w) * sin(rad)), r, g, b, 255);
            set_pixel(tile, (int)(cx + (inner_r + w) * cos(rad)), (int)(cy + (inner_r + w) * sin(rad)), r, g, b, 255);
        }
    }
}

static void draw_svg_rings(WtzImage *tile, int S, uint32_t fg) {
    draw_svg_circles(tile, S, fg);
}

static void draw_svg_lines(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int margin = S / 10;
    int spacing = (S - 2 * margin) / 5;
    for (int i = 0; i < 5; i++) {
        int x = margin + i * spacing;
        draw_line(tile, x, margin, x, S - margin, 1, r, g, b, 255);
    }
}

static void draw_svg_worms(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int margin = S / 10;
    int spacing = (S - 2 * margin) / 3;
    for (int i = 0; i < 3; i++) {
        int y = margin + i * spacing + spacing / 2;
        for (int x = margin; x < S - margin; x++) {
            double phase = (x - margin) * 2 * M_PI / (S - 2 * margin);
            int wy = (int)(y + sin(phase) * spacing / 3);
            set_pixel(tile, x, wy, r, g, b, 255);
            set_pixel(tile, x, wy + 1, r, g, b, 255);
        }
    }
}

static void draw_svg_dots_scattered(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int radius = S / 20;
    int positions[][2] = {
        {18, 18}, {50, 14}, {82, 20}, {28, 42}, {55, 34}, {78, 48},
        {14, 68}, {42, 62}, {62, 72}, {86, 65}, {24, 88}, {50, 86}, {76, 90}
    };
    for (int i = 0; i < 13; i++) {
        int cx = positions[i][0] * S / 100;
        int cy = positions[i][1] * S / 100;
        draw_circle(tile, cx, cy, radius, r, g, b, 255);
    }
}

static void draw_svg_diamonds_scattered(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int positions[][2] = {
        {20, 10}, {50, 10}, {80, 10}, {35, 32}, {65, 32},
        {20, 52}, {50, 52}, {80, 52}, {35, 72}, {65, 72},
        {20, 92}, {50, 92}, {80, 92}
    };
    int hs = S / 12;
    for (int i = 0; i < 13; i++) {
        int cx = positions[i][0] * S / 100;
        int cy = positions[i][1] * S / 100;
        draw_line(tile, cx, cy - hs, cx + hs, cy, 1, r, g, b, 255);
        draw_line(tile, cx + hs, cy, cx, cy + hs, 1, r, g, b, 255);
        draw_line(tile, cx, cy + hs, cx - hs, cy, 1, r, g, b, 255);
        draw_line(tile, cx - hs, cy, cx, cy - hs, 1, r, g, b, 255);
    }
}

static void draw_svg_square_single(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int margin = S / 6;
    for (int x = margin; x < S - margin; x++) {
        set_pixel(tile, x, margin, r, g, b, 255);
        set_pixel(tile, x, S - margin - 1, r, g, b, 255);
    }
    for (int y = margin; y < S - margin; y++) {
        set_pixel(tile, margin, y, r, g, b, 255);
        set_pixel(tile, S - margin - 1, y, r, g, b, 255);
    }
}

static void draw_svg_triangle_single(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int margin = S / 6;
    draw_line(tile, S / 2, margin, margin, S - margin, 1, r, g, b, 255);
    draw_line(tile, S / 2, margin, S - margin, S - margin, 1, r, g, b, 255);
    draw_line(tile, margin, S - margin, S - margin, S - margin, 1, r, g, b, 255);
}

static void draw_svg_circle_single(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int cx = S / 2, cy = S / 2;
    int radius = S / 2 - S / 6;
    for (int angle = 0; angle < 360; angle++) {
        double rad = angle * M_PI / 180.0;
        set_pixel(tile, (int)(cx + radius * cos(rad)), (int)(cy + radius * sin(rad)), r, g, b, 255);
    }
}

static void draw_svg_diamond_single(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int margin = S / 6;
    int cx = S / 2;
    draw_line(tile, cx, margin, S - margin, S / 2, 1, r, g, b, 255);
    draw_line(tile, S - margin, S / 2, cx, S - margin, 1, r, g, b, 255);
    draw_line(tile, cx, S - margin, margin, S / 2, 1, r, g, b, 255);
    draw_line(tile, margin, S / 2, cx, margin, 1, r, g, b, 255);
}

static void draw_svg_ring_single(WtzImage *tile, int S, uint32_t fg) {
    draw_svg_circle_single(tile, S, fg);
}

static void draw_svg_wave(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int margin = S / 10;
    int y = S / 2;
    int amplitude = S / 8;
    for (int x = margin; x < S - margin; x++) {
        double phase = (x - margin) * 4 * M_PI / (S - 2 * margin);
        int wy = (int)(y + sin(phase) * amplitude);
        set_pixel(tile, x, wy, r, g, b, 255);
        set_pixel(tile, x, wy + 1, r, g, b, 255);
    }
}

static void draw_svg_cross(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int margin = S / 6;
    draw_line(tile, margin, margin, S - margin, S - margin, 1, r, g, b, 255);
    draw_line(tile, S - margin, margin, margin, S - margin, 1, r, g, b, 255);
}

static void draw_svg_disc(WtzImage *tile, int S, uint32_t fg) {
    uint8_t r = (fg >> 16) & 0xFF, g = (fg >> 8) & 0xFF, b = fg & 0xFF;
    int cx = S / 2, cy = S / 2;
    int radius = S / 2 - S / 10;
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                set_pixel(tile, cx + x, cy + y, r, g, b, 255);
            }
        }
    }
}

// ── Pattern dispatch ──────────────────────────────────────────────────────

WtzImage* wtz_generate_pattern_tile(int kind, int index, int tile_size,
                                    uint32_t fg_color, double scale) {
    WtzImage *tile = wtz_image_new(tile_size, tile_size);
    if (!tile) return NULL;

    memset(tile->pixels, 0, tile->stride * tile_size);

    fprintf(stderr, "DEBUG wtz_generate_pattern_tile: kind=%d index=%d size=%d color=%08x\n", kind, index, tile_size, fg_color);
    fflush(stderr);

    if (kind == 0) {
        switch (index) {
            case 0: draw_dots(tile, tile_size, fg_color, scale); break;
            case 1: draw_stripes_h(tile, tile_size, fg_color, scale); break;
            case 2: draw_stripes_v(tile, tile_size, fg_color, scale); break;
            case 3: draw_stripes_d(tile, tile_size, fg_color, scale); break;
            case 4: draw_checkerboard(tile, tile_size, fg_color, scale); break;
            case 5: draw_chevron(tile, tile_size, fg_color, scale); break;
            case 6: draw_diamonds(tile, tile_size, fg_color, scale); break;
            case 7: draw_crosshatch(tile, tile_size, fg_color, scale); break;
            default: break;
        }
    } else if (kind == 1) {
        switch (index) {
            case 0: draw_svg_squares(tile, tile_size, fg_color); break;
            case 1: draw_svg_triangles(tile, tile_size, fg_color); break;
            case 2: draw_svg_circles(tile, tile_size, fg_color); break;
            case 3: draw_svg_rings(tile, tile_size, fg_color); break;
            case 4: draw_svg_lines(tile, tile_size, fg_color); break;
            case 5: draw_svg_worms(tile, tile_size, fg_color); break;
            case 6: draw_svg_dots_scattered(tile, tile_size, fg_color); break;
            case 7: draw_svg_diamonds_scattered(tile, tile_size, fg_color); break;
            case 8: draw_svg_square_single(tile, tile_size, fg_color); break;
            case 9: draw_svg_triangle_single(tile, tile_size, fg_color); break;
            case 10: draw_svg_circle_single(tile, tile_size, fg_color); break;
            case 11: draw_svg_diamond_single(tile, tile_size, fg_color); break;
            case 12: draw_svg_ring_single(tile, tile_size, fg_color); break;
            case 13: draw_svg_wave(tile, tile_size, fg_color); break;
            case 14: draw_svg_cross(tile, tile_size, fg_color); break;
            case 15: draw_svg_disc(tile, tile_size, fg_color); break;
            default: break;
        }
    }

    fflush(stderr);
    return tile;
}
