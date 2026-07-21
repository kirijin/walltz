#!/usr/bin/env bash
# Verify all four walltz features are wired correctly
# Run from host (not inside distrobox)
set -euo pipefail
ROOT="/var/home/pavel/src/walltz/walltz"
PASS=0; FAIL=0
ok() { msg="  [PASS] $1"; PASS=$((PASS+1)); echo "$msg"; }
fail() { msg="  [FAIL] $1"; FAIL=$((FAIL+1)); echo "$msg"; }

echo "=== walltz feature verification ==="

# 1) Build check — run from host via distrobox
echo "--- Build ---"
DISTROBOX="distrobox enter walltz-dev --"
BUILD_OUTPUT=$($DISTROBOX bash -c "cd '$ROOT/build' && make -j\$(nproc) 2>&1" 2>&1) || true
if echo "$BUILD_OUTPUT" | grep -q "Built target walltz"; then
    ok "Build succeeds"
else
    echo "   Build output: $BUILD_OUTPUT" | tail -3
    fail "Build failed"; exit 1
fi

# 2) Swap button — ToolButton with swap-panels icon
echo "--- Feature 1: Swap X/Y button ---"
grep -q 'icon.name: "swap-panels"' "$ROOT/src/Main.qml" \
  && ok "Swap button with swap-panels icon found" \
  || fail "Swap button not found"
grep -q 'processor.aspectMode = 0' "$ROOT/src/Main.qml" \
  && ok "Swap resets aspect mode" \
  || fail "Swap doesn't reset aspect mode"

# 3) Braille loading animation
echo "--- Feature 2: Braille spinner ---"
grep -q 'u280B' "$ROOT/src/Main.qml" \
  && ok "Braille spinner frames found" \
  || fail "Braille frames missing"
grep -q 'interval: 100; repeat: true' "$ROOT/src/Main.qml" \
  && ok "Braille animation timer (100ms) found" \
  || fail "Braille timer missing"
grep -q "Kirigami.LoadingPlaceholder" "$ROOT/src/Main.qml" \
  && fail "Old LoadingPlaceholder still present" \
  || ok "Old LoadingPlaceholder removed"

# 4) Background rotation slider
echo "--- Feature 4: Background rotation ---"
grep -q 'bgBlurAngle' "$ROOT/src/WallpaperProcessor.h" \
  && ok "bgBlurAngle Q_PROPERTY declared" \
  || fail "bgBlurAngle not in header"
grep -q 'm_bgBlurAngle' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "bgBlurAngle used in renderWallpaper" \
  || fail "bgBlurAngle not in renderWallpaper"
grep -q 'onBgBlurAngleChanged' "$ROOT/src/Main.qml" \
  && ok "bgBlurAngle debounce wired in QML" \
  || fail "bgBlurAngle debounce missing"

# 5) Gaussian blur
echo "--- Feature 3: Gaussian blur ---"
grep -q 'gaussianBlur\|double sigma =' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "Gaussian blur implementation found" \
  || fail "Gaussian blur code missing"
grep -q 'boxBlurPass' "$ROOT/src/WallpaperProcessor.cpp" \
  && fail "Old boxBlurPass still present" \
  || ok "boxBlurPass fully removed"
grep -q 'boxBlurPass' "$ROOT/src/WallpaperProcessor.h" \
  && fail "boxBlurPass declaration still in header" \
  || ok "boxBlurPass declaration removed from header"
# Verify kernel is >1D (not uniform) — Gaussian uses exp()
grep -q 'std::exp' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "Gaussian kernel uses exp()" \
  || fail "No exp() in blur code"

# 6) Slider icons reset on click — no bare Kirigami.Icon or Switches remain
echo "--- Feature 6: Icon reset buttons ---"
grep -q 'Kirigami.Icon' "$ROOT/src/Main.qml" \
  && fail "Bare Kirigami.Icon still present" \
  || ok "All slider icons converted to ToolButtons"
grep -q 'Controls.Switch' "$ROOT/src/Main.qml" \
  && fail "Controls.Switch still present in sliders" \
  || ok "All slider Switches removed"
for icon in contrast noise blur color-management zoom-original transform-rotate; do
  grep -q "icon.name: \"$icon\"" "$ROOT/src/Main.qml" \
    && ok "ToolButton icon $icon found" \
    || fail "ToolButton icon $icon missing"
done

# 7) Reset effects button
echo "--- Feature 7: Reset effects button ---"
grep -q 'Reset effects' "$ROOT/src/Main.qml" \
  && ok "Reset effects button found" \
  || fail "Reset effects button missing"
grep -q 'vignetteStrength = 0.0' "$ROOT/src/Main.qml" \
  && ok "Reset effects resets vignette" \
  || fail "Reset effects doesn't reset vignette"

# 8) Improved vignette rendering
echo "--- Feature 8: Stronger vignette ---"
grep -q 'fadeStart' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "Vignette uses dynamic fadeStart" \
  || fail "Vignette missing fadeStart"
grep -q '200 \* s' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "Vignette uses stronger alpha (200)" \
  || fail "Vignette alpha still 120 or lower"

echo
echo "Result: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ] && echo "All checks OK" || exit 1
