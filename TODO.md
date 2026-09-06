# walltz GTK Port — TODO.md

**Last updated**: 2026-09-06
**Phase**: MVP complete, pattern tiles debug pending

---

## Target Stack (elementaryOS 8.1 — Latest)

| Component | Target Version | Current | Source |
|-----------|---------------|---------|--------|
| **Platform** | `io.elementary.Platform` 9.0.0 | 46 (GNOME) | elementary/flatpak-platform |
| **GNOME runtime** | 50 | 46 | Platform 9.0.0 (PR #237) |
| **GTK4** | 4.14+ | 4.12 (GNOME 46) | GNOME 50 |
| **Granite** | 7.8.1 | 7.8.1 | Platform 9.0.0 (PR #227) |
| **Vala** | 0.56.19 | 0.56.19 | Fedora 44 |
| **libportal** | 0.10.0 | 0.9.1 | Platform 9.0.0 (PR #242) |
| **stylesheet** | 8.2.2 | 8.2.2 | Platform 9.0.0 (PR #218) |
| **icons** | 9.0.0 | 9.0.0 | Platform 9.0.0 (PR #247) |

**References**:
- https://releases.elementary.io/ — latest platform 9.0.0 (Aug 28, 2026)
- https://github.com/elementary/flatpak-platform/releases — changelog
- https://www.debugpoint.com/elementary-os-8-1-features/ — OS 8.1 uses GNOME 46 base

---

## CRITICAL (must fix for MVP)

### Pattern tiles return transparent
- **Status**: OPEN
- **Commit**: `11eca7b` (debug logging added)
- **Symptom**: `wtz_generate_pattern_tile()` returns 0 opaque pixels
- **Debug findings**: Dispatch happens, `draw_circle()` called, `set_pixel()` writes, but pixels remain 0
- **Next steps**:
  1. Verify `wtz_image_new()` returns valid memory
  2. Check `tile->stride == tile->width * 4`
  3. Add granular debug inside `set_pixel()` and `draw_circle()`
  4. Check if issue is tile-size specific

---

## HIGH (should fix for AppCenter submission)

### Upgrade to platform 9.0.0 / GNOME 50 runtime
- **Status**: OPEN
- **Current**: `runtime: org.gnome.Platform` version `46`
- **Target**: `runtime: io.elementary.Platform` version `9.0.0` (or `//9.0.0` for Flatpak)
- **Changes needed**:
  - Update `flatpak/org.walltz.walltz.yml`:
    - `runtime: io.elementary.Platform`
    - `runtime-version: '9.0.0'`
    - `sdk: io.elementary.Sdk`
  - Update `meson.build` deps:
    - `granite-7` → `granite-7` (still 7.8.1, but from platform 9.0.0)
    - `gtk4` → version requirement bump if needed
  - Test against new runtime in distrobox

### Replace deprecated GTK widgets
- **Status**: OPEN
- **Widgets to replace**:
  - `Gtk.ComboBoxText` → `Gtk.DropDown` (deprecated since 4.10)
  - `Gtk.Picture.set_pixbuf` → `set_paintable` (deprecated since 4.12)
  - `Gtk.Picture.keep_aspect_ratio` → `set_content_fit` (deprecated since 4.8)
- **File**: `src/controls.vala`, `src/window.vala`

### AppCenter screenshots
- **Status**: OPEN
- **Need**: Full-window screenshot with elementaryOS default settings
- **File**: `data/screenshots/main.png`

### Flatpak manifest finalization
- **Status**: OPEN
- **Check**: runtime version, SDK, finish-args, modules
- **File**: `flatpak/org.walltz.walltz.yml`

---

## MEDIUM (nice to have)

### Batch processing (async queue)
- **Status**: OPEN
- **Description**: Process multiple images with GTask thread pool
- **Qt original**: `QtConcurrent::mapped` + `QFutureWatcher`
- **GTK equivalent**: `GTask` + `GThreadPool`

### CLI batch mode
- **Status**: OPEN
- **Description**: Headless rendering via command-line flags
- **Qt original**: `--input`, `--output`, `--width`, `--height`, `--blur`, etc.
- **Files**: `src/main.vala`, `engine/engine.c`

### Granite.Toast for feedback
- **Status**: OPEN
- **Description**: Ephemeral notifications for save/set-wallpaper
- **File**: `src/window.vala`

### Granite.AboutDialog
- **Status**: OPEN
- **Description**: App info dialog
- **File**: `src/window.vala`

### Granite.HeaderLabel for section headers
- **Status**: OPEN
- **Description**: Bold section labels in controls panel
- **File**: `src/controls.vala`

---

## LOW (defer)

### Pattern mix mode
- **Status**: OPEN
- **Description**: Mix multiple pattern types in one background
- **Qt original**: `bgPatternMixEnabled` + `bgPatternMixMotifs`
- **File**: `engine/pattern.c`

### Pattern random rotation
- **Status**: OPEN
- **Description**: Randomly rotate each tile instance
- **Qt original**: `bgPatternRandomRotate`
- **File**: `engine/pattern.c`

### Procedural texture overlays
- **Status**: OPEN
- **Description**: Light leak, polaroid frame, film border (retro looks)
- **Qt original**: `textureKind` 1/2/3
- **File**: `engine/render_composition.c`

### Photo grade (color grade foreground)
- **Status**: OPEN
- **Description**: Apply sat/gamma/warmth/blackLift to foreground photo
- **Qt original**: `photoGrade`
- **File**: `engine/render_composition.c`

### i18n (internationalization)
- **Status**: OPEN
- **Description**: Translate UI strings to Russian
- **Files**: All `.vala` files

---

## DONE (this session)

- ✅ Float32 pipeline (blur → sat → overlay → brightness → grade → dither)
- ✅ Gaussian blur (edge-folding, branch-free)
- ✅ Box blur cascade (Jarosz/Kutskir radii)
- ✅ Image I/O (GdkPixbuf: PNG, JPEG, WebP, SVG)
- ✅ Background blur mode
- ✅ Background solid color
- ✅ Background gradient (12 presets)
- ✅ Background mood (auto-color)
- ✅ Vignette, grain, chromatic aberration
- ✅ Photo frame, shadow, foreground composition
- ✅ Mood V1 (24-bin hue histogram)
- ✅ Mood V2 (3D RGB histogram)
- ✅ Smart Auto (auto-tune from image stats)
- ✅ Live preview (async thread → UI update)
- ✅ Open/Save dialogs
- ✅ Drag-and-drop
- ✅ Granite-7 integration
- ✅ 16 blur presets + 12 gradient presets
- ✅ D-Bus portal (set wallpaper)
- ✅ Settings persistence (GSettings)
- ✅ AppCenter compliance (Flatpak, metainfo, icons, desktop)

---

## Estimated Effort

| Task | Effort | Priority |
|------|--------|----------|
| Fix pattern tiles | 2-4 hours | CRITICAL |
| Upgrade to platform 9.0.0 | 1-2 hours | HIGH |
| Replace deprecated widgets | 1 hour | HIGH |
| AppCenter screenshots | 30 min | HIGH |
| Flatpak finalization | 1 hour | HIGH |
| Batch processing | 4-6 hours | MEDIUM |
| CLI mode | 2-3 hours | MEDIUM |
| Granite widgets | 1 hour | MEDIUM |
| Pattern mix/rotation | 2 hours | LOW |
| Procedural textures | 3 hours | LOW |
| Photo grade | 1 hour | LOW |
| i18n | 2 hours | LOW |

---

## Session Recovery

```bash
# Read first
cat STATE.md
cat STEPS.md

# Build
cd /var/home/pavel/src/walltz-gtk
distrobox enter walltz-dev -- meson setup builddir
distrobox enter walltz-dev -- ninja -C builddir

# Run tests
distrobox enter walltz-dev -- ./builddir/tests/test-render   # PASS
distrobox enter walltz-dev -- ./builddir/tests/test-mood     # PASS
distrobox enter walltz-dev -- ./builddir/tests/test-pattern  # FAIL (0 opaque pixels)

# Run app
distrobox enter walltz-dev -- ./builddir/src/walltz
```

---

*This file tracks remaining work. Move items to DONE as they are completed.*
