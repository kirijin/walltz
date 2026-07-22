# Walltz

Create wallpapers from images — blur, color extraction, gradient overlay, and a suite of post-processing effects. Built with Qt6 + Kirigami for KDE Plasma.

## Features

- **Drop zone**: drag & drop images to process
- **Blurred background**: adjustable Gaussian blur (sigma up to 120), saturation boost, gradient overlay
- **Auto‑gradient moods**: 6 mood palettes extracted from image colors (V1 histogram + V2 3D RGB cube)
- **12 gradient presets**: Sunset Warmth, Ocean Depths, Tokyo Night, Catppuccin, Gruvbox, and more
- **Aspect ratios**: 1:1, 4:3, 16:9, 16:10, 21:9, 32:9 + free mode
- **Post‑processing**: vignette, photo grain (film noise), chromatic aberration
- **Photo frame**: rounded‑corner border overlay
- **Multi‑image queue**: batch process with preview cycling
- **Live preview**: two‑layer crossfade, debounced regeneration
- **Themed icons**: SVGs re‑tinted via `MultiEffect.colorization` to match KDE accent color

## Dependencies

- Qt6 (Core, Gui, Qml, Quick, QuickControls2, Concurrent)
- KF6 (Kirigami, KCoreAddons, KI18n, KWindowSystem)
- Extra CMake Modules (ECM)
- C++20 compiler (GCC 13+ or Clang 16+)
- CMake ≥ 3.20

### Fedora / Bazzite

```
sudo dnf install qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtquickcontrols2-devel \
  qt6-qt5compat-devel qt6-qtimageformats-devel qt6-qtshadertools-devel qt6-qtwayland \
  kf6-kirigami-devel kf6-kcoreaddons-devel kf6-ki18n-devel kf6-kwindowsystem-devel \
  kf6-extra-cmake-modules cmake gcc-c++
```

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(nproc)
```

Run:

```bash
./build/bin/walltz
```

## Development

This project uses a `distrobox` container (`walltz-dev`, Fedora 44) for build isolation.

Quick verification after changes:

```bash
bash scripts/verify-features.sh
```

### Flatpak

```bash
flatpak-builder --user --install build-dir flatpak/org.walltz.walltz.yml
flatpak run org.walltz.walltz
```

### AppImage

```bash
bash scripts/build-appimage.sh
# Output: build-appimage/Walltz-<version>-x86_64.AppImage
```

## License

GNU General Public License v3.0 or later. See [COPYING](COPYING).
