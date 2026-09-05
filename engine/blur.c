// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Float pipeline — all math in float32, single Bayer dither at the end
// This is the core quality insight from the Qt version.

#include "engine.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ── Float conversion ──────────────────────────────────────────────────────

static void image_to_float(const WtzImage *img, float *buf) {
    int w = img->width, h = img->height;
    for (int y = 0; y < h; y++) {
        const uint8_t *row = img->pixels + y * img->stride;
        float *fRow = buf + (size_t)y * w * 4;
        for (int x = 0; x < w; x++) {
            fRow[x * 4] = row[x * 4];
            fRow[x * 4 + 1] = row[x * 4 + 1];
            fRow[x * 4 + 2] = row[x * 4 + 2];
            fRow[x * 4 + 3] = row[x * 4 + 3];
        }
    }
}

static void float_to_image_dithered(const float *buf, WtzImage *img) {
    static const uint8_t bayer[8][8] = {
        {  0, 48, 12, 60,  3, 51, 15, 63 },
        { 32, 16, 44, 28, 35, 19, 47, 31 },
        {  8, 56,  4, 52, 11, 59,  7, 55 },
        { 40, 24, 36, 20, 43, 27, 39, 23 },
        {  2, 50, 14, 62,  1, 49, 13, 61 },
        { 34, 18, 46, 30, 33, 17, 45, 29 },
        { 10, 58,  6, 54,  9, 57,  5, 53 },
        { 42, 26, 38, 22, 41, 25, 37, 21 }
    };
    int w = img->width, h = img->height;
    float inv64 = 1.0f / 64.0f;
    for (int y = 0; y < h; y++) {
        uint8_t *row = img->pixels + y * img->stride;
        const float *fRow = buf + (size_t)y * w * 4;
        const uint8_t *bRow = bayer[y & 7];
        for (int x = 0; x < w; x++) {
            const float *fp = fRow + x * 4;
            float t = bRow[x & 7] * inv64;
            for (int c = 0; c < 4; c++) {
                float v = fmaxf(0.0f, fminf(fp[c], 255.0f));
                row[x * 4 + c] = (uint8_t)fminf((int)(v + t), 255);
            }
        }
    }
}

// ── Saturation boost (BT.601 luma, float) ─────────────────────────────────

static void boost_saturation_float(float *buf, int nPix, double factor) {
    float f = (float)factor;
    for (int i = 0; i < nPix; i++) {
        float *px = buf + i * 4;
        float b = px[0], g = px[1], r = px[2];
        float gray = 0.299f * r + 0.587f * g + 0.114f * b;
        px[0] = fmaxf(0.0f, fminf(gray + (b - gray) * f, 255.0f));
        px[1] = fmaxf(0.0f, fminf(gray + (g - gray) * f, 255.0f));
        px[2] = fmaxf(0.0f, fminf(gray + (r - gray) * f, 255.0f));
    }
}

// ── Color grade (gamma → warmth → blackLift, float LUT) ───────────────────

static void apply_color_grade_float(float *buf, int nPix,
                                     double gamma, double warmth, double blackLift) {
    int doGamma = fabs(gamma - 1.0) > 0.001;
    int doWarmth = fabs(warmth) > 0.001;
    int doLift = blackLift > 0.001;
    if (!doGamma && !doWarmth && !doLift) return;

    float *glut = NULL;
    if (doGamma) {
        glut = g_new(float, 4096);
        for (int i = 0; i < 4096; i++)
            glut[i] = 255.0f * powf((float)i / 4095.0f, (float)gamma);
    }
    float wr = 1.0f + 0.15f * (float)warmth;
    float wb = 1.0f - 0.15f * (float)warmth;
    float lift = 255.0f * (float)blackLift;

    for (int i = 0; i < nPix; i++) {
        float *px = buf + i * 4;
        for (int c = 0; c < 3; c++) {
            float v = px[c];
            if (doGamma) {
                int idx = (int)(v / 255.0f * 4095.0f);
                idx = (idx < 0) ? 0 : (idx > 4095) ? 4095 : idx;
                v = glut[idx];
            }
            if (doWarmth) v *= (c == 0) ? wb : (c == 2) ? wr : 1.0f;
            if (doLift && v < lift) v = lift;
            px[c] = fmaxf(0.0f, fminf(v, 255.0f));
        }
    }
    g_free(glut);
}

// ── Overlay blend (float, premultiplied invariant) ────────────────────────

static void apply_overlay_float(float *buf, int nPix,
                                 double opacity, uint32_t color) {
    if (opacity < 0.001) return;
    float fr = (float)((color >> 16) & 0xFF);
    float fg = (float)((color >> 8) & 0xFF);
    float fb = (float)(color & 0xFF);
    float a = fmaxf(0.0f, fminf(opacity, 1.0f));
    float ia = 1.0f - a;
    for (int i = 0; i < nPix; i++) {
        float *px = buf + i * 4;
        px[0] = px[0] * ia + fb * a;
        px[1] = px[1] * ia + fg * a;
        px[2] = px[2] * ia + fr * a;
        // Premultiplied invariant: blended RGB must not exceed alpha
        float A = px[3];
        px[0] = fminf(px[0], A);
        px[1] = fminf(px[1], A);
        px[2] = fminf(px[2], A);
    }
}

// ── Brightness (float multiply) ───────────────────────────────────────────

static void apply_brightness_float(float *buf, int nPix, double brightness) {
    if (fabs(brightness - 1.0) < 0.001) return;
    float b = (float)brightness;
    for (int i = 0; i < nPix; i++) {
        float *px = buf + i * 4;
        px[0] = fminf(px[0] * b, 255.0f);
        px[1] = fminf(px[1] * b, 255.0f);
        px[2] = fminf(px[2] * b, 255.0f);
    }
}

// ── Gaussian blur (edge-folding, branch-free, auto-vectorizing) ───────────

typedef struct {
    float *k;
    double *prefix;
    int radius;
    int size;
} GaussianKernel;

static GaussianKernel build_gaussian_kernel(double sigma) {
    int kSize = (int)ceil(3.0 * sigma);
    if ((kSize & 1) == 0) kSize++;
    GaussianKernel gk;
    gk.radius = kSize / 2;
    gk.size = kSize;
    gk.k = g_new(float, kSize);
    gk.prefix = g_new(double, kSize + 1);
    double sigmaSq2 = 2.0 * sigma * sigma;
    double sum = 0.0;
    for (int i = 0; i < kSize; i++) {
        int x = i - gk.radius;
        double v = exp(-(double)(x * x) / sigmaSq2);
        gk.k[i] = (float)v;
        sum += v;
    }
    double invSum = 1.0 / sum;
    gk.prefix[0] = 0.0;
    for (int i = 0; i < kSize; i++) {
        gk.k[i] = (float)(gk.k[i] * invSum);
        gk.prefix[i + 1] = gk.prefix[i] + gk.k[i];
    }
    return gk;
}

static void gaussian_blur_h(float *dst, const float *src, int w, int h,
                             const GaussianKernel *gk) {
    int kSize = gk->size;
    int radius = gk->radius;
    const float *k = gk->k;
    const double *pref = gk->prefix;
    for (int y = 0; y < h; y++) {
        const float *sRow = src + (size_t)y * w * 4;
        float *dRow = dst + (size_t)y * w * 4;
        for (int x = 0; x < w; x++) {
            int kStart = MAX(0, radius - x);
            int kEnd = MIN(kSize - 1, radius + (w - 1 - x));
            double c[4] = {0};
            double leftMass = pref[kStart];
            if (leftMass > 0.0) {
                const float *sp = sRow;
                c[0] += sp[0] * leftMass; c[1] += sp[1] * leftMass;
                c[2] += sp[2] * leftMass; c[3] += sp[3] * leftMass;
            }
            double rightMass = pref[kSize] - pref[kEnd + 1];
            if (rightMass > 0.0) {
                const float *sp = sRow + (w - 1) * 4;
                c[0] += sp[0] * rightMass; c[1] += sp[1] * rightMass;
                c[2] += sp[2] * rightMass; c[3] += sp[3] * rightMass;
            }
            const float *sp = sRow + (x + kStart - radius) * 4;
            for (int ki = kStart; ki <= kEnd; ki++, sp += 4) {
                double kw = k[ki];
                c[0] += sp[0] * kw; c[1] += sp[1] * kw;
                c[2] += sp[2] * kw; c[3] += sp[3] * kw;
            }
            float *dp = dRow + x * 4;
            dp[0] = (float)c[0]; dp[1] = (float)c[1];
            dp[2] = (float)c[2]; dp[3] = (float)c[3];
        }
    }
}

static void gaussian_blur_v(float *dst, const float *src, int w, int h,
                             const GaussianKernel *gk) {
    int kSize = gk->size;
    int radius = gk->radius;
    const float *k = gk->k;
    const double *pref = gk->prefix;
    size_t stride = (size_t)w * 4;
    for (int x = 0; x < w; x++) {
        const float *sBase = src + x * 4;
        float *dBase = dst + x * 4;
        for (int y = 0; y < h; y++) {
            int kStart = MAX(0, radius - y);
            int kEnd = MIN(kSize - 1, radius + (h - 1 - y));
            double c[4] = {0};
            double leftMass = pref[kStart];
            if (leftMass > 0.0) {
                const float *sp = sBase;
                c[0] += sp[0] * leftMass; c[1] += sp[1] * leftMass;
                c[2] += sp[2] * leftMass; c[3] += sp[3] * leftMass;
            }
            double rightMass = pref[kSize] - pref[kEnd + 1];
            if (rightMass > 0.0) {
                const float *sp = sBase + (h - 1) * stride;
                c[0] += sp[0] * rightMass; c[1] += sp[1] * rightMass;
                c[2] += sp[2] * rightMass; c[3] += sp[3] * rightMass;
            }
            const float *sp = sBase + (y + kStart - radius) * stride;
            for (int ki = kStart; ki <= kEnd; ki++, sp += stride) {
                double kw = k[ki];
                c[0] += sp[0] * kw; c[1] += sp[1] * kw;
                c[2] += sp[2] * kw; c[3] += sp[3] * kw;
            }
            float *dp = dBase + y * stride;
            dp[0] = (float)c[0]; dp[1] = (float)c[1];
            dp[2] = (float)c[2]; dp[3] = (float)c[3];
        }
    }
}

// ── Box blur cascade (Jarosz/Kutskir radii, O(n) for σ>50) ────────────────

static void sigma_to_boxes(int *boxes, int n, double sigma) {
    double wi = sqrt((12.0 * sigma * sigma / n) + 1.0);
    int wl = (int)floor(wi);
    if ((wl & 1) == 0) --wl;
    int wu = wl + 2;
    double mi = (12.0 * sigma * sigma - n * wl * wl - 4.0 * n * wl - 3.0 * n) / (-4.0 * wl - 4.0);
    int m = (int)round(mi);
    for (int i = 0; i < n; i++) boxes[i] = (i < m) ? wl : wu;
}

static void box_blur_h(float *dst, const float *src, int w, int h, int r,
                        int inValid, int outValid) {
    int div = 2 * r + 1;
    int xEnd = w - outValid;
    int yEnd = h - inValid;
    for (int y = inValid; y < yEnd; y++) {
        const float *s = src + (size_t)y * w * 4;
        float *d = dst + (size_t)y * w * 4;
        double acc[4] = {0};
        for (int x = outValid - r; x <= outValid + r; x++) {
            const float *p = s + x * 4;
            acc[0] += p[0]; acc[1] += p[1]; acc[2] += p[2]; acc[3] += p[3];
        }
        for (int x = outValid; x < xEnd - 1; x++) {
            float *dp = d + x * 4;
            dp[0] = (float)(acc[0] / div); dp[1] = (float)(acc[1] / div);
            dp[2] = (float)(acc[2] / div); dp[3] = (float)(acc[3] / div);
            const float *op = s + (x - r) * 4;
            const float *ip = s + (x + r + 1) * 4;
            acc[0] += ip[0] - op[0]; acc[1] += ip[1] - op[1];
            acc[2] += ip[2] - op[2]; acc[3] += ip[3] - op[3];
        }
        if (xEnd > outValid) {
            float *dp = d + (xEnd - 1) * 4;
            dp[0] = (float)(acc[0] / div); dp[1] = (float)(acc[1] / div);
            dp[2] = (float)(acc[2] / div); dp[3] = (float)(acc[3] / div);
        }
    }
}

static void box_blur_v(float *dst, const float *src, int w, int h, int r,
                        int inValid, int outValid) {
    int div = 2 * r + 1;
    size_t stride = (size_t)w * 4;
    int xEnd = w - outValid;
    int yEnd = h - outValid;
    for (int x = outValid; x < xEnd; x++) {
        const float *s = src + x * 4;
        float *d = dst + x * 4;
        double acc[4] = {0};
        for (int y = outValid - r; y <= outValid + r; y++) {
            const float *p = s + y * stride;
            acc[0] += p[0]; acc[1] += p[1]; acc[2] += p[2]; acc[3] += p[3];
        }
        for (int y = outValid; y < yEnd - 1; y++) {
            float *dp = d + y * stride;
            dp[0] = (float)(acc[0] / div); dp[1] = (float)(acc[1] / div);
            dp[2] = (float)(acc[2] / div); dp[3] = (float)(acc[3] / div);
            const float *op = s + (y - r) * stride;
            const float *ip = s + (y + r + 1) * stride;
            acc[0] += ip[0] - op[0]; acc[1] += ip[1] - op[1];
            acc[2] += ip[2] - op[2]; acc[3] += ip[3] - op[3];
        }
        if (yEnd > outValid) {
            float *dp = d + (yEnd - 1) * stride;
            dp[0] = (float)(acc[0] / div); dp[1] = (float)(acc[1] / div);
            dp[2] = (float)(acc[2] / div); dp[3] = (float)(acc[3] / div);
        }
    }
}

// ── Adaptive blur dispatch ────────────────────────────────────────────────

void wtz_blur_image(WtzImage *image, double sigma, double saturationFactor,
                     double overlayOpacity, uint32_t overlayColor, double brightness,
                     double colorGamma, double colorWarmth, double colorBlackLift) {
    if (sigma < 0.5 || !image || !image->pixels) return;
    int w = image->width, h = image->height;
    if (w < 1 || h < 1) return;
    int nPix = w * h;

    float *buf1 = g_new(float, nPix * 4);
    float *buf2 = g_new(float, nPix * 4);
    image_to_float(image, buf1);

    // Adaptive dispatch: Gaussian for σ≤50, box cascade for σ>50
    if (sigma <= 50.0) {
        GaussianKernel gk = build_gaussian_kernel(sigma);
        gaussian_blur_h(buf2, buf1, w, h, &gk);
        gaussian_blur_v(buf1, buf2, w, h, &gk);
        g_free(gk.k);
        g_free(gk.prefix);
    } else {
        int passes = (sigma < 100.0) ? 6 : 12;
        int widths[12], radii[12], pad = 0;
        sigma_to_boxes(widths, passes, sigma);
        for (int p = 0; p < passes; p++) { radii[p] = (widths[p] - 1) / 2; pad += radii[p]; }
        int W = w + 2 * pad, H = h + 2 * pad;
        // Pad once with replicated edges
        float *pb1 = g_new0(float, (size_t)W * H * 4);
        float *pb2 = g_new(float, (size_t)W * H * 4);
        for (int y = 0; y < H; y++) {
            int sy = (y < pad) ? 0 : (y >= pad + h) ? h - 1 : y - pad;
            float *dstRow = pb1 + (size_t)y * W * 4;
            const float *srcRow = buf1 + (size_t)sy * w * 4;
            const float *edgeL = srcRow;
            const float *edgeR = srcRow + (w - 1) * 4;
            for (int x = 0; x < pad; x++) memcpy(dstRow + x * 4, edgeL, 4 * sizeof(float));
            memcpy(dstRow + pad * 4, srcRow, (size_t)w * 4 * sizeof(float));
            for (int x = 0; x < pad; x++) memcpy(dstRow + (pad + w + x) * 4, edgeR, 4 * sizeof(float));
        }
        int valid = 0;
        for (int p = 0; p < passes; p++) {
            int inValid = valid;
            valid += radii[p];
            box_blur_h(pb2, pb1, W, H, radii[p], inValid, valid);
            box_blur_v(pb1, pb2, W, H, radii[p], inValid, valid);
        }
        // Crop valid region back
        for (int y = 0; y < h; y++)
            memcpy(buf1 + (size_t)y * w * 4,
                   pb1 + ((size_t)(y + pad) * W + pad) * 4,
                   (size_t)w * 4 * sizeof(float));
        g_free(pb1);
        g_free(pb2);
    }

    // Float pipeline: saturation → overlay → brightness → color grade → dither
    if (saturationFactor >= 0.0 && fabs(saturationFactor - 1.0) > 0.001)
        boost_saturation_float(buf1, nPix, saturationFactor);
    apply_overlay_float(buf1, nPix, overlayOpacity, overlayColor);
    apply_brightness_float(buf1, nPix, brightness);
    apply_color_grade_float(buf1, nPix, colorGamma, colorWarmth, colorBlackLift);
    float_to_image_dithered(buf1, image);
    g_free(buf1);
    g_free(buf2);
}
