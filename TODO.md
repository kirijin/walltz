# walltz GTK Port — TODO.md

**Last updated**: 2026-09-06
**Phase**: MVP complete, pattern tiles debug pending

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

*This file tracks remaining work. Move items to DONE as they are completed.*
