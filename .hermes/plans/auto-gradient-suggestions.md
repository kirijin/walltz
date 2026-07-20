# Auto-Gradient Suggestion System: Quality Algorithm Plan

We must aim for quality. The current auto-gradient system (`bgGradientStyle === 2`)
extracts a single mood-fit gradient pair from the image. We need to offer the user
**two rows of suggestions** instead — a short-term array and a long-term array.

---

## Row 1: Short-term suggestion array (implement now)

Quick wins that dramatically improve the current histogram-based extraction.

### 1a. 3D RGB histogram (replace 24-bin hue histogram)

**Current:** 24-bin hue-only histogram bins hues = 1D, misses saturation/value info
**Target:** 16×16×16 RGB cube (4096 bins) = captures color distribution in 3D

```cpp
bins_r = 16, bins_g = 16, bins_b = 16
index = (r_quantized * 16 + g_quantized) * 16 + b_quantized
```

- Count pixels per RGB bin → find top-8 populated bins
- For each bin, average its R,G,B into a representative color
- Score each: `pop * chroma²` where `chroma = max(r,g,b) - min(r,g,b)`
- Pick top 3 distinct bin-colors as primary palette

### 1b. WSMeans refinement (1–2 iterations)

After picking top-3 from histogram, run 1–2 iterations of Weighted
Spatial-Means (WSMeans) to pull cluster centroids closer to the
perceptual center of each color mass.

```cpp
// Given N initial centroids c_i:
// Weight = chroma² * pixel (prefers colorful pixels)
// New centroid = weighted average of nearby in-bin pixels
```

Adds ~2ms, prevents off-center picks.

### 1c. Desaturation guard

When all top-3 palette colors have chroma < 0.15 (grayish image):
- Fall back to mood-adaptive defaults instead of muted grays
- Warm → warm orange, Cool → steel blue, Vivid → high-chroma orange+blue

### 1d. QML: Two-row layout for short-term suggestions

```
Row 1: [suggestion 1] [suggestion 2] [suggestion 3] (short-term)
Row 2: [mood: Auto|Soft|Vivid|Warm|Cool|Deep]
```

- Each suggestion is a clickable gradient swatch with name label
- Click applies that pair as `gradientColor1` / `gradientColor2`
- Mood row re-filters the suggestions
- Flow layout wraps on narrow windows

**Estimate: 2–4 hours coding, C++ + QML.**

---

## Row 2: Long-term suggestion array (deferred quality pass)

Full perceptual color science — the gold standard.

### 2a. Wu 3D quantizer (32×32×32, incremental)

- Build 3D histogram with 32 bins per channel (32768 bins)
- Compute running sum, sum² per bin for fast box-splitting
- Split recursively: always split the box with highest variance × population
- Stop at K boxes (K = 6–10 for gradient pairs)
- Each box's color-weighted centroid → palette color

**Reference:** Xiaolin Wu (1991) "Efficient Statistical Computations for
Optimal Color Quantization" / Material-Color-Utilities implementation.

### 2b. Scorer: `pop × chroma² × (1 − |0.5 − lightness|)`

Score each Wu centroid by:
- **Population** (dominance)
- **Chroma²** (purity — prefers saturated colors over gray)
- **Lightness weighting** (prefers mid-tones over extremes)

Top-6 scored colors → gradient pair candidates.

### 2c. Perceptual gradient pairing (HCT/CAM16)

Instead of pairing light-dark or random, use HCT hue and tone:
- Pair colors with 60°–180° hue separation (complementary or split-complementary)
- Tone difference 0.2–0.5 (enough contrast for readable gradient)
- Avoid pairing two very-light or two very-dark colors

### 2d. Scene-aware mood model

Classify image scene:
- **Landscape** → greens + blues, low contrast, nature tones
- **Portrait/people** → warm skin tones, rose, gentle contrast
- **Vibrant/city** → high chroma, bold complementary pairs
- **Dark/low-key** → deep tones + bright accent

Each scene type gets different pairing rules and fallback defaults.

### 2e. QML: Full suggestion carousel

```
Row 2 (long-term):
┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐
│ WARM │ │ COOL │ │SOFT  │ │VIVID │ │ DEEP │ ← mood filter
└──────┘ └──────┘ └──────┘ └──────┘ └──────┘
┌──────────┐ ┌──────────┐ ┌──────────┐ kfr
│ Sunset   │ │ Ocean    │ │ Aurora   │ ← long-term suggestions
│ #ff6b6b  │ │ #0abde3  │ │ #00b894  │
│ #feca57  │ │ #48dbfb  │ │ #6c5ce7  │
└──────────┘ └──────────┘ └──────────┘
```

Each suggestion swatch shows:
- The gradient (56×40)
- Name label
- "→ Pick" on hover/click sets the gradient

Scrolling horizontal carousel for 8–12 suggestions.

**Estimate: 1–2 weeks (Wu quantizer + HCT port).**

---

## Implementation order

Phase | What | When | Est.
------|------|------|-----
S-1   | 3D RGB histogram (replace 1D hue) | Now | 30m
S-2   | WSMeans refinement (1 iteration) | Now | 30m
S-3   | Desaturation guard | Now | 10m
S-4   | Short-term suggestion QML row | Now | 1h
L-1   | Wu 3D quantizer | Next week | 4h
L-2   | HCT/CAM16 scorer + pairing | Next week | 6h
L-3   | Scene classifier | Future | 4h
L-4   | Long-term suggestion carousel | Future | 2h

**Total: ~4h short-term, ~16h long-term.**
