#include "WallpaperProcessor.h"

#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QWindow>
#include <QFileInfo>
#include <QtMath>
#include <QRandomGenerator>
#include <cmath>
#include <QPainterPath>
#include <QDir>
#include <QTimer>
#include <QUrl>
#include <QCryptographicHash>
#include <vector>
#include <functional>
#include <KLocalizedString>

static const int SHADOW_RADIUS = 3;
static const int FRAME_RADIUS = 2;  // photo frame corner radius (small, paper-like)

// ── gradient presets (color-theory-based) ─────────────────────────────────

const WallpaperProcessor::GradientPreset WallpaperProcessor::s_presets[12] = {
    // name             color1        color2
    // ── Warm tones ──
    { "Sunset Warmth",  0xffff6b6b, 0xfffeca57 },
    { "Coral Reef",     0xffff6b6b, 0xff48dbfb },
    { "Lemonade",       0xfffdcb6e, 0xff00cec9 },
    // ── Cool tones ──
    { "Ocean Depths",   0xff0abde3, 0xff48dbfb },
    { "Tokyo Night",    0xff1a1b26, 0xff7aa2f7 },
    { "Arctic",         0xff2e3440, 0xff88c0d0 },
    // ── Dev themes ──
    { "Catppuccin",     0xff1e1e2e, 0xffcba6f7 },
    { "Gruvbox",        0xff282828, 0xff8f3f1a },
    { "Solarized",      0xff073642, 0xff268bd2 },
    // ── Purple/Pink / Nature / Neutral ──
    { "Dusk",           0xff6c5ce7, 0xfffd79a8 },
    { "Everforest",     0xff2b3339, 0xffa7c080 },
    { "Grayscale",      0xff444444, 0xffcccccc },
};

// ── constructor ──────────────────────────────────────────────────────────

WallpaperProcessor::WallpaperProcessor(QObject *parent)
    : QObject(parent)
{
}

// ── property setters ─────────────────────────────────────────────────────

void WallpaperProcessor::setTargetWidth(int w)
{
    if (m_targetWidth != w && w > 0 && w < 15000) {
        m_targetWidth = w;
        Q_EMIT targetWidthChanged();
        if (m_aspectRatio > 0.0) {
            int newH = qRound(w / m_aspectRatio);
            if (newH != m_targetHeight) {
                m_targetHeight = newH;
                Q_EMIT targetHeightChanged();
            }
        }
    }
}

void WallpaperProcessor::setTargetHeight(int h)
{
    if (m_targetHeight != h && h > 0 && h < 15000) {
        m_targetHeight = h;
        Q_EMIT targetHeightChanged();
        if (m_aspectRatio > 0.0) {
            int newW = qRound(h * m_aspectRatio);
            if (newW != m_targetWidth) {
                m_targetWidth = newW;
                Q_EMIT targetWidthChanged();
            }
        }
    }
}

void WallpaperProcessor::setBlurMode(bool blur)
{
    if (m_blurMode != blur) {
        m_blurMode = blur;
        Q_EMIT blurModeChanged();
    }
}

void WallpaperProcessor::setBackgroundColor(const QColor &c)
{
    if (m_bgColor != c) {
        m_bgColor = c;
        Q_EMIT backgroundColorChanged();
    }
}

void WallpaperProcessor::setAutoColor(bool autoC)
{
    if (m_autoColor != autoC) {
        m_autoColor = autoC;
        Q_EMIT autoColorChanged();
    }
}

// ── new parameter setters ────────────────────────────────────────────

void WallpaperProcessor::setBlurRadius(int r)
{
    r = qBound(0, r, 120);
    if (m_blurRadius != r) {
        m_blurRadius = r;
        Q_EMIT blurRadiusChanged();
    }
}

void WallpaperProcessor::setSaturationFactor(double f)
{
    f = qBound(0.0, f, 3.0);
    if (!qFuzzyCompare(m_saturationFactor, f)) {
        m_saturationFactor = f;
        Q_EMIT saturationFactorChanged();
    }
}

void WallpaperProcessor::setBgGradientStyle(int s)
{
    s = qBound(0, s, 2);
    if (m_bgGradientStyle != s) {
        m_bgGradientStyle = s;
        Q_EMIT bgGradientStyleChanged();
    }
}

void WallpaperProcessor::setAspectMode(int mode)
{
    mode = qBound(0, mode, 6);
    if (m_aspectMode == mode) return;
    m_aspectMode = mode;

    if (mode == 0) {
        m_aspectRatio = 0.0;
    } else {
        static const double ratios[] = {0.0, 1.0, 4.0/3.0, 16.0/9.0, 16.0/10.0, 21.0/9.0, 32.0/9.0};
        m_aspectRatio = ratios[mode];
        // Keep the longer dimension, recalc the shorter
        if (m_targetWidth >= m_targetHeight) {
            int newH = qRound(m_targetWidth / m_aspectRatio);
            if (newH != m_targetHeight) {
                m_targetHeight = newH;
                Q_EMIT targetHeightChanged();
            }
        } else {
            int newW = qRound(m_targetHeight * m_aspectRatio);
            if (newW != m_targetWidth) {
                m_targetWidth = newW;
                Q_EMIT targetWidthChanged();
            }
        }
    }
    Q_EMIT aspectModeChanged();
}

void WallpaperProcessor::setBgGradientPreset(int p)
{
    p = qBound(0, p, 11);
    if (m_bgGradientPreset != p) {
        m_bgGradientPreset = p;
        Q_EMIT bgGradientPresetChanged();
    }
}

void WallpaperProcessor::setGradientAngle(double a)
{
    a = std::fmod(a, 360.0);
    if (a < 0) a += 360.0;
    if (!qFuzzyCompare(m_gradientAngle, a)) {
        m_gradientAngle = a;
        Q_EMIT gradientAngleChanged();
    }
}

void WallpaperProcessor::setBgZoom(double z)
{
    z = qBound(0.5, z, 3.0);
    if (!qFuzzyCompare(m_bgZoom, z)) {
        m_bgZoom = z;
        Q_EMIT bgZoomChanged();
    }
}

void WallpaperProcessor::setBgBlurAngle(double a)
{
    a = std::fmod(a, 360.0);
    if (a < 0) a += 360.0;
    if (!qFuzzyCompare(m_bgBlurAngle, a)) {
        m_bgBlurAngle = a;
        Q_EMIT bgBlurAngleChanged();
    }
}

void WallpaperProcessor::setAutoMood(int m)
{
    m = qBound(0, m, 5);
    if (m_autoMood != m) {
        m_autoMood = m;
        Q_EMIT autoMoodChanged();
    }
}

void WallpaperProcessor::setUseV2(bool v2)
{
    if (m_useV2 != v2) {
        m_useV2 = v2;
        Q_EMIT useV2Changed();
    }
}

void WallpaperProcessor::setVignetteStrength(double s)
{
    s = qBound(0.0, s, 1.0);
    if (!qFuzzyCompare(m_vignetteStrength, s)) {
        m_vignetteStrength = s;
        Q_EMIT vignetteStrengthChanged();
    }
}

void WallpaperProcessor::setGrainStrength(double s)
{
    s = qBound(0.0, s, 1.0);
    if (!qFuzzyCompare(m_grainStrength, s)) {
        m_grainStrength = s;
        Q_EMIT grainStrengthChanged();
    }
}

void WallpaperProcessor::setCaStrength(double s)
{
    s = qBound(0.0, s, 1.0);
    if (!qFuzzyCompare(m_caStrength, s)) {
        m_caStrength = s;
        Q_EMIT caStrengthChanged();
    }
}

void WallpaperProcessor::setPhotoFrame(bool on)
{
    if (m_photoFrame != on) {
        m_photoFrame = on;
        Q_EMIT photoFrameChanged();
    }
}

void WallpaperProcessor::setPhotoFrameWidth(int w)
{
    w = qBound(0, w, 25);
    if (m_photoFrameWidth != w) {
        m_photoFrameWidth = w;
        Q_EMIT photoFrameWidthChanged();
    }
}

int WallpaperProcessor::gradientPresetCount() const { return 12; }

QString WallpaperProcessor::gradientPresetName(int index) const
{
    if (index < 0 || index >= 12) return {};
    return i18n(s_presets[index].name);
}

QString WallpaperProcessor::gradientPresetColor1(int index) const
{
    if (index < 0 || index >= 12) return {};
    return QColor(s_presets[index].color1).name();
}

QString WallpaperProcessor::gradientPresetColor2(int index) const
{
    if (index < 0 || index >= 12) return {};
    return QColor(s_presets[index].color2).name();
}

double WallpaperProcessor::aspectRatioForMode(int mode) const
{
    static const double ratios[] = {0.0, 1.0, 4.0/3.0, 16.0/9.0, 16.0/10.0, 21.0/9.0, 32.0/9.0};
    if (mode < 0 || mode > 6) return 0.0;
    return ratios[mode];
}

// ── window binding ──────────────────────────────────────────────────────

void WallpaperProcessor::setWindow(QWindow *window)
{
    m_window = window;
    if (m_window) {
        // Re-detect on screen change (moving to a different monitor)
        connect(m_window, &QWindow::screenChanged,
                this, &WallpaperProcessor::detectFromWindow);

        double dpr = m_window->devicePixelRatio();
        if (!qFuzzyCompare(m_windowDpr, dpr)) {
            m_windowDpr = dpr;
            Q_EMIT windowDprChanged();
        }

        // Initial detection — on Wayland the window's screen may not be
        // ready yet; detectFromWindow() retries with a timer.
        detectFromWindow();

        // Poll for fractional DPR arrival (Qt emits no signal for this
        // on Qt < 6.8).  Poll every second indefinitely so the correct
        // scale is always picked up, even when it arrives late.
        QTimer::singleShot(1000, this, &WallpaperProcessor::pollDpr);
    }
}


void WallpaperProcessor::pollDpr()
{
    if (!m_window) return;
    double dpr = m_window->devicePixelRatio();
    if (!qFuzzyCompare(dpr, m_windowDpr)) {
        m_windowDpr = dpr;
        Q_EMIT windowDprChanged();
        detectFromWindow();
    }
    // Keep polling while the app lives; no bounded limit — fractional DPR
    // on Wayland can arrive unpredictably late (seconds not ms).
    QTimer::singleShot(1000, this, &WallpaperProcessor::pollDpr);
}

void WallpaperProcessor::setKeepAbove(bool keep)
{
    if (m_keepAbove == keep) return;
    m_keepAbove = keep;
    Q_EMIT keepAboveChanged();

    if (m_window) {
        Qt::WindowFlags cur = m_window->flags();
        if (keep) {
            m_window->setFlags(cur | Qt::WindowStaysOnTopHint);
        } else {
            m_window->setFlags(cur & ~Qt::WindowStaysOnTopHint);
        }
        m_window->show(); // re-assert flags on Wayland
    }
}

// ── screen detection ────────────────────────────────────────────────────
//
// Unified via detectFromWindow() — uses m_window→screen() × m_window→devicePixelRatio()
// for correct fractional scale on Wayland.  Called initially from setWindow() and
// every time devicePixelRatio changes (the compositor delivers wp_fractional_scale).
// detectScreenSize() is the public entry for the Detect button / startup.

void WallpaperProcessor::detectFromWindow()
{
    if (!m_window) return;

    // Try the window's specific screen first
    QScreen *screen = m_window->screen();
    if (screen && screen->size().width() > 0 && screen->size().height() > 0) {
        QSize dips = screen->size();
        qreal dpr = m_window->devicePixelRatio();
        updateScreenSize(qRound(dips.width() * dpr), qRound(dips.height() * dpr));
        m_detectAttempt = 0;
        return;
    }

    // Screen not yet ready (Wayland compositor hasn't sent configure yet)
    static const int MAX_RETRIES = 6;
    static const int RETRY_MS = 200;
    if (++m_detectAttempt <= MAX_RETRIES) {
        QTimer::singleShot(RETRY_MS, this, &WallpaperProcessor::detectFromWindow);
    } else {
        m_detectAttempt = 0;
        // Fallback: keep previous value (user can type manually or click Detect)
    }
}

void WallpaperProcessor::detectScreenSize()
{
    // Try window path first (includes fractional DPR on Wayland)
    if (m_window) {
        detectFromWindow();
        return;
    }

    // No window yet — fallback via primaryScreen (dips only, no DPR multiply)
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen && screen->size().width() > 0 && screen->size().height() > 0) {
        updateScreenSize(screen->size().width(), screen->size().height());
        m_detectAttempt = 0;
        return;
    }

    // Retry with timer (rare — only if both screen and window are unavailable)
    static const int MAX_RETRIES = 6;
    static const int RETRY_MS = 200;
    if (++m_detectAttempt <= MAX_RETRIES) {
        QTimer::singleShot(RETRY_MS, this, &WallpaperProcessor::detectScreenSize);
    } else {
        m_detectAttempt = 0;
    }
}

void WallpaperProcessor::updateScreenSize(int w, int h)
{
    if (w < 1 || h < 1) return;

    if (m_screenWidth != w || m_screenHeight != h) {
        if (w != m_screenWidth) {
            m_screenWidth = w;
            Q_EMIT screenWidthChanged();
        }
        if (h != m_screenHeight) {
            m_screenHeight = h;
            Q_EMIT screenHeightChanged();
        }
    }

    // Always reset target to screen values (no early return on unchanged
    // screen dimensions — the caller might want a forced reset even when
    // the screen dims are already known, e.g. Detect button after a ratio
    // preset changed the target away from native)
    if (m_targetWidth != w || m_targetHeight != h) {
        m_targetWidth = w;
        m_targetHeight = h;
        Q_EMIT targetWidthChanged();
        Q_EMIT targetHeightChanged();
    }
}

// ── async queue processing ───────────────────────────────────────────────

void WallpaperProcessor::processImage(const QString &sourcePath)
{
    processQueue({sourcePath});
}

void WallpaperProcessor::processQueue(const QStringList &paths)
{
    if (paths.isEmpty() || m_busy) return;

    m_queue = paths;
    m_currentIndex = 0;
    m_queueProgress = 0;
    m_cancelRequested = false;

    m_busy = true;
    Q_EMIT busyChanged();
    Q_EMIT processingStarted();
    Q_EMIT queueChanged();
    Q_EMIT queueProgressChanged();

    // Yield to event loop before processing so the UI repaints with busy=true.
    // 50ms is enough for one frame at 60fps.
    QTimer::singleShot(50, this, &WallpaperProcessor::processNext);
}

void WallpaperProcessor::cancelProcessing()
{
    m_cancelRequested = true;
}

void WallpaperProcessor::processNext()
{
    if (m_cancelRequested || m_currentIndex >= m_queue.size()) {
        // Done
        m_busy = false;
        Q_EMIT busyChanged();
        Q_EMIT processingFinished();
        m_queueProgress = 0;
        Q_EMIT queueProgressChanged();
        return;
    }

    m_statusMessage = i18n("Processing %1 of %2: %3",
                           m_currentIndex + 1, m_queue.size(),
                           QFileInfo(m_queue[m_currentIndex]).fileName());
    Q_EMIT statusMessageChanged();

    QString outPath;
    bool ok = processSingleImage(m_queue[m_currentIndex], outPath);
    m_queueProgress = m_currentIndex + 1;
    Q_EMIT queueProgressChanged();

    if (ok) {
        m_outputPath = outPath;
        Q_EMIT outputPathChanged();
        m_statusMessage = i18n("[%1/%2] Saved: %3",
                               m_currentIndex + 1, m_queue.size(),
                               QFileInfo(outPath).fileName());
        Q_EMIT statusMessageChanged();
    }

    m_currentIndex++;

    // Schedule next image on the event loop (so UI repaints between images)
    QTimer::singleShot(10, this, &WallpaperProcessor::processNext);
}

// ── core image processing ────────────────────────────────────────────────

bool WallpaperProcessor::processSingleImage(const QString &sourcePath, QString &outPath)
{
    QImage srcImage(sourcePath);
    if (srcImage.isNull()) {
        Q_EMIT errorOccurred(i18n("Cannot load: %1", QFileInfo(sourcePath).fileName()));
        return false;
    }

    int W = m_targetWidth;
    int H = m_targetHeight;

    // Scale source down if oversized (matching original 2/5 iteration)
    int imgW = srcImage.width();
    int imgH = srcImage.height();
    if (imgW > W || imgH > H) {
        while (imgW > W || imgH > H) {
            imgW = imgW * 2 / 5;
            imgH = imgH * 2 / 5;
        }
        srcImage = srcImage.scaled(imgW, imgH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    QImage output = renderWallpaper(srcImage, W, H);

    // Write output
    QFileInfo fi(sourcePath);
    outPath = fi.absolutePath() + QDir::separator() + fi.completeBaseName() + QStringLiteral(".wp.png");
    if (!output.save(outPath, "PNG")) {
        Q_EMIT errorOccurred(i18n("Failed to save: %1", QFileInfo(outPath).fileName()));
        return false;
    }
    return true;
}

// ── render pipeline (shared between full output and preview) ────────────

QImage WallpaperProcessor::renderWallpaper(const QImage &src, int W, int H)
{
    int imgW = src.width(), imgH = src.height();
    int cx = qMax(0, (W - imgW) / 2);
    int cy = qMax(0, (H - imgH) / 2);
    double fillZoom = qMax(W / (double)imgW, H / (double)imgH);

    QImage output(W, H, QImage::Format_ARGB32_Premultiplied);

    QPainter p;
    p.begin(&output);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_blurMode) {
        // ── Blur: fill with zoomed+centered source ──
        double zoom = fillZoom * m_bgZoom;

        // Fill gaps with harmonized background color when zoomed out
        if (m_bgZoom < 1.0) {
            auto hc = extractHarmonizedColors(src);
            output.fill(hc.first);
        } else {
            output.fill(Qt::white);
        }

        double bgW = src.width() * zoom;
        double bgH = src.height() * zoom;
        p.save();
        p.translate(W / 2.0, H / 2.0);
        if (m_bgBlurAngle != 0.0)
            p.rotate(m_bgBlurAngle);
        p.translate(-bgW / 2.0, -bgH / 2.0);
        p.scale(zoom, zoom);
        p.drawImage(0, 0, src);
        p.restore();
        p.fillRect(0, 0, W, H, QColor(0, 0, 0, 25));
        p.end();

        // Use pre-allocated blur buffer instead of output.copy() + drawing back
        if (m_blurBuf.size() != output.size() || m_blurBuf.format() != output.format())
            m_blurBuf = QImage(output.size(), output.format());
        memcpy(m_blurBuf.bits(), output.constBits(), output.sizeInBytes());
        // Use manual blur radius if set, else auto-calc (adaptive 0.051×H)
        double sigma = m_blurRadius > 0
            ? qMax(1.0, (double)m_blurRadius)       // manual: sigma = slider value (1-120)
            : qMax(0.5, 0.017 * H);                 // auto:  sigma ≈ 18 at 1080p
        stackBlur(m_blurBuf, sigma);
        boostSaturation(m_blurBuf, m_saturationFactor);

        p.begin(&output);
        p.drawImage(0, 0, m_blurBuf);
    } else {
        // ── Color / Gradient: fill background, then draw centered image ──
        switch (m_bgGradientStyle) {
        case 1: {
            // Gradient preset
            const auto &preset = s_presets[qBound(0, m_bgGradientPreset, 11)];
            double rad = m_gradientAngle * M_PI / 180.0;
            double t = W * 0.5 * qAbs(qCos(rad)) + H * 0.5 * qAbs(qSin(rad));
            double dx = t * qCos(rad);
            double dy = t * qSin(rad);
            double cx2 = W / 2.0, cy2 = H / 2.0;
            QLinearGradient grad(cx2 - dx, cy2 - dy, cx2 + dx, cy2 + dy);
            grad.setColorAt(0.0, QColor(preset.color1));
            grad.setColorAt(1.0, QColor(preset.color2));
            p.fillRect(0, 0, W, H, grad);
            break;
        }
        case 2: {
            // Auto gradient from image harmonized colors
            auto colors = extractHarmonizedColors(src, m_autoMood);
            double rad = m_gradientAngle * M_PI / 180.0;
            double t = W * 0.5 * qAbs(qCos(rad)) + H * 0.5 * qAbs(qSin(rad));
            double dx = t * qCos(rad);
            double dy = t * qSin(rad);
            double cx2 = W / 2.0, cy2 = H / 2.0;
            QLinearGradient grad(cx2 - dx, cy2 - dy, cx2 + dx, cy2 + dy);
            grad.setColorAt(0.0, colors.first);
            grad.setColorAt(1.0, colors.second);
            p.fillRect(0, 0, W, H, grad);
            break;
        }
        default:
            // Solid color
            QColor bgColor = m_bgColor;
            if (m_autoColor) {
                auto hc = extractHarmonizedColors(src);
                bgColor = hc.first;
            }
            output.fill(bgColor);
            break;
        }
    }

    // ── Effects (post-processing, applies on all background styles) ──
    if (m_vignetteStrength > 0.001) {
        double radius = std::sqrt((W/2.0)*(W/2.0) + (H/2.0)*(H/2.0));
        double s = m_vignetteStrength;
        QRadialGradient vg(W / 2.0, H / 2.0, radius);
        vg.setColorAt(0.0, QColor(0, 0, 0, 0));
        double fadeStart = 1.0 - 0.4 * s;
        vg.setColorAt(fadeStart, QColor(0, 0, 0, 0));
        int alpha = qMin(255, (int)(200 * s));
        vg.setColorAt(1.0, QColor(0, 0, 0, alpha));
        p.fillRect(0, 0, W, H, vg);
    }
    if (m_grainStrength > 0.001) {
        ensureNoiseTexture(W, H);
        // Tile noise from shared texture, applying intensity offset.
        // Grayscale8: pixel = (random byte shifted by intensity) + offset
        int intensity = qMax(1, (int)(15 * m_grainStrength));
        // Re-randomize with intensity in one pass on a writable copy
        QImage grain = s_noiseTexture.copy(0, 0, W, H);
        for (int y = 0; y < H; ++y) {
            unsigned char *line = grain.scanLine(y);
            for (int x = 0; x < W; ++x) {
                // Map pre-generated 0-255 noise into [-intensity, +intensity] + 128 range
                line[x] = (unsigned char)qBound(0,
                    (line[x] % (intensity * 2 + 1)) - intensity + 128, 255);
            }
        }
        p.save();
        p.setCompositionMode(QPainter::CompositionMode_SoftLight);
        p.drawImage(0, 0, grain);
        p.restore();
    }

    // ── Shadow (expands to include photo frame when enabled) ──
    int shCx = cx, shCy = cy + 2, shW = imgW, shH = imgH;
    if (m_photoFrame) {
        int fw = m_photoFrameWidth;
        shCx = cx - fw; shCy = cy - fw + 2;
        shW = imgW + 2*fw; shH = imgH + 2*fw;
    }
    QImage sh(W, H, QImage::Format_ARGB32_Premultiplied);
    sh.fill(Qt::transparent);
    QPainter sp(&sh);
    sp.setRenderHint(QPainter::Antialiasing);
    QPainterPath shPath;
    shPath.addRoundedRect(shCx, shCy, shW, shH, SHADOW_RADIUS, SHADOW_RADIUS);
    sp.fillPath(shPath, QColor(0, 0, 0, 102));
    sp.end();
    double shadowBlurSigma = qMax(0.5, 0.0046 * H / 3.0);
    stackBlur(sh, shadowBlurSigma);
    p.drawImage(0, 0, sh);

    // ── Photo frame (scale proportionally to output size) ──
    if (m_photoFrame) {
        int fw = qMax(2, (int)(m_photoFrameWidth * std::min(W, H) / 500.0));
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawRoundedRect(cx - fw, cy - fw, imgW + 2*fw, imgH + 2*fw,
                          FRAME_RADIUS, FRAME_RADIUS);
        // Thin outer border on the frame
        p.setPen(QPen(QColor(200, 200, 200), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(cx - fw, cy - fw, imgW + 2*fw, imgH + 2*fw,
                          FRAME_RADIUS, FRAME_RADIUS);
    }

    // ── Foreground image with rounded clip ──
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath clipPath;
    int clipRadius = m_photoFrame ? FRAME_RADIUS : SHADOW_RADIUS;
    clipPath.addRoundedRect(cx, cy, imgW, imgH, clipRadius, clipRadius);
    p.setClipPath(clipPath);
    p.drawImage(cx, cy, src);
    p.restore();
    p.end();

    // ── Chromatic aberration (post-processing on full render) ──
    if (m_caStrength > 0.001) {
        double maxShift = m_caStrength * std::min(W, H) * 0.05;
        double cx = W / 2.0, cy = H / 2.0;
        double maxDist = std::sqrt(cx * cx + cy * cy);

        QImage ca(W, H, QImage::Format_ARGB32_Premultiplied);
        const int bpp = 4;
        const int stride = output.bytesPerLine();

        for (int y = 0; y < H; ++y) {
            uchar *dstLine = ca.bits() + y * stride;
            for (int x = 0; x < W; ++x) {
                double dx = (x - cx) / maxDist;
                double dy = (y - cy) / maxDist;
                double dist = std::sqrt(dx * dx + dy * dy);
                int shift = (int)(dist * maxShift);

                int sx = (int)(dx * shift);
                int sy = (int)(dy * shift);

                int rx = qBound(0, x + sx, W - 1);
                int ry = qBound(0, y + sy, H - 1);
                int bx = qBound(0, x - sx, W - 1);
                int by = qBound(0, y - sy, H - 1);

                const uchar *srcPx = output.constBits() + y * stride + x * bpp;
                const uchar *rPx   = output.constBits() + ry * stride + rx * bpp;
                const uchar *bPx   = output.constBits() + by * stride + bx * bpp;
                uchar *dst = dstLine + x * bpp;

                // B shifts inward, R shifts outward, G stays
                dst[0] = bPx[0];   // B
                dst[1] = srcPx[1]; // G (unchanged)
                dst[2] = rPx[2];   // R
                dst[3] = srcPx[3]; // A (unchanged)
            }
        }
        output = ca;
    }

    return output;
}

// ── live preview (wallpaperize feature) ─────────────────────────────────

QString WallpaperProcessor::generatePreview(const QString &sourcePath)
{
    QImage srcImage(sourcePath);
    if (srcImage.isNull()) return {};

    // Use full target resolution so the preview matches the final output exactly.
    // The QML Image element downscales both the live preview and the post-process
    // .wp.png identically via PreserveAspectFit.
    int W = m_targetWidth;
    int H = m_targetHeight;

    // Scale source down if oversized (same logic as processSingleImage)
    int imgW = srcImage.width(), imgH = srcImage.height();
    if (imgW > W || imgH > H) {
        while (imgW > W || imgH > H) {
            imgW = imgW * 2 / 5;
            imgH = imgH * 2 / 5;
        }
        srcImage = srcImage.scaled(imgW, imgH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    // Pre-compute mood palettes from the source image for QML display
    m_moodsComputed = false;
    computeMoodPalettes(srcImage);

    QImage preview = renderWallpaper(srcImage, W, H);

    // Save to temp (deterministic filename — old preview overwritten on re-gen)
    QString tmpDir = QDir::tempPath() + QStringLiteral("/walltz");
    QDir().mkpath(tmpDir);
    QString tmpName = QStringLiteral("pv_")
        + QString::fromLatin1(QCryptographicHash::hash(sourcePath.toUtf8(), QCryptographicHash::Md5).toHex().left(16))
        + QStringLiteral(".png");
    QString tmpPath = tmpDir + QDir::separator() + tmpName;
    if (!preview.save(tmpPath, "PNG")) return {};

    return QStringLiteral("file://") + tmpPath;
}

// ── simple average color extraction ──────────────────────────────────────

QColor WallpaperProcessor::extractAverageColor(const QImage &image)
{
    qint64 rSum = 0, gSum = 0, bSum = 0;
    int w = image.width(), h = image.height();
    int step = qMax(1, qMax(w, h) / 200);
    int samples = 0;

    for (int y = 0; y < h; y += step) {
        for (int x = 0; x < w; x += step) {
            QRgb px = image.pixel(x, y);
            rSum += (px >> 16) & 0xFF;
            gSum += (px >> 8) & 0xFF;
            bSum += px & 0xFF;
            ++samples;
        }
    }
    if (samples == 0) return QColor(128, 128, 128);

    return QColor(std::clamp(int(rSum / samples), 0, 255),
                  std::clamp(int(gSum / samples), 0, 255),
                  std::clamp(int(bSum / samples), 0, 255));
}

// ── harmonized color extraction (for auto-gradient) ──────────────────────
//
// Uses a saturation-weighted hue histogram to extract real chromatic colors
// from the image. White/black/gray pixels (saturation < 0.15) are excluded
// so they don't pollute the palette with their meaningless hue.
// Supports 6 mood variants (0=Auto, 1=Soft, 2=Vivid, 3=Warm, 4=Cool, 5=Deep).
// Heavy histogram computation runs once, results cached in m_moodColorsA/B[].
//
// Falls back to a 30° analogous shift if only one hue family is found.

void WallpaperProcessor::computeMoodPalettes(const QImage &image)
{
    static const int HUE_BINS = 24;       // 15° each
    static const float SAT_THRESHOLD = 0.15f;
    static const float LIGHT_MIN = 0.15f;
    static const float LIGHT_MAX = 0.90f;

    int w = image.width(), h = image.height();
    int step = qMax(1, qMax(w, h) / 64);

    // Hue histogram weighted by saturation
    double hueWeight[HUE_BINS] = {0};
    double hueSat[HUE_BINS] = {0};    // avg saturation per bin
    double hueLight[HUE_BINS] = {0};  // avg lightness per bin
    int hueCount[HUE_BINS] = {0};

    // Track the single most-saturated chromatic pixel as the "key"
    float maxSat = 0.0f;
    int keyR = 128, keyG = 128, keyB = 128;

    for (int y = 0; y < h; y += step) {
        const QRgb *row = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < w; x += step) {
            QRgb px = row[x];
            int r = qRed(px), g = qGreen(px), b = qBlue(px);
            int mn = qMin(qMin(r, g), b);
            int mx = qMax(qMax(r, g), b);
            float sat = (mx == 0) ? 0.0f : (mx - mn) / (float)mx;
            float lgt = (mx + mn) / 510.0f;

            // Skip desaturated or near-black/near-white pixels
            if (sat < SAT_THRESHOLD || lgt < LIGHT_MIN || lgt > LIGHT_MAX)
                continue;

            // Hue in 0..1 from RGB
            float hue = 0.0f;
            float delta = mx - mn;
            if (delta > 0) {
                if (mx == r)
                    hue = (g - b) / delta;
                else if (mx == g)
                    hue = 2.0f + (b - r) / delta;
                else
                    hue = 4.0f + (r - g) / delta;
                hue /= 6.0f;
                if (hue < 0) hue += 1.0f;
            }

            int bin = qMin(int(hue * HUE_BINS), HUE_BINS - 1);
            double wgt = qMax(sat * 100.0, 1.0);
            hueWeight[bin] += wgt;
            hueSat[bin] += sat * wgt;
            hueLight[bin] += lgt * wgt;
            hueCount[bin]++;

            // Key color
            float score = sat * qMax(0.0f, lgt - 0.15f) * 1.5f;
            if (score > maxSat) {
                maxSat = score;
                keyR = r; keyG = g; keyB = b;
            }
        }
    }

    // --- Find top 2 hue bins (by weight) ---
    int best1 = 0, best2 = 0;
    double bestW1 = 0, bestW2 = 0;
    for (int i = 0; i < HUE_BINS; ++i) {
        if (hueWeight[i] > bestW1) {
            bestW2 = bestW1; best2 = best1;
            bestW1 = hueWeight[i]; best1 = i;
        } else if (hueWeight[i] > bestW2) {
            bestW2 = hueWeight[i]; best2 = i;
        }
    }

    QColor key(keyR, keyG, keyB);
    float hK, sK, lK;
    key.getHslF(&hK, &sK, &lK);

    // Helper: representative color from a hue bin with clamping
    auto binColor = [&](int bin, float defaultSat, float defaultLight) -> QColor {
        float h = (bin + 0.5f) / HUE_BINS;
        float s = (hueCount[bin] > 0) ? hueSat[bin] / hueWeight[bin] : defaultSat;
        float l = (hueCount[bin] > 0) ? hueLight[bin] / hueWeight[bin] : defaultLight;
        s = qBound(0.35f, s, 0.75f);
        l = qBound(0.35f, l, 0.70f);
        return QColor::fromHslF(h, s, l);
    };

    // ── Compute all 6 mood palettes ──────────────────────────────────────

    // Mood 0: Auto (top-2 weighted bins, current algorithm)
    if (bestW1 < 1.0) {
        for (int m = 0; m < 6; ++m) {
            m_moodColorsA[m] = QColor(128, 128, 128);
            m_moodColorsB[m] = QColor(180, 180, 180);
        }
    } else {
        QColor autoA = binColor(best1, 0.50f, 0.50f);
        float hueA = (best1 + 0.5f) / HUE_BINS;
        float hueB;
        if (bestW2 < 1.0) {
            hueB = fmod(hueA + 1.0f / 12.0f, 1.0f);
        } else {
            hueB = (best2 + 0.5f) / HUE_BINS;
            float hD = hK - hueB;
            if (hD > 0.5f) hD -= 1.0f;
            if (hD < -0.5f) hD += 1.0f;
            hueB = fmod(hueB + hD * 0.2f + 1.0f, 1.0f);
        }
        float sB = qBound(0.30f, (autoA.hslSaturationF() + sK) * 0.45f, 0.65f);
        float lB = qBound(0.40f, autoA.lightnessF() + 0.15f, 0.78f);
        QColor autoB = QColor::fromHslF(hueB, sB, lB);
        float spread = qAbs(autoB.hslHueF() - autoA.hslHueF());
        if (spread > 0.5f) spread = 1.0f - spread;
        if (spread < 0.014f)
            autoB = QColor::fromHslF(fmod(hueA + 1.0f / 12.0f, 1.0f), sB, lB);
        m_moodColorsA[0] = autoA;
        m_moodColorsB[0] = autoB;

        // ── Helper: pick robust bin with character-preserving fallback ──
        // If no bin matches the predicate, return sentinel so caller
        // transforms best1 to fit the mood's character.
        auto pickMoodBin = [&](const std::function<bool(int)> &pred) -> int {
            for (int i = 0; i < HUE_BINS; ++i)
                if (hueWeight[i] > 0 && pred(i)) return i;
            return -1;
        };

        // ── Mood 1: Soft — mid-sat, mid-light ──
        {
            int sb = pickMoodBin([&](int i){
                float s = hueSat[i]/hueWeight[i];
                float l = hueLight[i]/hueWeight[i];
                return s >= 0.25f && s <= 0.60f && l >= 0.35f && l <= 0.70f;
            });
            float h = (sb >= 0)
                ? ((sb + 0.5f) / HUE_BINS)
                : ((best1 + 0.5f) / HUE_BINS);
            float sV = qBound(0.28f,
                (sb >= 0) ? hueSat[sb]/hueWeight[sb] : 0.40f,
                0.48f);
            float lV = qBound(0.38f,
                (sb >= 0) ? hueLight[sb]/hueWeight[sb] : 0.52f,
                0.62f);
            m_moodColorsA[1] = QColor::fromHslF(h, sV, lV);
            m_moodColorsB[1] = QColor::fromHslF(
                fmod(h + 1.0f/12.0f, 1.0f),
                qBound(0.22f, sV*0.85f, 0.40f),
                qBound(0.48f, lV+0.10f, 0.72f));
        }

        // ── Mood 2: Vivid — highest sat×light ──
        {
            int vb = best1;
            double vs = -1;
            for (int i = 0; i < HUE_BINS; ++i) {
                if (hueWeight[i] <= 0) continue;
                double sc = (hueSat[i]/hueWeight[i]) * (hueLight[i]/hueWeight[i]);
                if (sc > vs) { vs = sc; vb = i; }
            }
            float hV = (vb + 0.5f) / HUE_BINS;
            float sV = qBound(0.55f,
                (hueCount[vb] > 0) ? hueSat[vb]/hueWeight[vb] : 0.65f,
                0.85f);
            float lV = qBound(0.40f,
                (hueCount[vb] > 0) ? hueLight[vb]/hueWeight[vb] : 0.55f,
                0.68f);
            m_moodColorsA[2] = QColor::fromHslF(hV, sV, lV);
            float h2 = (best2 != vb && bestW2 > 1.0)
                ? ((best2 + 0.5f) / HUE_BINS)
                : fmod(hV + 1.0f/8.0f, 1.0f);
            m_moodColorsB[2] = QColor::fromHslF(h2,
                qBound(0.45f, sV*0.80f, 0.70f),
                qBound(0.45f, lV+0.10f, 0.72f));
        }

        // ── Mood 3: Warm — hues 0-60° (bins 0-3) ──
        {
            int wb = -1;
            for (int i = 0; i <= 3; ++i)
                if (hueWeight[i] > 0) { wb = i; break; }
            float hW = (wb >= 0)
                ? ((wb + 0.5f) / HUE_BINS)
                : 0.10f; // ~36° — safe warm orange if no warm bin exists
            float sW = qBound(0.40f,
                (wb >= 0 && hueCount[wb] > 0) ? hueSat[wb]/hueWeight[wb] : 0.55f,
                0.72f);
            float lW = qBound(0.38f,
                (wb >= 0 && hueCount[wb] > 0) ? hueLight[wb]/hueWeight[wb] : 0.50f,
                0.65f);
            m_moodColorsA[3] = QColor::fromHslF(hW, sW, lW);
            m_moodColorsB[3] = QColor::fromHslF(
                fmod(hW + 1.0f/10.0f, 1.0f),
                qBound(0.35f, sW*0.85f, 0.60f),
                qBound(0.42f, lW+0.12f, 0.72f));
        }

        // ── Mood 4: Cool — hues 180-270° (bins 12-17) ──
        {
            int cb = -1;
            for (int i = 12; i <= 17; ++i)
                if (hueWeight[i] > 0) { cb = i; break; }
            float hC = (cb >= 0)
                ? ((cb + 0.5f) / HUE_BINS)
                : 0.60f; // ~216° — safe cool blue
            float sC = qBound(0.40f,
                (cb >= 0 && hueCount[cb] > 0) ? hueSat[cb]/hueWeight[cb] : 0.50f,
                0.72f);
            float lC = qBound(0.38f,
                (cb >= 0 && hueCount[cb] > 0) ? hueLight[cb]/hueWeight[cb] : 0.50f,
                0.65f);
            m_moodColorsA[4] = QColor::fromHslF(hC, sC, lC);
            m_moodColorsB[4] = QColor::fromHslF(
                fmod(hC - 1.0f/10.0f + 1.0f, 1.0f),
                qBound(0.35f, sC*0.85f, 0.60f),
                qBound(0.42f, lC+0.12f, 0.72f));
        }

        // ── Mood 5: Deep — lowest-lightness bin ──
        {
            int db = best1;
            double ml = 1.0;
            for (int i = 0; i < HUE_BINS; ++i) {
                if (hueWeight[i] <= 0) continue;
                float l = hueLight[i] / hueWeight[i];
                if (l < ml) { ml = l; db = i; }
            }
            float hD = (db + 0.5f) / HUE_BINS;
            float sD = qBound(0.35f,
                (hueCount[db] > 0) ? hueSat[db]/hueWeight[db] : 0.40f,
                0.60f);
            float lD = qBound(0.20f,
                (hueCount[db] > 0) ? hueLight[db]/hueWeight[db] : 0.35f,
                0.40f);
            m_moodColorsA[5] = QColor::fromHslF(hD, sD, lD);
            m_moodColorsB[5] = QColor::fromHslF(
                fmod(hD + 1.0f/12.0f, 1.0f),
                qBound(0.30f, sD*0.90f, 0.50f),
                qBound(0.30f, lD+0.12f, 0.50f));
        }
    }

    m_moodsComputed = true;

    // Also compute V2 for the second row of suggestions
    computeMoodPalettesV2(image);
}



QColor WP_colorFromCentroid(double r, double g, double b)
{
    return QColor(qBound(0, (int)qRound(r), 255),
                  qBound(0, (int)qRound(g), 255),
                  qBound(0, (int)qRound(b), 255));
}

// ── V2: 3D RGB histogram (future use, not wired up) ────────────────────────
//
// 8×8×8 RGB cube. Bins scored by population × chroma² × (1 − |0.5 − lightness|).
// Top-3 well-separated centroids feed all 6 moods.  Called from
// computeMoodPalettes() and stored in m_moodColorsV2A/B for the second row.
//
void WallpaperProcessor::computeMoodPalettesV2(const QImage &image)
{
    static const int RGB_BINS = 8;
    static const float SAT_THRESHOLD = 0.12f;
    static const float LIGHT_MIN = 0.10f;
    static const float LIGHT_MAX = 0.92f;

    int w = image.width(), h = image.height();
    int step = qMax(1, qMax(w, h) / 64);

    struct Bin { double weight = 0; double rSum = 0, gSum = 0, bSum = 0; int count = 0; };
    Bin bins[RGB_BINS][RGB_BINS][RGB_BINS];

    for (int y = 0; y < h; y += step) {
        const QRgb *row = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < w; x += step) {
            QRgb px = row[x];
            int r = qRed(px), g = qGreen(px), b = qBlue(px);
            int mn = qMin(qMin(r, g), b);
            int mx = qMax(qMax(r, g), b);
            float sat = (mx == 0) ? 0.0f : (mx - mn) / (float)mx;
            float lgt = (mx + mn) / 510.0f;
            if (sat < SAT_THRESHOLD || lgt < LIGHT_MIN || lgt > LIGHT_MAX) continue;

            int ri = qMin(r * RGB_BINS / 256, RGB_BINS - 1);
            int gi = qMin(g * RGB_BINS / 256, RGB_BINS - 1);
            int bi = qMin(b * RGB_BINS / 256, RGB_BINS - 1);
            double wgt = sat * sat * (1.0 - qAbs(0.5 - lgt));
            bins[ri][gi][bi].weight += wgt;
            bins[ri][gi][bi].rSum += r * wgt;
            bins[ri][gi][bi].gSum += g * wgt;
            bins[ri][gi][bi].bSum += b * wgt;
            bins[ri][gi][bi].count++;
        }
    }

    std::vector<Centroid3D> centroids;
    for (int ri = 0; ri < RGB_BINS; ++ri)
        for (int gi = 0; gi < RGB_BINS; ++gi)
            for (int bi = 0; bi < RGB_BINS; ++bi) {
                const auto &bin = bins[ri][gi][bi];
                if (bin.weight < 0.5) continue;
                double rAvg = bin.rSum / bin.weight;
                double gAvg = bin.gSum / bin.weight;
                double bAvg = bin.bSum / bin.weight;
                double mnC = qMin(qMin(rAvg, gAvg), bAvg);
                double mxC = qMax(qMax(rAvg, gAvg), bAvg);
                double chroma = (mxC == 0) ? 0.0 : (mxC - mnC) / mxC;
                centroids.push_back({rAvg, gAvg, bAvg, bin.weight * chroma * chroma, ri, gi, bi, bin.count});
            }

    if (centroids.empty()) {
        for (int m = 0; m < 6; ++m) { m_moodColorsV2A[m] = QColor(128,128,128); m_moodColorsV2B[m] = QColor(180,180,180); }
        m_moodsComputed = true;
        return;
    }

    std::sort(centroids.begin(), centroids.end(),
        [](const Centroid3D &a, const Centroid3D &b) { return a.score > b.score; });

    std::vector<Centroid3D *> picks;
    picks.push_back(&centroids[0]);
    for (size_t i = 1; i < centroids.size() && picks.size() < 3; ++i) {
        bool ok = true;
        for (auto *p : picks) {
            if (qAbs(centroids[i].ri - p->ri) < 2 && qAbs(centroids[i].gi - p->gi) < 2 && qAbs(centroids[i].bi - p->bi) < 2) {
                ok = false; break;
            }
        }
        if (ok) picks.push_back(&centroids[i]);
    }

    // Raw centroid colors (seed values)
    QColor colors[3] = {
        WP_colorFromCentroid(picks[0]->r, picks[0]->g, picks[0]->b),
        picks.size() > 1 ? WP_colorFromCentroid(picks[1]->r, picks[1]->g, picks[1]->b) : WP_colorFromCentroid(picks[0]->r, picks[0]->g, picks[0]->b),
        picks.size() > 2 ? WP_colorFromCentroid(picks[2]->r, picks[2]->g, picks[2]->b) : WP_colorFromCentroid(picks[0]->r, picks[0]->g, picks[0]->b)
    };

    // ── Artistically soften centroids for wallpaper-friendly gradients ──
    //
    // Color theory rules applied:
    //   • Saturation > 0.55 → desaturate to [0.25, 0.55] (harsh colors softened)
    //   • Lightness < 0.30 → brighten; > 0.75 → darken (extreme tones moderated)
    //   • Hue preserved from image → gradient feels "extracted" not synthetic
    //   • Mood pairs respect analogous/complementary relationships

    QColor softColors[3];
    for (int i = 0; i < 3; ++i) {
        float h, s, l;
        colors[i].getHslF(&h, &s, &l);
        s = qBound(0.25f, s * 0.60f, 0.55f);   // desaturate 40%, cap at 0.55
        l = qBound(0.38f, l, 0.70f);             // keep in gentle mid-range
        softColors[i] = QColor::fromHslF(h, s, l);
    }

    auto pairByMaxContrast = [&]() -> QPair<QColor, QColor> {
        double bestDist = -1; int bestA = 0, bestB = 1;
        for (int i = 0; i < 3; ++i)
            for (int j = i+1; j < 3; ++j) {
                double dr = softColors[i].redF() - softColors[j].redF();
                double dg = softColors[i].greenF() - softColors[j].greenF();
                double db = softColors[i].blueF() - softColors[j].blueF();
                double dist = dr*dr + dg*dg + db*db;
                if (dist > bestDist) { bestDist = dist; bestA = i; bestB = j; }
            }
        return {softColors[bestA], softColors[bestB]};
    };

    // 0 — Dynamic: highest-contrast pair, both softened
    { auto p = pairByMaxContrast();
      m_moodColorsV2A[0] = p.first;
      m_moodColorsV2B[0] = p.second; }

    // 1 — Tonal: same-hue gentle shift (analogous, ±15°), low saturation
    { float h, s, l; softColors[0].getHslF(&h, &s, &l);
      m_moodColorsV2A[1] = QColor::fromHslF(h, qBound(0.20f, s*0.75f, 0.40f), qBound(0.40f, l*0.95f, 0.60f));
      m_moodColorsV2B[1] = QColor::fromHslF(fmod(h+1.0f/24.0f,1.0f), qBound(0.18f, s*0.70f, 0.35f), qBound(0.42f, l*1.05f, 0.65f)); }

    // 2 — Expressive: most saturated soft color + complement at 120°
    { int best = 0; float maxS = 0;
      for (int i=0;i<3;++i) { float h,s,l; softColors[i].getHslF(&h,&s,&l); if (s>maxS) { maxS=s; best=i; } }
      float h,s,l; softColors[best].getHslF(&h,&s,&l);
      m_moodColorsV2A[2] = QColor::fromHslF(h, qBound(0.35f,s*1.15f,0.65f), qBound(0.40f,l,0.65f));
      m_moodColorsV2B[2] = QColor::fromHslF(fmod(h+1.0f/4.0f,1.0f), qBound(0.25f,s*0.70f,0.45f), qBound(0.38f,l+0.05f,0.70f)); }

    // 3 — Ember: warmest centroid → golden, gentle accent
    { int warmIdx=0; float warmestDist=1.0f;
      for (int i=0;i<3;++i) { float h,s,l; softColors[i].getHslF(&h,&s,&l);
        float d=qMin(qAbs(h-0.05f), qAbs(h-0.95f));
        if (d<warmestDist) { warmestDist=d; warmIdx=i; } }
      float h,s,l; softColors[warmIdx].getHslF(&h,&s,&l);
      m_moodColorsV2A[3]=QColor::fromHslF(h, qBound(0.30f,s*0.90f,0.50f), qBound(0.40f,l+0.02f,0.65f));
      m_moodColorsV2B[3]=QColor::fromHslF(fmod(h+1.0f/16.0f,1.0f), qBound(0.25f,s*0.75f,0.40f), qBound(0.45f,l+0.08f,0.72f)); }

    // 4 — Glacier: coolest centroid → silver-blue, restful
    { int coolIdx=0; float coolestD=999;
      for (int i=0;i<3;++i) { float h,s,l; softColors[i].getHslF(&h,&s,&l);
        float d=qAbs(h-0.58f); if (d<coolestD) { coolestD=d; coolIdx=i; } }
      float h,s,l; softColors[coolIdx].getHslF(&h,&s,&l);
      m_moodColorsV2A[4]=QColor::fromHslF(h, qBound(0.22f,s*0.80f,0.42f), qBound(0.42f,l+0.02f,0.68f));
      m_moodColorsV2B[4]=QColor::fromHslF(fmod(h-1.0f/18.0f+1.0f,1.0f), qBound(0.20f,s*0.70f,0.35f), qBound(0.45f,l+0.08f,0.72f)); }

    // 5 — Shadow: darkest centroid, muted, gentle depth
    { int darkIdx=0; double minL=softColors[0].lightnessF();
      for (int i=1;i<3;++i) { if (softColors[i].lightnessF()<minL) { minL=softColors[i].lightnessF(); darkIdx=i; } }
      float h,s,l; softColors[darkIdx].getHslF(&h,&s,&l);
      m_moodColorsV2A[5]=QColor::fromHslF(h, qBound(0.20f,s*0.75f,0.38f), qBound(0.32f,l-0.05f,0.50f));
      m_moodColorsV2B[5]=QColor::fromHslF(fmod(h+0.5f,1.0f), qBound(0.20f,s*0.70f,0.35f), qBound(0.35f,l+0.08f,0.45f)); }

    m_moodsComputed = true;
}


QPair<QColor, QColor> WallpaperProcessor::extractHarmonizedColors(const QImage &image, int mood)
{
    if (!m_moodsComputed)
        computeMoodPalettes(image);
    int m = qBound(0, mood, 5);
    if (m_useV2)
        return {m_moodColorsV2A[m], m_moodColorsV2B[m]};
    return {m_moodColorsA[m], m_moodColorsB[m]};
}

// ── Mood palette accessors (QML) ──────────────────────────────────────────

QString WallpaperProcessor::moodName(int index) const
{
    static const char *names[] = {
        QT_TRANSLATE_NOOP("WallpaperProcessor", "Auto"),
        QT_TRANSLATE_NOOP("WallpaperProcessor", "Soft"),
        QT_TRANSLATE_NOOP("WallpaperProcessor", "Vivid"),
        QT_TRANSLATE_NOOP("WallpaperProcessor", "Warm"),
        QT_TRANSLATE_NOOP("WallpaperProcessor", "Cool"),
        QT_TRANSLATE_NOOP("WallpaperProcessor", "Deep")
    };
    if (index < 0 || index >= 6) return {};
    return i18n(names[index]);
}

QString WallpaperProcessor::moodColorA(int index) const
{
    if (index < 0 || index >= 6 || !m_moodsComputed) return {};
    return m_moodColorsA[index].name();
}

QString WallpaperProcessor::moodColorB(int index) const
{
    if (index < 0 || index >= 6 || !m_moodsComputed) return {};
    return m_moodColorsB[index].name();
}

QString WallpaperProcessor::moodColorV2A(int index) const
{
    if (index < 0 || index >= 6) return {};
    if (!m_moodsComputed) {
        static const char *fallback[] = {"#e57373","#a5d6a7","#90caf9","#ffcc80","#80cbc4","#ce93d8"};
        return QString::fromUtf8(fallback[qBound(0, index, 5)]);
    }
    return m_moodColorsV2A[index].name();
}

QString WallpaperProcessor::moodColorV2B(int index) const
{
    if (index < 0 || index >= 6) return {};
    if (!m_moodsComputed) {
        static const char *fallback[] = {"#ff8a80","#81c784","#64b5f6","#ffb74d","#4db6ac","#ba68c8"};
        return QString::fromUtf8(fallback[qBound(0, index, 5)]);
    }
    return m_moodColorsV2B[index].name();
}

QString WallpaperProcessor::moodNameV2(int index) const
{
    static const char *names[] = {
        "Dynamic",    // 0 — highest-contrast pair, automatically selected
        "Tonal",      // 1 — muted, analogous
        "Expressive", // 2 — most saturated color paired
        "Ember",      // 3 — warmest tones
        "Glacier",     // 4 — coolest tones
        "Shadow"      // 5 — darkest pair
    };
    if (index < 0 || index >= 6) return {};
    return QString::fromUtf8(names[index]);
}

// ── O(n) box blur helpers (sliding window, radius-independent) ──────────
//
// Each pass uses a running sum over a sliding window of (2*radius+1) pixels.
// Only the pixels entering and leaving the window are touched per step,
// making this O(w*h) regardless of radius.
//
// Multi-threaded: horizontal pass farms out rows, vertical pass farms out
// columns, via QtConcurrent::blockingMap (uses all CPU cores).

void WallpaperProcessor::boxBlurH(QImage &dst, const QImage &src, int radius)
{
    int w = src.width(), h = src.height();
    int bpl = src.bytesPerLine();

    // Each row is independent — parallelize over rows
    auto rowOp = [&](int y) {
        const uchar *sRow = src.constBits() + y * bpl;
        uchar *dRow = dst.bits() + y * bpl;

        double sb = 0, sg = 0, sr = 0, sa = 0;

        for (int x = 0; x < w; ++x) {
            int left  = std::max(0, x - radius);
            int right = std::min(w - 1, x + radius);
            int count = right - left + 1;

            if (x == 0) {
                // Full initial sum — each pixel counted once
                for (int sx = 0; sx <= right; ++sx) {
                    const uchar *p = sRow + sx * 4;
                    sb += p[0]; sg += p[1]; sr += p[2]; sa += p[3];
                }
            } else {
                int prevLeft  = std::max(0, x - 1 - radius);
                int prevRight = std::min(w - 1, x - 1 + radius);
                // Remove pixel that left the window (left edge advanced)
                if (left > prevLeft) {
                    const uchar *pOut = sRow + prevLeft * 4;
                    sb -= pOut[0]; sg -= pOut[1]; sr -= pOut[2]; sa -= pOut[3];
                }
                // Add pixel that entered the window (right edge advanced)
                if (right > prevRight) {
                    const uchar *pIn = sRow + right * 4;
                    sb += pIn[0]; sg += pIn[1]; sr += pIn[2]; sa += pIn[3];
                }
            }

            uchar *dp = dRow + x * 4;
            dp[0] = (uchar)(sb / count + 0.5);
            dp[1] = (uchar)(sg / count + 0.5);
            dp[2] = (uchar)(sr / count + 0.5);
            dp[3] = (uchar)(sa / count + 0.5);
        }
    };

    // Parallelize over all rows
    QList<int> rows(h);
    std::iota(rows.begin(), rows.end(), 0);
    QtConcurrent::blockingMap(rows, rowOp);
}

void WallpaperProcessor::boxBlurV(QImage &dst, const QImage &src, int radius)
{
    int w = src.width(), h = src.height();
    int bpl = src.bytesPerLine();

    // Each column is independent — parallelize over columns
    auto colOp = [&](int x) {
        const uchar *sBase = src.constBits();
        uchar *dBase = dst.bits();

        double sb = 0, sg = 0, sr = 0, sa = 0;

        for (int y = 0; y < h; ++y) {
            int top    = std::max(0, y - radius);
            int bottom = std::min(h - 1, y + radius);
            int count  = bottom - top + 1;

            if (y == 0) {
                for (int sy = 0; sy <= bottom; ++sy) {
                    const uchar *p = sBase + sy * bpl + x * 4;
                    sb += p[0]; sg += p[1]; sr += p[2]; sa += p[3];
                }
            } else {
                int prevTop    = std::max(0, y - 1 - radius);
                int prevBottom = std::min(h - 1, y - 1 + radius);
                if (top > prevTop) {
                    const uchar *pOut = sBase + prevTop * bpl + x * 4;
                    sb -= pOut[0]; sg -= pOut[1]; sr -= pOut[2]; sa -= pOut[3];
                }
                if (bottom > prevBottom) {
                    const uchar *pIn = sBase + bottom * bpl + x * 4;
                    sb += pIn[0]; sg += pIn[1]; sr += pIn[2]; sa += pIn[3];
                }
            }

            uchar *dp = dBase + y * bpl + x * 4;
            dp[0] = (uchar)(sb / count + 0.5);
            dp[1] = (uchar)(sg / count + 0.5);
            dp[2] = (uchar)(sr / count + 0.5);
            dp[3] = (uchar)(sa / count + 0.5);
        }
    };

    // Parallelize over all columns
    QList<int> cols(w);
    std::iota(cols.begin(), cols.end(), 0);
    QtConcurrent::blockingMap(cols, colOp);
}

/// Convert sigma to 3 box-blur radii (Ivan Kutskir / Peter Kovesi method)
/// Three box blurs approximate a Gaussian via the Central Limit Theorem.
static void sigmaToBoxes(int boxes[3], double sigma)
{
    double n = 3.0;                     // 3 box passes = good Gaussian approx
    double wi = std::sqrt((12.0 * sigma * sigma / n) + 1.0);
    int wl = (int)std::floor(wi);
    if (wl % 2 == 0) --wl;
    int wu = wl + 2;

    double mi = (12.0 * sigma * sigma - n * wl * wl - 4.0 * n * wl - 3.0 * n)
                / (-4.0 * wl - 4.0);
    int m = (int)std::round(mi);

    for (int i = 0; i < 3; ++i)
        boxes[i] = ((i < m ? wl : wu) - 1) / 2;
}

void WallpaperProcessor::stackBlur(QImage &image, double sigma)
{
    if (sigma < 0.5 || image.isNull()) return;
    if (image.format() != QImage::Format_ARGB32_Premultiplied)
        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    int w = image.width(), h = image.height();
    if (w < 1 || h < 1) return;

    // Three box radii approximating a Gaussian of the requested sigma
    int boxes[3];
    sigmaToBoxes(boxes, sigma);

    // Temp buffer for intermediate results
    QImage tmp(w, h, QImage::Format_ARGB32_Premultiplied);

    // Apply 3 box blur passes (each = H + V).
    // Each pass always: read from image → H → tmp → V → image
    // This avoids the stale-buffer bug: tmp is always overwritten with
    // the fresh H result before V reads it.
    for (int pass = 0; pass < 3; ++pass) {
        int r = boxes[pass];
        if (r < 1) r = 1;
        boxBlurH(tmp, image, r);   // H: image → tmp (parallel)
        boxBlurV(image, tmp, r);   // V: tmp  → image (parallel)
    }
}

// ── pre-generated noise texture ──────────────────────────────────────────

QImage WallpaperProcessor::s_noiseTexture;

void WallpaperProcessor::ensureNoiseTexture(int w, int h)
{
    if (!s_noiseTexture.isNull() && s_noiseTexture.width() >= w && s_noiseTexture.height() >= h)
        return;

    // Allocate a generously-sized noise texture (large enough for any output)
    int tw = qMax(w, 1024);
    int th = qMax(h, 1024);
    s_noiseTexture = QImage(tw, th, QImage::Format_Grayscale8);

    // Fill with random noise in 64-row batches for speed
    for (int y = 0; y < th; y += 64) {
        int endY = qMin(y + 64, th);
        for (int yy = y; yy < endY; ++yy) {
            unsigned char *line = s_noiseTexture.scanLine(yy);
            for (int x = 0; x < tw; ++x) {
                line[x] = (unsigned char)QRandomGenerator::global()->bounded(256);
            }
        }
    }
}

// ── saturation boost (backgroundifier: 1.8x) ───────────────────────────
//
// Quick approximation: scale each channel's distance from gray.
//   gray = (R + G + B) / 3
//   channel = gray + (channel - gray) * factor
//
// This avoids full HSL conversion while producing a nearly identical result.
// Works on ARGB32_Premultiplied (B,G,R,A on little-endian x86_64).

void WallpaperProcessor::boostSaturation(QImage &image, double factor)
{
    if (qFuzzyCompare(factor, 1.0) || image.isNull()) return;
    int w = image.width(), h = image.height();
    int bpl = image.bytesPerLine();
    uchar *data = image.bits();

    for (int y = 0; y < h; ++y) {
        uchar *row = data + y * bpl;
        for (int x = 0; x < w; ++x) {
            uchar *p = row + x * 4;   // B,G,R,A on little-endian x86_64
            int gray = (p[2] + p[1] + p[0]) / 3;   // (R+G+B)/3
            p[0] = (uchar)std::clamp((int)(gray + (p[0] - gray) * factor), 0, 255);
            p[1] = (uchar)std::clamp((int)(gray + (p[1] - gray) * factor), 0, 255);
            p[2] = (uchar)std::clamp((int)(gray + (p[2] - gray) * factor), 0, 255);
        }
    }
}
