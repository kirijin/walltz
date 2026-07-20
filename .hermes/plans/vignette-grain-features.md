# Post-processing features: Vignette + Grain

Both apply on top of the blurred background (after `boostSaturation` + `stackBlur`).

## 1. Vignette (subtle edge darkening)

**UX:** `Controls.Switch { text: "Vignette" }` toggle + `Controls.Slider` for strength (0-100%).

**Implementation in `renderWallpaper`:**

```
m_vignetteStrength: 0.0 – 1.0 (double, default 0.0)
m_vignetteRadius:   0.3 – 1.0 (controls falloff curve, default 0.5)
```

After blur + saturation, paint a radial-gradient overlay:
```cpp
QRadialGradient vignette(w/2, h/2, qMax(w, h) * 0.7);
vignette.setColorAt(0.0, QColor(0,0,0,0));              // fully transparent center
vignette.setColorAt(0.5 + m_vignetteRadius * 0.3, QColor(0,0,0,0));
vignette.setColorAt(1.0, QColor(0,0,0,(int)(80 * m_vignetteStrength))); // ~80 max alpha
QPainter p(&result);
p.setCompositionMode(QPainter::CompositionMode_Multiply);
p.fillRect(result.rect(), vignette);
```

**Color theory:** Dark edges frame the wallpaper, reduce visual fatigue at periphery, mimic lens vignette. Max alpha 80 keeps it subtle even at full strength.

**Properties/notifications:** `vignetteStrength`, `vignetteChanged`.

## 2. Photo Grain (film noise overlay)

**UX:** `Controls.Switch { text: "Grain" }` toggle + `Controls.Slider` intensity (0-100%).

**Implementation:**

```
m_grainStrength: 0.0 – 1.0 (double, default 0.0)
```

After blur + saturation (and optional vignette), add noise:
```cpp
if (m_grainStrength > 0.001) {
    QImage noise(w, h, QImage::Format_Grayscale8);
    int intensity = (int)(12 * m_grainStrength); // max 12/255 ≈ very subtle
    for (int y = 0; y < h; ++y) {
        unsigned char *line = noise.scanLine(y);
        for (int x = 0; x < w; ++x)
            line[x] = (QRandomGenerator::global()->bounded(intensity * 2 + 1)) - intensity + 128;
    }
    QPainter p(&result);
    p.setCompositionMode(QPainter::CompositionMode_SoftLight);
    p.drawImage(0, 0, noise);
}
```

**Color theory:** Subtle grain adds tactile texture, reduces "digital plastic" feel, mimics analog film stock. At max 12/255 it's barely visible on most content but adds depth to flat areas.

**Performance note:** Noise generation is `O(w×h)` per render. For 4K wallpapers, consider pre-seeding a tileable noise pattern and repeating it, or limiting to preview-only and turning off for export.

**Properties/notifications:** `grainStrength`, `grainChanged`.

## 3. QML layout — where they go

In the Blur section alongside existing sliders:

```
// Vignette
RowLayout {
    Controls.Label { text: i18n("Vignette") }
    Controls.Switch { id: vignetteSwitch; checked: processor.vignetteStrength > 0 }
    Controls.Slider {
        from: 0; to: 1.0; value: processor.vignetteStrength
        enabled: vignetteSwitch.checked
        onMoved: processor.vignetteStrength = value
    }
}

// Grain  
RowLayout {
    Controls.Label { text: i18n("Grain") }
    Controls.Switch { id: grainSwitch; checked: processor.grainStrength > 0 }
    Controls.Slider {
        from: 0; to: 1.0; value: processor.grainStrength
        enabled: grainSwitch.checked
        onMoved: processor.grainStrength = value
    }
}
```

Both sliders hidden when `!processor.blurMode`.

## 4. Order of operations in `renderWallpaper`

```
1. Scale/crop image to target
2. boostSaturation (existing)
3. stackBlur (existing)
4. Vignette overlay (new)
5. Grain overlay (new)
6. Gradient overlay (existing bgGradientStyle)
```

## 5. Implementation steps

1. Add `double m_vignetteStrength`, `double m_grainStrength` to header + Q_PROPERTY
2. Implement painting in `renderWallpaper` after blur
3. Add QML controls in the Blur-tweaks section
4. Wire signal/slot for preview update on change
