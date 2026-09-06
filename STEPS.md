# walltz GTK Port — STEPS.md

**Last updated**: 2026-09-06
**Phase**: MVP complete, pattern tiles debug pending

---

## Commit History (chronological)

### 1. `b311289` — Initial scaffold (2026-09-05)

**What**: Project skeleton — Meson build, C engine stubs, Vala UI shell.

**Files created**:
- `meson.build` — top-level project definition
- `engine/meson.build` — C library build
- `engine/engine.h` — public API with WtzImage, WtzRenderParams structs
- `engine/engine.c` — preset tables (16 blur, 12 gradient)
- `engine/blur.c` — stub
- `engine/effects.c` — stub
- `engine/pattern.c` — stub
- `engine/mood.c` — stub
- `engine/image_io.c` — stub
- `engine/render_composition.c` — stub
- `src/meson.build` — Vala executable build
- `src/application.vala` — Gtk.Application
- `src/window.vala` — stub
- `src/controls.vala` — stub
- `src/drop-area.vala` — stub
- `src/settings.vala` — stub
- `src/vapi/engine.vapi` — C bindings
- `data/` — desktop, metainfo, gschema, icons
- `flatpak/org.walltz.walltz.yml` — Flatpak manifest
- `tests/` — test scaffold

**Verified**: Binary compiles and links. 82KB ELF.

---

### 2. `dde0edf` — Float pipeline, GdkPixbuf I/O, Granite-7, box blur cascade (2026-09-05)

**What**: Core image processing engine.

**Changes**:
- `engine/blur.c` — Full float32 pipeline:
  - Gaussian blur with edge-folding (branch-free, auto-vectorizing)
  - Box blur cascade (Jarosz/Kutskir radii) for σ>50
  - Adaptive dispatch: Gaussian for σ≤50, box for σ>50
  - Saturation boost (BT.601 luma, float)
  - Overlay blend (premultiplied invariant)
  - Brightness (float multiply)
  - Color grade (gamma LUT → warmth → blackLift)
  - Bayer 8×8 ordered dither at 8-bit quantize
- `engine/image_io.c` — GdkPixbuf load/save (PNG, JPEG, WebP, SVG)
- `engine/engine.h` — Added `wtz_blur_image()` declaration
- `meson.build` — Added granite-7, gdk-pixbuf-2.0, libm deps
- `src/application.vala` — Gtk.Application (Granite.Application removed)

**Verified**: test-render passes (800x600 red square, center pixel correct).

**Issue encountered**: Granite.Application not in granite-7 (removed). Switched to Gtk.Application.

---

### 3. `e895c70` — Render composition: full pipeline (2026-09-05)

**What**: Complete render pipeline from source image to wallpaper.

**Changes**:
- `engine/render_composition.c` — Full pipeline:
  - Background blur mode (scaled source → darken → blur)
  - Background solid color (auto/mood/manual)
  - Background gradient (12 presets + mood)
  - Vignette (radial falloff)
  - Grain (per-pixel noise, LCG RNG)
  - Shadow (around foreground rect)
  - Photo frame (white matte + gray outline)
  - Foreground image (scaled, centered, rounded clip)
  - Chromatic aberration (R/B channel shift)
  - Self-similar composition (5% margin, golden ratio)

**Verified**: test-render passes. Center pixel red (foreground), corner white (blurred bg).

---

### 4. `c9479ef` — Mood palette extraction: V1 + V2 (2026-09-05)

**What**: Auto-color extraction from images.

**Changes**:
- `engine/mood.c` — Full implementation:
  - V1: 24-bin hue histogram, golden-angle (137.5°) secondary
  - V2: 8×8×8 RGB bins, top-3 centroids, max-contrast pairing
  - Smart Auto: sigma/sat/brightness/overlay from image stats
- `engine/engine.h` — Updated Smart Auto signature (out-param)
- `src/vapi/engine.vapi` — Updated bindings
- `src/window.vala` — Use return-value Smart Auto
- `tests/test-mood.c` — C test with blue/green image

**Verified**: test-mood passes. V1/V2 moods extracted, Smart Auto computed.

**Issue encountered**: Vala `out` params vs C pointers. Changed to return-value API.

---

### 5. `75452fa` — Full UI: live preview, controls, drop area, save (2026-09-05)

**What**: Interactive GTK4 UI.

**Changes**:
- `src/window.vala` — Full window:
  - Header bar (open, save, set wallpaper buttons)
  - Paned layout (controls left, preview right)
  - Gtk.Picture for preview
  - Status bar with progress
  - Open/Save dialogs
  - Set wallpaper via D-Bus portal
- `src/controls.vala` — All render params:
  - Resolution (width/height entries)
  - Background mode (combo)
  - Blur preset (combo)
  - Gradient preset (combo)
  - Mood (combo)
  - Blur radius, saturation, bg zoom, gradient angle (scales)
  - Vignette, grain, CA (scales)
  - Photo frame (switch + width scale)
  - Foreground zoom, PiP zoom (scales)
  - Reset button
- `src/drop-area.vala` — GTK4 drag-and-drop (URI list)
- `engine/portal.c` — XDG Desktop Portal integration

**Verified**: App runs, opens images, renders preview, saves PNG.

**Issue encountered**: GTK4 DnD API changed (`drop` signal instead of `on_value_received`). Fixed.

---

### 6. `6c26295` — Pattern tile generation: 8 geometric + 16 SVG geo icons (2026-09-05)

**What**: Pattern tile generators.

**Changes**:
- `engine/pattern.c` — Full implementation:
  - 8 geometric: dots, stripes H/V/D, checkerboard, chevron, diamonds, crosshatch
  - 16 SVG geo: squares, triangles, circles, rings, lines, worms, dots, diamonds, square, triangle, circle, diamond, ring, wave, cross, disc
  - Direct pixel manipulation (no Cairo)
  - Helper functions: set_pixel, fill_rect, draw_circle, draw_line

**Verified**: Code compiles. test-pattern FAILS (0 opaque pixels).

**Issue**: Pattern tiles return transparent. Debug in progress.

---

### 7. `9dcdf2d` — Pattern rendering integrated into render pipeline (2026-09-05)

**What**: Tiling patterns across the canvas.

**Changes**:
- `engine/render_composition.c` — Added `render_pattern()`:
  - Tiles pattern across canvas with configurable spacing
  - Jitter support (sine/cosine displacement)
  - 35% opacity blend
  - Called after background, before vignette

**Verified**: Code compiles. Pattern still transparent (inherited bug).

---

### 8. `b3af354` — D-Bus portal integration for setting wallpaper (2026-09-05)

**What**: Set rendered image as desktop wallpaper.

**Changes**:
- `engine/portal.c` — Full implementation:
  - XDG Desktop Portal `SetWallpaperURI` call
  - Signal subscription for response
  - 30s timeout
  - Fallback to Pictures folder on failure
- `engine/engine.h` — Added portal function declarations
- `engine/meson.build` — Added portal.c, gio-unix dep
- `src/window.vala` — Set wallpaper button handler

**Verified**: Code compiles. Portal call works on GNOME (not tested on elementaryOS).

---

### 9. `6197733` — Settings persistence via GSettings (2026-09-06)

**What**: Remember render params between sessions.

**Changes**:
- `src/settings.vala` — WalltzSettings class (all 30+ params)
- `src/window.vala` — load_settings() on startup, save_settings() on close
- `src/meson.build` — Added settings.vala
- Color parse/string helpers (hex ↔ uint32)

**Verified**: Settings load/save correctly. App restores previous state.

**Issue encountered**: `uint32_t` not available in Vala. Changed to `uint32`.

---

### 10. `11eca7b` — Pattern rendering: debug logging (2026-09-06)

**What**: Added debug logging to pattern generation.

**Changes**:
- `engine/pattern.c` — Added `fprintf(stderr, ...)` to:
  - `wtz_generate_pattern_tile()` entry
  - `draw_dots()` with parameters
  - Each dot position
- `tests/test-pattern.c` — Debug test

**Verified**: Debug output confirms dispatch happens but pixels still 0.

**Root cause identified**: The `WtzImage` struct is created correctly, `draw_circle()` is called with correct parameters, but pixel writes don't persist. Likely a memory corruption or stride issue.

---

## Wrong Turns & Lessons

### 1. Granite.Application removed in granite-7

**Problem**: `Granite.Application` class doesn't exist in granite-7 (only in granite 6.x).

**Solution**: Switched to plain `Gtk.Application`.

**Lesson**: Check API existence before committing to a dependency.

### 2. Vala `out` params vs C pointers

**Problem**: Initial Smart Auto API used `out` params in Vala, but C used pointers. Type mismatch.

**Solution**: Changed to return-value API (`WtzSmartAutoParams wtz_compute_smart_auto(img)`).

**Lesson**: Design C API to be Vala-friendly from the start.

### 3. GTK4 DnD API change

**Problem**: `Gtk.DropTarget.on_value_received` doesn't exist in GTK4.

**Solution**: Use `drop` signal with `Value.get_boxed()` / `Value.get_string()`.

**Lesson**: GTK4 API differs significantly from GTK3. Check docs.

### 4. Pattern tiles transparent (UNRESOLVED)

**Problem**: `wtz_generate_pattern_tile()` returns transparent tiles.

**Debug findings**:
- Dispatch function IS called
- `draw_circle()` IS called with correct params
- `set_pixel()` writes to `pixels[y * stride + x * 4]`
- But pixels remain 0 after return

**Likely cause**: Memory corruption or stride mismatch.

**Lesson**: When pixel manipulation doesn't work, verify memory allocation and stride calculation first.

---

## Verification History

| Commit | test-render | test-mood | test-pattern | App runs |
|--------|-------------|-----------|--------------|----------|
| b311289 | — | — | — | ✅ (stub) |
| dde0edf | ✅ | — | — | ✅ |
| e895c70 | ✅ | — | — | ✅ |
| c9479ef | ✅ | ✅ | — | ✅ |
| 75452fa | ✅ | ✅ | — | ✅ |
| 6c26295 | ✅ | ✅ | ❌ | ✅ |
| 9dcdf2d | ✅ | ✅ | ❌ | ✅ |
| b3af354 | ✅ | ✅ | ❌ | ✅ |
| 6197733 | ✅ | ✅ | ❌ | ✅ |
| 11eca7b | ✅ | ✅ | ❌ | ✅ |

---

## Recovery from Scratch

```bash
# 1. Clone
git clone https://github.com/kirijin/walltz.git
cd walltz

# 2. Build
distrobox enter walltz-dev -- meson setup builddir
distrobox enter walltz-dev -- ninja -C builddir

# 3. Verify
distrobox enter walltz-dev -- ./builddir/tests/test-render
distrobox enter walltz-dev -- ./builddir/tests/test-mood
distrobox enter walltz-dev -- ./builddir/tests/test-pattern  # should pass after fix

# 4. Run
distrobox enter walltz-dev -- ./builddir/src/walltz
```

---

*This file documents the commit-by-commit progression. Read STATE.md for current status.*
