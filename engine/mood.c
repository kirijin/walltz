// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Mood palette extraction — hue histogram (V1) + 3D RGB histogram (V2)
// Faithful port of Qt WallpaperProcessor::computeMoodPalettes/computeMoodPalettesV2

#include "engine.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define HUE_BINS 24
#define RGB_BINS 8

// ── Helpers ───────────────────────────────────────────────────────────────

static inline double lum(double r, double g, double b) {
    return 0.299 * r + 0.587 * g + 0.114 * b;
}

static float hue_deg(float r, float g, float b, float sat, float *out_weight) {
    if (sat < 1e-6) { *out_weight = 0; return 0; }
    float mx = fmaxf(r, fmaxf(g, b));
    float mn = fminf(r, fminf(g, b));
    float delta = mx - mn;
    float h = 0;
    if (delta < 1e-6) { *out_weight = 0; return 0; }
    if (mx == r) h = 60.0f * fmodf((g - b) / delta, 6.0f);
    else if (mx == g) h = 60.0f * ((b - r) / delta + 2.0f);
    else h = 60.0f * ((r - g) / delta + 4.0f);
    if (h < 0) h += 360.0f;
    *out_weight = sat;
    return h;
}

static void rgb_to_hsl(float r, float g, float b, float *h, float *s, float *l) {
    float mx = fmaxf(r, fmaxf(g, b));
    float mn = fminf(r, fminf(g, b));
    *l = (mx + mn) / 2.0f;
    float delta = mx - mn;
    if (delta < 1e-6) { *h = 0; *s = 0; return; }
    *s = (*l > 0.5f) ? delta / (2.0f - mx - mn) : delta / (mx + mn);
    if (mx == r) *h = fmodf((g - b) / delta, 6.0f);
    else if (mx == g) *h = (b - r) / delta + 2.0f;
    else *h = (r - g) / delta + 4.0f;
    *h /= 6.0f;
    if (*h < 0) *h += 1.0f;
}

static void hsl_to_rgb(float h, float s, float l, uint8_t *r, uint8_t *g, uint8_t *b) {
    float c = (1.0f - fabsf(2.0f * l - 1.0f)) * s;
    float x = c * (1.0f - fabsf(fmodf(h * 6.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;
    float rr, gg, bb;
    if (h < 1.0f/6.0f) { rr = c; gg = x; bb = 0; }
    else if (h < 2.0f/6.0f) { rr = x; gg = c; bb = 0; }
    else if (h < 3.0f/6.0f) { rr = 0; gg = c; bb = x; }
    else if (h < 4.0f/6.0f) { rr = 0; gg = x; bb = c; }
    else if (h < 5.0f/6.0f) { rr = x; gg = 0; bb = c; }
    else { rr = c; gg = 0; bb = x; }
    *r = (uint8_t)roundf((rr + m) * 255.0f);
    *g = (uint8_t)roundf((gg + m) * 255.0f);
    *b = (uint8_t)roundf((bb + m) * 255.0f);
}

static uint32_t make_rgba(uint8_t r, uint8_t g, uint8_t b) {
    return 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static uint32_t hsl_to_packed(float h, float s, float l) {
    uint8_t r, g, b;
    hsl_to_rgb(h, s, l, &r, &g, &b);
    return make_rgba(r, g, b);
}

// ── V1: Hue histogram mood palettes ───────────────────────────────────────

WtzMoodPalettes* wtz_extract_mood_palettes(const WtzImage *img) {
    WtzMoodPalettes *palettes = g_new0(WtzMoodPalettes, 1);

    for (int i = 0; i < 6; i++) {
        palettes->moods[i].color_a = 0xFF808080;
        palettes->moods[i].color_b = 0xFFB4B4B4;
        palettes->v2_moods[i].color_a = 0xFF808080;
        palettes->v2_moods[i].color_b = 0xFFB4B4B4;
    }

    if (!img || !img->pixels) return palettes;

    int w = img->width, h = img->height;
    int step = MAX(1, MAX(w, h) / 64);
    double hue_weight[HUE_BINS];
    double hue_sat[HUE_BINS];
    double hue_light[HUE_BINS];
    int hue_count[HUE_BINS];
    memset(hue_weight, 0, sizeof(hue_weight));
    memset(hue_sat, 0, sizeof(hue_sat));
    memset(hue_light, 0, sizeof(hue_light));
    memset(hue_count, 0, sizeof(hue_count));

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

            float wgt;
            float h_val = hue_deg(r / 255.0f, g / 255.0f, b / 255.0f, sat, &wgt);
            if (wgt > 0) {
                int bin = MIN((int)(h_val / 30.0f), HUE_BINS - 1);
                double w = MAX(sat * 100.0, 1.0);
                hue_weight[bin] += w;
                hue_sat[bin] += sat * w;
                hue_light[bin] += lgt * w;
                hue_count[bin]++;
            }
        }
    }

    int best1 = 0, best2 = 0;
    double best_w1 = 0, best_w2 = 0;
    for (int i = 0; i < HUE_BINS; i++) {
        if (hue_weight[i] > best_w1) { best_w2 = best_w1; best2 = best1; best_w1 = hue_weight[i]; best1 = i; }
        else if (hue_weight[i] > best_w2) { best_w2 = hue_weight[i]; best2 = i; }
    }

    if (best_w1 < 1.0) return palettes;

    static const float GOLDEN_TURN = 0.38197f;

    float h1 = (best1 + 0.5f) / HUE_BINS;
    float s1 = (hue_count[best1] > 0) ? hue_sat[best1] / hue_weight[best1] : 0.5f;
    float l1 = (hue_count[best1] > 0) ? hue_light[best1] / hue_weight[best1] : 0.5f;
    s1 = fmaxf(0.35f, fminf(s1, 0.75f));
    l1 = fmaxf(0.35f, fminf(l1, 0.70f));

    // Mood 0: Auto
    float hueA = h1;
    float hueB;
    if (best_w2 < 1.0) {
        hueB = fmodf(hueA + GOLDEN_TURN, 1.0f);
    } else {
        hueB = (best2 + 0.5f) / HUE_BINS;
        float hD = fmodf(hueA - hueB + 1.5f, 1.0f) - 0.5f;
        hueB = fmodf(hueB + hD * 0.2f + 1.0f, 1.0f);
    }
    float sB = fmaxf(0.30f, fminf((s1 + 0.5f) * 0.45f, 0.65f));
    float lB = fmaxf(0.40f, fminf(l1 + 0.15f, 0.78f));
    float spread = fabsf(hueB - hueA);
    if (spread > 0.5f) spread = 1.0f - spread;
    if (spread < 0.014f) hueB = fmodf(hueA + GOLDEN_TURN, 1.0f);

    palettes->moods[0].color_a = hsl_to_packed(hueA, s1, l1);
    palettes->moods[0].color_b = hsl_to_packed(hueB, sB, lB);

    // Mood 1: Soft
    int sb = -1;
    for (int i = 0; i < HUE_BINS; i++) {
        if (hue_weight[i] <= 0) continue;
        float s = hue_sat[i] / hue_weight[i];
        float l = hue_light[i] / hue_weight[i];
        if (s >= 0.25f && s <= 0.60f && l >= 0.35f && l <= 0.70f) { sb = i; break; }
    }
    float hs = (sb >= 0) ? ((sb + 0.5f) / HUE_BINS) : h1;
    float ss = (sb >= 0 && hue_count[sb] > 0) ? hue_sat[sb] / hue_weight[sb] : 0.40f;
    float ls = (sb >= 0 && hue_count[sb] > 0) ? hue_light[sb] / hue_weight[sb] : 0.52f;
    ss = fmaxf(0.28f, fminf(ss, 0.48f));
    ls = fmaxf(0.38f, fminf(ls, 0.62f));
    palettes->moods[1].color_a = hsl_to_packed(hs, ss, ls);
    palettes->moods[1].color_b = hsl_to_packed(fmodf(hs + GOLDEN_TURN, 1.0f),
        fmaxf(0.22f, fminf(ss * 0.85f, 0.40f)), fmaxf(0.48f, fminf(ls + 0.10f, 0.72f)));

    // Mood 2: Vivid
    int vb = best1;
    double vs = -1;
    for (int i = 0; i < HUE_BINS; i++) {
        if (hue_weight[i] <= 0) continue;
        double sc = (hue_sat[i] / hue_weight[i]) * (hue_light[i] / hue_weight[i]);
        if (sc > vs) { vs = sc; vb = i; }
    }
    float hv = (vb + 0.5f) / HUE_BINS;
    float sv = (hue_count[vb] > 0) ? hue_sat[vb] / hue_weight[vb] : 0.65f;
    float lv = (hue_count[vb] > 0) ? hue_light[vb] / hue_weight[vb] : 0.55f;
    sv = fmaxf(0.55f, fminf(sv, 0.85f));
    lv = fmaxf(0.40f, fminf(lv, 0.68f));
    palettes->moods[2].color_a = hsl_to_packed(hv, sv, lv);
    float h2v = (best2 != vb && best_w2 > 1.0) ? ((best2 + 0.5f) / HUE_BINS) : fmodf(hv + GOLDEN_TURN, 1.0f);
    palettes->moods[2].color_b = hsl_to_packed(h2v,
        fmaxf(0.45f, fminf(sv * 0.80f, 0.70f)), fmaxf(0.45f, fminf(lv + 0.10f, 0.72f)));

    // Mood 3: Warm (bins 0-3 = 0°-60°)
    int wb = -1;
    for (int i = 0; i <= 3; i++) { if (hue_weight[i] > 0) { wb = i; break; } }
    float hw = (wb >= 0) ? ((wb + 0.5f) / HUE_BINS) : 0.10f;
    float sw = (wb >= 0 && hue_count[wb] > 0) ? hue_sat[wb] / hue_weight[wb] : 0.55f;
    float lw = (wb >= 0 && hue_count[wb] > 0) ? hue_light[wb] / hue_weight[wb] : 0.50f;
    sw = fmaxf(0.40f, fminf(sw, 0.72f));
    lw = fmaxf(0.38f, fminf(lw, 0.65f));
    palettes->moods[3].color_a = hsl_to_packed(hw, sw, lw);
    palettes->moods[3].color_b = hsl_to_packed(fmodf(hw + GOLDEN_TURN, 1.0f),
        fmaxf(0.35f, fminf(sw * 0.85f, 0.60f)), fmaxf(0.42f, fminf(lw + 0.12f, 0.72f)));

    // Mood 4: Cool (bins 12-17 = 180°-270°)
    int cb = -1;
    for (int i = 12; i <= 17; i++) { if (hue_weight[i] > 0) { cb = i; break; } }
    float hc = (cb >= 0) ? ((cb + 0.5f) / HUE_BINS) : 0.60f;
    float sc = (cb >= 0 && hue_count[cb] > 0) ? hue_sat[cb] / hue_weight[cb] : 0.50f;
    float lc = (cb >= 0 && hue_count[cb] > 0) ? hue_light[cb] / hue_weight[cb] : 0.50f;
    sc = fmaxf(0.40f, fminf(sc, 0.72f));
    lc = fmaxf(0.38f, fminf(lc, 0.65f));
    palettes->moods[4].color_a = hsl_to_packed(hc, sc, lc);
    palettes->moods[4].color_b = hsl_to_packed(fmodf(hc + GOLDEN_TURN, 1.0f),
        fmaxf(0.35f, fminf(sc * 0.85f, 0.60f)), fmaxf(0.42f, fminf(lc + 0.12f, 0.72f)));

    // Mood 5: Deep (lowest lightness)
    int db = best1;
    double ml = 1.0;
    for (int i = 0; i < HUE_BINS; i++) {
        if (hue_weight[i] <= 0) continue;
        float l = hue_light[i] / hue_weight[i];
        if (l < ml) { ml = l; db = i; }
    }
    float hd = (db + 0.5f) / HUE_BINS;
    float sd = (hue_count[db] > 0) ? hue_sat[db] / hue_weight[db] : 0.40f;
    float ld = (hue_count[db] > 0) ? hue_light[db] / hue_weight[db] : 0.35f;
    sd = fmaxf(0.35f, fminf(sd, 0.60f));
    ld = fmaxf(0.20f, fminf(ld, 0.40f));
    palettes->moods[5].color_a = hsl_to_packed(hd, sd, ld);
    palettes->moods[5].color_b = hsl_to_packed(fmodf(hd + GOLDEN_TURN, 1.0f),
        fmaxf(0.30f, fminf(sd * 0.90f, 0.50f)), fmaxf(0.30f, fminf(ld + 0.12f, 0.50f)));

    // ── V2: 3D RGB histogram mood palettes ─────────────────────────────────
    struct Bin { double weight, r, g, b; int count; };
    struct Bin bins[RGB_BINS][RGB_BINS][RGB_BINS];
    memset(bins, 0, sizeof(bins));

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
            if (sat < 0.12f || lgt < 0.10f || lgt > 0.92f) continue;
            int ri = MIN(r * RGB_BINS / 256, RGB_BINS - 1);
            int gi = MIN(g * RGB_BINS / 256, RGB_BINS - 1);
            int bi = MIN(b * RGB_BINS / 256, RGB_BINS - 1);
            double wgt = sat * sat * (1.0 - fabs(0.5 - lgt));
            bins[ri][gi][bi].weight += wgt;
            bins[ri][gi][bi].r += r * wgt;
            bins[ri][gi][bi].g += g * wgt;
            bins[ri][gi][bi].b += b * wgt;
            bins[ri][gi][bi].count++;
        }
    }

    typedef struct { double r, g, b, weight; int ri, gi, bi, count; } Centroid;
    Centroid centroids[512];
    int n_centroids = 0;
    for (int ri = 0; ri < RGB_BINS; ri++) {
        for (int gi = 0; gi < RGB_BINS; gi++) {
            for (int bi = 0; bi < RGB_BINS; bi++) {
                struct Bin *bin = &bins[ri][gi][bi];
                if (bin->weight < 0.5) continue;
                double rAvg = bin->r / bin->weight;
                double gAvg = bin->g / bin->weight;
                double bAvg = bin->b / bin->weight;
                double mnC = fmin(rAvg, fmin(gAvg, bAvg));
                double mxC = fmax(rAvg, fmax(gAvg, bAvg));
                double chroma = (mxC == 0) ? 0.0 : (mxC - mnC) / mxC;
                centroids[n_centroids].r = rAvg;
                centroids[n_centroids].g = gAvg;
                centroids[n_centroids].b = bAvg;
                centroids[n_centroids].weight = bin->weight * chroma * chroma;
                centroids[n_centroids].ri = ri;
                centroids[n_centroids].gi = gi;
                centroids[n_centroids].bi = bi;
                centroids[n_centroids].count = bin->count;
                n_centroids++;
            }
        }
    }

    if (n_centroids == 0) return palettes;

    // Sort by weight descending
    for (int i = 0; i < n_centroids - 1; i++) {
        for (int j = i + 1; j < n_centroids; j++) {
            if (centroids[j].weight > centroids[i].weight) {
                Centroid tmp = centroids[i];
                centroids[i] = centroids[j];
                centroids[j] = tmp;
            }
        }
    }

    // Pick top 3 distinct centroids
    Centroid *picks[3] = {NULL, NULL, NULL};
    int n_picks = 0;
    picks[n_picks++] = &centroids[0];
    for (int i = 1; i < n_centroids && n_picks < 3; i++) {
        int ok = 1;
        for (int j = 0; j < n_picks; j++) {
            if (abs(centroids[i].ri - picks[j]->ri) < 2 &&
                abs(centroids[i].gi - picks[j]->gi) < 2 &&
                abs(centroids[i].bi - picks[j]->bi) < 2) { ok = 0; break; }
        }
        if (ok) picks[n_picks++] = &centroids[i];
    }

    // Convert to HSL colors
    float colors_h[3], colors_s[3], colors_l[3];
    for (int i = 0; i < n_picks; i++) {
        rgb_to_hsl(centroids[i].r / 255.0f, centroids[i].g / 255.0f, centroids[i].b / 255.0f,
                   &colors_h[i], &colors_s[i], &colors_l[i]);
    }
    for (int i = n_picks; i < 3; i++) {
        colors_h[i] = colors_h[0]; colors_s[i] = colors_s[0]; colors_l[i] = colors_l[0];
    }

    // Softened versions
    float soft_h[3], soft_s[3], soft_l[3];
    for (int i = 0; i < 3; i++) {
        soft_h[i] = colors_h[i];
        soft_s[i] = fmaxf(0.25f, fminf(colors_s[i] * 0.60f, 0.55f));
        soft_l[i] = fmaxf(0.38f, fminf(colors_l[i], 0.70f));
    }

    // Pair by max contrast
    double bestDist = -1; int bestA = 0, bestB = 1;
    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 3; j++) {
            double dr = soft_h[i] - soft_h[j];
            double dg = soft_s[i] - soft_s[j];
            double db = soft_l[i] - soft_l[j];
            double dist = dr * dr + dg * dg + db * db;
            if (dist > bestDist) { bestDist = dist; bestA = i; bestB = j; }
        }
    }

    // Mood 0: Dynamic (max contrast pair)
    palettes->v2_moods[0].color_a = hsl_to_packed(soft_h[bestA], soft_s[bestA], soft_l[bestA]);
    palettes->v2_moods[0].color_b = hsl_to_packed(soft_h[bestB], soft_s[bestB], soft_l[bestB]);

    // Mood 1: Tonal
    palettes->v2_moods[1].color_a = hsl_to_packed(soft_h[0],
        fmaxf(0.20f, fminf(soft_s[0] * 0.75f, 0.40f)), fmaxf(0.40f, fminf(soft_l[0] * 0.95f, 0.60f)));
    palettes->v2_moods[1].color_b = hsl_to_packed(fmodf(soft_h[0] + 1.0f / 24.0f, 1.0f),
        fmaxf(0.18f, fminf(soft_s[0] * 0.70f, 0.35f)), fmaxf(0.42f, fminf(soft_l[0] * 1.05f, 0.65f)));

    // Mood 2: Vibrant (highest sat)
    int best = 0; float maxS = 0;
    for (int i = 0; i < 3; i++) { if (soft_s[i] > maxS) { maxS = soft_s[i]; best = i; } }
    palettes->v2_moods[2].color_a = hsl_to_packed(soft_h[best],
        fmaxf(0.35f, fminf(soft_s[best] * 1.15f, 0.65f)), fmaxf(0.40f, fminf(soft_l[best], 0.65f)));
    palettes->v2_moods[2].color_b = hsl_to_packed(fmodf(soft_h[best] + 1.0f / 4.0f, 1.0f),
        fmaxf(0.25f, fminf(soft_s[best] * 0.70f, 0.45f)), fmaxf(0.38f, fminf(soft_l[best] + 0.05f, 0.70f)));

    // Mood 3: Ember (warmest)
    int warmIdx = 0; float warmestDist = 1.0f;
    for (int i = 0; i < 3; i++) {
        float d = fminf(fabsf(soft_h[i] - 0.05f), fabsf(soft_h[i] - 0.95f));
        if (d < warmestDist) { warmestDist = d; warmIdx = i; }
    }
    palettes->v2_moods[3].color_a = hsl_to_packed(soft_h[warmIdx],
        fmaxf(0.30f, fminf(soft_s[warmIdx] * 0.90f, 0.50f)), fmaxf(0.40f, fminf(soft_l[warmIdx] + 0.02f, 0.65f)));
    palettes->v2_moods[3].color_b = hsl_to_packed(fmodf(soft_h[warmIdx] + 1.0f / 16.0f, 1.0f),
        fmaxf(0.25f, fminf(soft_s[warmIdx] * 0.75f, 0.40f)), fmaxf(0.45f, fminf(soft_l[warmIdx] + 0.08f, 0.72f)));

    // Mood 4: Glacier (coolest)
    int coolIdx = 0; float coolestD = 999;
    for (int i = 0; i < 3; i++) { float d = fabsf(soft_h[i] - 0.58f); if (d < coolestD) { coolestD = d; coolIdx = i; } }
    palettes->v2_moods[4].color_a = hsl_to_packed(soft_h[coolIdx],
        fmaxf(0.22f, fminf(soft_s[coolIdx] * 0.80f, 0.42f)), fmaxf(0.42f, fminf(soft_l[coolIdx] + 0.02f, 0.68f)));
    palettes->v2_moods[4].color_b = hsl_to_packed(fmodf(soft_h[coolIdx] - 1.0f / 18.0f + 1.0f, 1.0f),
        fmaxf(0.20f, fminf(soft_s[coolIdx] * 0.70f, 0.35f)), fmaxf(0.45f, fminf(soft_l[coolIdx] + 0.08f, 0.72f)));

    // Mood 5: Shadow (darkest)
    int darkIdx = 0; double minL = soft_l[0];
    for (int i = 1; i < 3; i++) { if (soft_l[i] < minL) { minL = soft_l[i]; darkIdx = i; } }
    palettes->v2_moods[5].color_a = hsl_to_packed(soft_h[darkIdx],
        fmaxf(0.20f, fminf(soft_s[darkIdx] * 0.75f, 0.38f)), fmaxf(0.32f, fminf(soft_l[darkIdx] - 0.05f, 0.50f)));
    palettes->v2_moods[5].color_b = hsl_to_packed(fmodf(soft_h[darkIdx] + 0.5f, 1.0f),
        fmaxf(0.20f, fminf(soft_s[darkIdx] * 0.70f, 0.35f)), fmaxf(0.35f, fminf(soft_l[darkIdx] + 0.08f, 0.45f)));

    return palettes;
}

void wtz_mood_palettes_free(WtzMoodPalettes *palettes) {
    g_free(palettes);
}

// ── Smart Auto params ─────────────────────────────────────────────────────

void wtz_compute_smart_auto(const WtzImage *img, WtzSmartAutoParams *result) {
    result->sigma = 15;
    result->sat_boost = 0.85;
    result->brightness = 0.92;
    result->overlay_opacity = 0.35;
    result->overlay_color = 0xFF1A1A1A;
    result->vignette = 0;
    result->grain = 0;

    if (!img || !img->pixels) return;

    int w = img->width, h = img->height;
    int step = MAX(1, MAX(w, h) / 64);
    double sumL = 0, sumS = 0;
    int count = 0;

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
            sumL += lgt;
            sumS += sat;
            count++;
        }
    }

    if (count == 0) return;

    double srcL = sumL / count;
    double srcS = sumS / count;

    result->sigma = 10 + srcS * 25;

    if (srcS < 0.20) result->sat_boost = 1.0;
    else if (srcS > 0.60) result->sat_boost = 0.35;
    else {
        result->sat_boost = 1.0 - (srcS - 0.20) * (0.65 / 0.40);
        result->sat_boost = fmax(0.35, fmin(result->sat_boost, 1.0));
    }

    if (srcL > 0.01) result->brightness = fmax(0.55, fmin(0.40 / srcL, 1.15));
    else result->brightness = 0.85;

    result->overlay_opacity = fmax(0.18, fmin(0.18 + srcL * 0.30, 0.45));
}
