// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Quick test: render a 100x100 solid red image with blur background

#include "engine.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Create a small test image (red square on white)
    WtzImage *src = wtz_image_new(100, 100);
    if (!src) { printf("FAIL: wtz_image_new\n"); return 1; }

    for (int y = 0; y < 100; y++) {
        uint8_t *row = src->pixels + y * src->stride;
        for (int x = 0; x < 100; x++) {
            row[x * 4 + 0] = 255; // B
            row[x * 4 + 1] = 0;   // G
            row[x * 4 + 2] = 0;   // R
            row[x * 4 + 3] = 255; // A
        }
    }

    WtzRenderParams params = {0};
    params.target_width = 800;
    params.target_height = 600;
    params.blur_mode = 1;
    params.blur_radius = 30;
    params.saturation_factor = 1.5;
    params.fg_zoom = 0.6;

    WtzImage *result = wtz_render(src, &params);
    if (!result) { printf("FAIL: wtz_render returned NULL\n"); wtz_image_free(src); return 1; }

    printf("OK: rendered %dx%d image\n", result->width, result->height);

    // Check center pixel (should be from foreground image)
    int cx = result->width / 2;
    int cy = result->height / 2;
    uint8_t *center = result->pixels + cy * result->stride + cx * 4;
    printf("Center pixel: B=%d G=%d R=%d A=%d\n", center[0], center[1], center[2], center[3]);

    // Check corner pixel (should be blurred background)
    uint8_t *corner = result->pixels;
    printf("Corner pixel: B=%d G=%d R=%d A=%d\n", corner[0], corner[1], corner[2], corner[3]);

    // Save test
    if (wtz_image_save_png(result, "/tmp/walltz_test.png")) {
        printf("OK: saved /tmp/walltz_test.png\n");
    } else {
        printf("FAIL: could not save\n");
    }

    wtz_image_free(src);
    wtz_image_free(result);
    return 0;
}
