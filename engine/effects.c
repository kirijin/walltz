// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Effects: vignette, grain, chromatic aberration

#include "engine.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void wtz_apply_vignette(WtzImage *image, double strength) {
    if (!image || !image->pixels || strength < 0.001) return;
    int w = image->width, h = image->height;
    double center_x = w / 2.0, center_y = h / 2.0;
    double max_dist = sqrt(center_x * center_x + center_y * center_y);
    
    for (int y = 0; y < h; y++) {
        uint8_t *row = image->pixels + y * image->stride;
        for (int x = 0; x < w; x++) {
            double dx = x - center_x, dy = y - center_y;
            double dist = sqrt(dx * dx + dy * dy) / max_dist;
            double factor = 1.0 - strength * dist * dist;
            if (factor < 0) factor = 0;
            for (int c = 0; c < 3; c++) {
                row[x * 4 + c] = (uint8_t)(row[x * 4 + c] * factor);
            }
        }
    }
}

void wtz_apply_grain(WtzImage *image, double strength) {
    if (!image || !image->pixels || strength < 0.001) return;
    int w = image->width, h = image->height;
    int intensity = (int)(15 * strength);
    
    for (int y = 0; y < h; y++) {
        uint8_t *row = image->pixels + y * image->stride;
        for (int x = 0; x < w; x++) {
            int noise = (rand() % (intensity * 2 + 1)) - intensity;
            for (int c = 0; c < 3; c++) {
                int val = row[x * 4 + c] + noise;
                row[x * 4 + c] = (val < 0) ? 0 : (val > 255) ? 255 : val;
            }
        }
    }
}

void wtz_apply_chromatic_aberration(WtzImage *image, double strength) {
    if (!image || !image->pixels || strength < 0.001) return;
    int w = image->width, h = image->height;
    double max_shift = strength * (w < h ? w : h) * 0.05;
    double center_x = w / 2.0, center_y = h / 2.0;
    double max_dist = sqrt(center_x * center_x + center_y * center_y);
    
    WtzImage *temp = wtz_image_new(w, h);
    if (!temp) return;
    
    for (int y = 0; y < h; y++) {
        uint8_t *dst_row = temp->pixels + y * temp->stride;
        for (int x = 0; x < w; x++) {
            double dx = (x - center_x) / max_dist;
            double dy = (y - center_y) / max_dist;
            int shift = (int)(sqrt(dx * dx + dy * dy) * max_shift);
            int sx = (int)(dx * shift), sy = (int)(dy * shift);
            
            int rx = x + sx, ry = y + sy;
            int bx = x - sx, by = y - sy;
            rx = (rx < 0) ? 0 : (rx >= w) ? w - 1 : rx;
            ry = (ry < 0) ? 0 : (ry >= h) ? h - 1 : ry;
            bx = (bx < 0) ? 0 : (bx >= w) ? w - 1 : bx;
            by = (by < 0) ? 0 : (by >= h) ? h - 1 : by;
            
            const uint8_t *rPx = image->pixels + ry * image->stride + rx * 4;
            const uint8_t *bPx = image->pixels + by * image->stride + bx * 4;
            const uint8_t *srcPx = image->pixels + y * image->stride + x * 4;
            
            dst_row[x * 4 + 0] = bPx[0];  // B shifts inward
            dst_row[x * 4 + 1] = srcPx[1]; // G unchanged
            dst_row[x * 4 + 2] = rPx[2];  // R shifts outward
            dst_row[x * 4 + 3] = srcPx[3]; // A unchanged
        }
    }
    
    memcpy(image->pixels, temp->pixels, image->stride * h);
    wtz_image_free(temp);
}
