# walltz GTK Port — STATE.md

**Last updated**: 2026-09-06 (commit `11eca7b`)
**Phase**: MVP complete, pattern tiles debug pending
**Binary**: `builddir/src/walltz` — 550KB ELF

---

## 1. Project Identity

| Field | Value |
|-------|-------|
| **Name** | walltz |
| **Version** | 0.1.0 |
| **License** | GPL-3.0-or-later |
| **Author** | kirijin <avel.ronin@gmail.com> |
| **Repo** | https://github.com/kirijin/walltz |
| **Old repo** | https://github.com/kirijin/walltz-kde (Qt6/Kirigami, renamed) |
| **Location** | `/var/home/pavel/src/walltz-gtk/` |
| **Platform** | elementaryOS 7 (GTK4 + Granite-7) |
| **Build system** | Meson + Ninja |
| **Container** | `walltz-dev` distrobox (Fedora 44) |

---

## 2. Architecture

```
Vala UI (GTK4 + Granite-7)
  ├── application.vala    (Gtk.Application entry)
  ├── window.vala         (main window, preview, settings)
  ├── controls.vala       (all render param sliders)
  ├── drop-area.vala      (GTK4 drag-and-drop)
  ├── settings.vala       (GSettings wrapper)
  └── vapi/engine.vapi    (C ↔ Vala bridge)

C Engine (libwalltz-engine.a)
  ├── engine.h/.c         (public API, preset tables)
  ├── blur.c              (Gaussian + box cascade, float pipeline)
  ├── effects.c           (vignette, grain, chromatic aberration)
  ├── pattern.c           (8 geometric + 16 SVG geo tile generators)
  ├── mood.c              (V1/V2 mood palettes, Smart Auto)
  ├── image_io.c          (GdkPixbuf load/save)
  ├── render_composition.c (full pipeline: bg → pattern → effects → fg → CA)
  └── portal.c            (XDG Desktop Portal for set-wallpaper)
```

---

## 3. File Inventory

| File | Lines | Role |
|------|-------|------|
| engine/engine.h | 180 | Public API, struct definitions |
| engine/engine.c | 128 | Preset tables (16 blur, 12 gradient) |
| engine/blur.c | 392 | Gaussian blur (edge-folding), box cascade, float pipeline |
| engine/effects.c | 87 | Vignette, grain, chromatic aberration |
| engine/pattern.c | 411 | Pattern tile generators (24 types) |
| engine/mood.c | 455 | V1 hue histogram, V2 3D RGB, Smart Auto |
| engine/image_io.c | 104 | GdkPixbuf PNG/JPEG/WebP/SVG load/save |
| engine/render_composition.c | 573 | Full render pipeline |
| engine/portal.c | 203 | D-Bus portal integration |
| src/application.vala | 26 | Gtk.Application |
| src/window.vala | 489 | Main window, preview, settings load/save |
| src/controls.vala | 357 | All render param controls |
| src/drop-area.vala | 52 | GTK4 drag-and-drop |
| src/settings.vala | 57 | GSettings wrapper |
| src/vapi/engine.vapi | 183 | C ↔ Vala bridge |
| tests/test-render.c | 58 | Render test (PASSES) |
| tests/test-mood.c | 78 | Mood extraction test (PASSES) |
| tests/test-pattern.c | 36 | Pattern test (FAILS — 0 opaque pixels) |
| **Total** | **~4,200** | |

---

## 4. Build & Run

```bash
# Setup (inside distrobox)
distrobox enter walltz-dev -- meson setup builddir

# Build
distrobox enter walltz-dev -- ninja -C builddir

# Run tests
distrobox enter walltz-dev -- ./builddir/tests/test-render   # PASS
distrobox enter walltz-dev -- ./builddir/tests/test-mood     # PASS
distrobox enter walltz-dev -- ./builddir/tests/test-pattern  # FAIL

# Run app
distrobox enter walltz-dev -- ./builddir/src/walltz
```

---

## 5. Feature Parity Matrix

| Feature | Qt Original | GTK Port | Status |
|---------|-------------|----------|--------|
| Float32 pipeline | ✅ | ✅ | ✅ |
| Gaussian blur (σ≤50) | ✅ | ✅ | ✅ |
| Box blur cascade (σ>50) | ✅ | ✅ | ✅ |
| Image load/save (GdkPixbuf) | ✅ | ✅ | ✅ |
| Background blur mode | ✅ | ✅ | ✅ |
| Background solid color | ✅ | ✅ | ✅ |
| Background gradient (12 presets) | ✅ | ✅ | ✅ |
| Background mood (auto-color) | ✅ | ✅ | ✅ |
| Vignette | ✅ | ✅ | ✅ |
| Grain | ✅ | ✅ | ✅ |
| Chromatic aberration | ✅ | ✅ | ✅ |
| Photo frame | ✅ | ✅ | ✅ |
| Shadow | ✅ | ✅ | ✅ |
| Foreground composition | ✅ | ✅ | ✅ |
| Mood V1 (24-bin hue histogram) | ✅ | ✅ | ✅ |
| Mood V2 (3D RGB histogram) | ✅ | ✅ | ✅ |
| Smart Auto | ✅ | ✅ | ✅ |
| Live preview | ✅ | ✅ | ✅ |
| Open/Save dialogs | ✅ | ✅ | ✅ |
| Drag-and-drop | ✅ | ✅ | ✅ |
| Granite-7 widgets | ❌ | ✅ | ✅ |
| 16 blur presets | ✅ | ✅ | ✅ |
| 12 gradient presets | ✅ | ✅ | ✅ |
| Pattern rendering (24 types) | ✅ | ⚠️ | ❌ |
| D-Bus portal (set wallpaper) | ✅ | ✅ | ✅ |
| Settings persistence (GSettings) | ✅ | ✅ | ✅ |
| AppCenter compliance | ❌ | ✅ | ✅ |

---

## 6. Known Issues

### 6.1 Pattern Tiles Return Transparent (CRITICAL)

**Symptom**: `wtz_generate_pattern_tile()` returns a tile with 0 opaque pixels.

**Debug findings**:
- `draw_dots()` IS called (confirmed via `fprintf` to stderr)
- `draw_circle()` IS called with correct parameters
- `set_pixel()` writes to `tile->pixels[y * stride + x * 4]`
- After function returns, all pixels are still 0

**Likely cause**: Memory corruption or stride mismatch. The `WtzImage` struct is created correctly but pixel writes don't persist.

**Next steps**:
1. Verify `wtz_image_new()` returns valid memory
2. Check `tile->stride == tile->width * 4`
3. Add granular debug inside `set_pixel()` and `draw_circle()`
4. Check if issue is tile-size specific

### 6.2 Deprecated GTK Warnings

- `Gtk.ComboBoxText` deprecated since 4.10 → use `Gtk.DropDown`
- `Gtk.Picture.set_pixbuf` deprecated since 4.12 → use `set_paintable`
- `Gtk.Picture.keep_aspect_ratio` deprecated since 4.8 → use `set_content_fit`

**Priority**: Low (cosmetic, doesn't affect functionality)

---

## 7. AppCenter Requirements Checklist

| Requirement | Status |
|-------------|--------|
| Native GTK4 frontend | ✅ |
| Flatpak manifest | ✅ (`flatpak/org.walltz.walltz.yml`) |
| elementary runtime | ✅ (`org.gnome.Platform` 46) |
| metainfo.xml | ✅ |
| .desktop file | ✅ |
| Icons (32/48/64/128) | ✅ (SVG) |
| GSettings schema | ✅ |
| No Qt/Electron | ✅ |
| Tight sandbox | ✅ (home + dri + portal dbus) |
| No "elementary" in name | ✅ |
| No duplicate apps | ✅ (different from walltz-kde) |

---

## 8. Key Data Structures

### WtzRenderParams (C struct)
```c
typedef struct {
    int target_width, target_height;
    int blur_mode, bg_gradient_style;
    double bg_zoom, bg_blur_angle;
    int blur_radius;
    double saturation_factor, overlay_opacity;
    uint32_t overlay_color;
    double blur_brightness;
    int auto_color;
    uint32_t bg_color;
    int bg_gradient_preset;
    double gradient_angle;
    uint32_t mood_color_a, mood_color_b;
    int bg_pattern_enabled, bg_pattern_type;
    uint32_t bg_pattern_color;
    double bg_pattern_scale, bg_pattern_rotation, bg_pattern_spacing;
    int bg_pattern_random_rotate, bg_pattern_jitter;
    double bg_pattern_grid_amplitude;
    int bg_pattern_mix_enabled;
    double vignette_strength, grain_strength, ca_strength;
    double color_gamma, color_warmth, color_black_lift;
    int photo_frame, photo_frame_width;
    double fg_zoom, pip_zoom;
    int photo_grade, auto_mood, use_v2;
} WtzRenderParams;
```

### WtzImage (C struct)
```c
typedef struct {
    uint8_t *pixels;  // RGBA, premultiplied alpha
    int width, height, stride;
} WtzImage;
```

---

## 9. Render Pipeline Order

1. Background (blur / solid / gradient / mood)
2. Pattern overlay (tiled, 35% opacity, optional jitter)
3. Vignette (radial falloff)
4. Grain (per-pixel noise, LCG RNG)
5. Shadow (around foreground rect)
6. Photo frame (white matte + gray outline)
7. Foreground image (scaled, centered, rounded clip)
8. Chromatic aberration (R/B channel shift)

---

## 10. Preset Tables

### Blur Presets (16)
`default`, `auto`, `serenity`, `focus`, `comfort`, `apple`, `gnome`, `mica`, `acrylic`, `reddit`, `kodachrome`, `polaroid`, `vintage`, `trix`, `coolfilm`

### Gradient Presets (12)
`Sunset Warmth`, `Coral Reef`, `Lemonade`, `Ocean Depths`, `Tokyo Night`, `Arctic`, `Catppuccin`, `Gruvbox`, `Solarized`, `Dusk`, `Everforest`, `Grayscale`

### Mood Palettes (6+6)
- V1: Auto, Soft, Vivid, Warm, Cool, Deep
- V2: Dynamic, Tonal, Vibrant, Ember, Glacier, Shadow

---

## 11. Dependencies

| Package | Version | Purpose |
|---------|---------|---------|
| glib-2.0 | ≥ 2.70 | Core utilities |
| gobject-2.0 | ≥ 2.70 | Object system |
| gio-2.0 | ≥ 2.70 | D-Bus, settings, file I/O |
| gio-unix-2.0 | ≥ 2.70 | Unix-specific GIO |
| gtk4 | ≥ 4.6 | UI toolkit |
| granite-7 | ≥ 7.0 | elementaryOS widgets |
| gdk-pixbuf-2.0 | ≥ 2.42 | Image loading/saving |
| libm | — | Math functions (exp, sin, cos) |

---

## 12. What's Next (Priority Order)

1. **Fix pattern tiles** — debug why pixel writes don't land
2. **AppCenter packaging** — finalize Flatpak, screenshots, release
3. **UI polish** — replace deprecated widgets, add Granite.Toast/AboutDialog
4. **Batch processing** — async queue (GTask) for multiple images
5. **CLI mode** — headless batch rendering

---

## 13. Recovery Commands

```bash
# Full clean rebuild
cd /var/home/pavel/src/walltz-gtk
rm -rf builddir .meson-cache .ninja_deps .ninja_log
distrobox enter walltz-dev -- meson setup builddir
distrobox enter walltz-dev -- ninja -C builddir

# Verify all tests pass
distrobox enter walltz-dev -- ./builddir/tests/test-render
distrobox enter walltz-dev -- ./builddir/tests/test-mood
distrobox enter walltz-dev -- ./builddir/tests/test-pattern  # should pass after fix
```

---

*This file is the canonical state snapshot. Read this first in any future session.*
