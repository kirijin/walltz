#!/usr/bin/env bash
# Verify all walltz features are wired correctly
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
grep -q 'std::exp' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "Gaussian kernel uses exp()" \
  # 6) Text reset buttons instead of icon-only ToolButtons
  echo "--- Feature 6: Text reset buttons ---"
  # 8 text buttons exactly (V, G, CA, Blur, Sat, Zoom, Rot, Angle)
  BTN_COUNT=$(grep -c 'text: i18n' "$ROOT/src/Main.qml" | head -1 || true)
  for btn in '"V"' '"G"' '"CA"' '"Blur"' '"Sat"' '"Zoom"' '"Rot"' '"Angle"'; do
    grep -q "text: i18n($btn)" "$ROOT/src/Main.qml" \
      && ok "Text button $btn found" \
      || fail "Text button $btn missing"
  done
  # 1 Switch allowed (photo frame toggle) — no other switches
  SWITCH_COUNT=$(grep -c 'Controls.Switch' "$ROOT/src/Main.qml" || true)
  if [ "$SWITCH_COUNT" -eq 1 ]; then
    ok "One Controls.Switch (photo frame) — correct"
  else
    fail "Expected 1 Controls.Switch, found $SWITCH_COUNT"
  fi
  # No icon-only ToolButtons for effects remain (2 allowed: swap + detect screen)
  ICON_BTNS=$(grep -c "display: Controls.AbstractButton.IconOnly" "$ROOT/src/Main.qml" || true)
  [ "$ICON_BTNS" -eq 2 ] && ok "Only 2 icon ToolButtons (swap + detect) — correct" || fail "Expected 2 icon ToolButtons, found $ICON_BTNS"

# 7) Reset effects button
echo "--- Feature 7: Reset effects button ---"
grep -q 'Reset effects' "$ROOT/src/Main.qml" \
  && ok "Reset effects button found" \
  || fail "Reset effects button missing"
grep -q 'caStrength = 0.0' "$ROOT/src/Main.qml" \
  && ok "Reset effects includes CA" \
  || fail "CA not in Reset effects button"

# 8) Improved vignette rendering
echo "--- Feature 8: Stronger vignette ---"
grep -q 'fadeStart' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "Vignette uses dynamic fadeStart" \
  || fail "Vignette missing fadeStart"
grep -qF '(200 * s)' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "Vignette uses stronger alpha (200)" \
  || fail "Vignette alpha still 120 or lower"
grep -q 'std::sqrt((W/2.0)' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "Vignette radius matches image diagonal" \
  || fail "Vignette radius still qMax-based"

# 9) Chromatic aberration
echo "--- Feature 9: Chromatic aberration ---"
grep -q 'caStrength' "$ROOT/src/WallpaperProcessor.h" \
  && ok "caStrength Q_PROPERTY declared" \
  || fail "caStrength missing from header"
grep -q 'm_caStrength' "$ROOT/src/WallpaperProcessor.h" \
  && ok "caStrength member declared" \
  || fail "caStrength member missing"
grep -q 'setCaStrength' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "caStrength setter defined" \
  || fail "caStrength setter missing"
grep -qF 'maxShift = m_caStrength * std::min(W, H) * 0.05' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "CA maxShift proportional (5% of min dimension)" \
  || fail "CA maxShift not proportional"
grep -q 'text: i18n("CA")' "$ROOT/src/Main.qml" \
  && ok "CA text button in QML" \
  || fail "CA text button missing in QML"
grep -q 'onCaStrengthChanged' "$ROOT/src/Main.qml" \
  && ok "CA debounce wired" \
  || fail "CA debounce not wired"

# 10) Preview crossfade (no canvas bleed-through)
echo "--- Feature 10: Preview crossfade ---"
grep -q 'id: previewA' "$ROOT/src/Main.qml" \
  && ok "Two-layer preview (previewA)" \
  || fail "Missing previewA layer"
grep -q 'id: previewB' "$ROOT/src/Main.qml" \
  && ok "Two-layer preview (previewB)" \
  || fail "Missing previewB layer"
grep -q 'crossfadePreview' "$ROOT/src/Main.qml" \
  && ok "crossfadePreview function defined" \
  || fail "crossfadePreview function missing"
# Verify previewA stays opaque (no opacity animation on A)
grep -q 'NumberAnimation.*previewB.*opacity.*to: 1.0' "$ROOT/src/Main.qml" \
  && ok "Only previewB fades in (previewA stays opaque)" \
  || fail "Crossfade still animates previewA"
# Ensure no ParallelAnimation (old style)
grep -q 'ParallelAnimation' "$ROOT/src/Main.qml" \
  && fail "Old ParallelAnimation crossfade still present" \
  || ok "New single-target crossfade, no ParallelAnimation"

# 11) No SpinBoxes remain
echo "--- Feature 11: No SpinBox inputs ---"
SPINBOX_COUNT=$(grep -c 'Controls.SpinBox' "$ROOT/src/Main.qml" || true)
if [ "$SPINBOX_COUNT" -eq 0 ]; then
  ok "All SpinBox (text input) fields removed"
else
  fail "Expected 0 SpinBox, found $SPINBOX_COUNT"
fi

# 12) Visual polish: layer-based rounded clip + drop shadow
echo "--- Feature 12: Visual polish ---"
# dropZone no longer uses clip:true — layer.enabled clips to rounded shape
! grep -q 'dropZone.*clip: true' "$ROOT/src/Main.qml" \
  && ok "No clip:true on dropZone (layer handles rounded clip)" \
  || fail "dropZone still has clip:true"
grep -q 'cornerRadius' "$ROOT/src/Main.qml" \
  && ok "Uses Kirigami.Units.cornerRadius" \
  || fail "Still uses smallSpacing radius"
grep -q 'MultiEffect' "$ROOT/src/Main.qml" \
  && ok "MultiEffect imported and configured" \
  || fail "MultiEffect missing"
grep -q 'layer.enabled: true' "$ROOT/src/Main.qml" \
  && ok "Drop shadow layer enabled" \
  || fail "Layer not enabled"
grep -q 'shadowBlur: 16' "$ROOT/src/Main.qml" \
  && ok "Drop shadow blur 16px" \
  || fail "Shadow blur not 16"
grep -q 'shadowVerticalOffset: 4' "$ROOT/src/Main.qml" \
  && ok "Drop shadow vertical offset 4px" \
  || fail "Shadow offset not 4"

# 13) Signal handlers (no missing Connections)
echo "--- Feature 13: Signal handlers ---"
grep -q 'onAspectModeChanged' "$ROOT/src/Main.qml" \
  && ok "onAspectModeChanged handler wired" \
  || fail "Missing onAspectModeChanged"
grep -q 'onAutoMoodChanged' "$ROOT/src/Main.qml" \
  && ok "onAutoMoodChanged handler wired" \
  || fail "Missing onAutoMoodChanged"

# 14) No stale imagePreview references
echo "--- Feature 14: No stale references ---"
IM=$(grep -c 'imagePreview' "$ROOT/src/Main.qml" || true)
if [ "$IM" -eq 0 ]; then
  ok "All imagePreview references migrated to crossfadePreview"
else
  fail "Found $IM imagePreview references (should be 0)"
fi

# 15) Ghost outline (transparent fill + border only when empty)
echo "--- Feature 15: Ghost outline ---"
grep -q '"transparent"' "$ROOT/src/Main.qml" \
  && ok "dropZone uses transparent fill when empty" \
  || fail "Missing transparent fill"
grep -q 'border.color: dropArea.fileCount === 0 ?' "$ROOT/src/Main.qml" \
  && ok "Border conditionally hidden when image loaded" \
  || fail "Border not conditional on fileCount"

# 16) Text buttons (no more icon-only ToolButtons for effects)
echo "--- Feature 16: Text reset buttons ---"
for btn in '"V"' '"G"' '"CA"' '"Blur"' '"Sat"' '"Zoom"' '"Rot"' '"Angle"'; do
  grep -q "text: i18n($btn)" "$ROOT/src/Main.qml" \
    && ok "Reset button $btn" \
    || fail "Missing text button $btn"
done

# 17) Highlighted selection (accent color)
echo "--- Feature 17: Highlighted selection ---"
H_COUNT=$(grep -c 'highlighted: checked' "$ROOT/src/Main.qml" || true)
if [ "$H_COUNT" -ge 2 ]; then
  ok "Buttons use highlighted: checked for accent color ($H_COUNT occurrences)"
else
  fail "Expected >= 2 highlighted: checked, found $H_COUNT"
fi

# 18) No separators (including ratio divider line)
echo "--- Feature 18: No separators ---"
! grep -q 'Kirigami.Separator' "$ROOT/src/Main.qml" \
  && ok "No Kirigami.Separator remain" \
  || fail "Separator still present"
! grep -q '// Separator' "$ROOT/src/Main.qml" \
  && ok "No ratio divider Rectangle" \
  || fail "Ratio divider still present"

# 19) Photo frame properties (and preview accuracy)
echo "--- Feature 19: Photo frame ---"
grep -q 'Q_PROPERTY(bool photoFrame' "$ROOT/src/WallpaperProcessor.h" \
  && ok "photoFrame Q_PROPERTY declared" \
  || fail "Missing photoFrame property"
grep -q 'Q_PROPERTY(int photoFrameWidth' "$ROOT/src/WallpaperProcessor.h" \
  && ok "photoFrameWidth Q_PROPERTY declared" \
  || fail "Missing photoFrameWidth property"
grep -q 'm_photoFrame = false;' "$ROOT/src/WallpaperProcessor.h" \
  && ok "photoFrame defaults to false" \
  || fail "Missing photoFrame default"
grep -q 'Photo frame' "$ROOT/src/Main.qml" \
  && ok "Photo frame toggle in QML" \
  || fail "Missing photo frame QML toggle"
grep -q 'to: 25' "$ROOT/src/Main.qml" \
  && ok "Photo frame width max 25px" \
  || fail "Photo frame width not bounded at 25"
grep -q 'std::min(W, H) / 500.0' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "Frame width scales proportionally to output" \
  || fail "Frame width not proportional"
grep -q 'FRAME_RADIUS' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "FRAME_RADIUS = 2 defined" \
  || fail "FRAME_RADIUS missing"
grep -q 'Antialiasing' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "Antialiasing enabled for smooth corners" \
  || fail "Antialiasing missing"
grep -q "qBound(5, w, 25)" "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "setter bounds at 25px" \
  || fail "Setter doesn't cap at 25"
grep -q '400.0' "$ROOT/src/WallpaperProcessor.cpp" \
  && fail "generatePreview still uses 400px limit" \
  || ok "generatePreview no longer uses 400px limit"
grep -q 'interval: 700' "$ROOT/src/Main.qml" \
  && ok "Preview debounce 700ms (for full-res render)" \
  || fail "Debounce interval not 700ms"

# 20) Braille processing indicator
echo "--- Feature 20: Braille processing indicator ---"
grep -q 'processingIndicator' "$ROOT/src/Main.qml" \
  && ok "Braille indicator label exists" \
  || fail "Missing braille indicator"
grep -q 'visible: processor.busy' "$ROOT/src/Main.qml" \
  && ok "Indicator visible when busy" \
  || fail "Indicator not tied to busy"
grep -q '\\u28B8' "$ROOT/src/Main.qml" \
  && ok "Braille animation frames defined" \
  || fail "Missing braille frames"

# 21) O(n) box blur optimizations
echo "--- Feature 21: Performance optimizations ---"
grep -q 'boxBlurH' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "boxBlurH (parallel O(n) horizontal blur)" \
  || fail "Missing boxBlurH"
grep -q 'boxBlurV' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "boxBlurV (parallel O(n) vertical blur)" \
  || fail "Missing boxBlurV"
grep -q 'sigmaToBoxes' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "sigmaToBoxes (3-pass Gaussian approximation)" \
  || fail "Missing sigmaToBoxes"
grep -q 'QtConcurrent::blockingMap' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "QtConcurrent multi-threaded blur passes" \
  || fail "Missing QtConcurrent parallelization"
grep -q 'stackBlur(QImage &image, double sigma)' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "stackBlur now takes sigma (double), radius-independent" \
  || fail "stackBlur signature not updated"
grep -q 'm_blurBuf' "$ROOT/src/WallpaperProcessor.h" \
  && ok "Pre-allocated blur buffer (m_blurBuf)" \
  || fail "Missing m_blurBuf"
grep -q 'ensureNoiseTexture' "$ROOT/src/WallpaperProcessor.cpp" \
  && ok "Pre-generated noise texture (ensureNoiseTexture)" \
  || fail "Missing ensureNoiseTexture"
grep -q 'Qt6::Concurrent' "$ROOT/src/CMakeLists.txt" \
  && ok "Qt6::Concurrent linked" \
  || fail "Missing Qt6::Concurrent in CMake"
grep -q 'march=native' "$ROOT/src/CMakeLists.txt" \
  && ok "-march=native SIMD auto-vectorization enabled" \
  || fail "Missing -march=native"

echo
echo "Result: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ] && echo "All checks OK" || exit 1
