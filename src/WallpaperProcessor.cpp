#include "WallpaperProcessor.h"

#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QWindow>
#include <QFileInfo>
#include <QtMath>
#include <QPainterPath>
#include <QDir>
#include <QTimer>
#include <QUrl>
#include <QCryptographicHash>
#include <vector>
#include <KLocalizedString>

static const int SHADOW_RADIUS = 3;

// ── gradient presets (color-theory-based) ─────────────────────────────────

const WallpaperProcessor::GradientPreset WallpaperProcessor::s_presets[25] = {
    // name             color1        color2
    // ── Warm tones ─────────────────────────────────────┐
    { "Sunset Warmth",  0xffff6b6b, 0xfffeca57 }, // │
    { "Amber Glow",     0xfff0932b, 0xfffeca57 }, // │
    { "Coral Reef",     0xffff6b6b, 0xff48dbfb }, // │
    { "Lemonade",       0xfffdcb6e, 0xff00cec9 }, // │
    { "Dusk",           0xff6c5ce7, 0xfffd79a8 }, // │
    // ── Cool tones ─────────────────────────────────────┤
    { "Ocean Depths",   0xff0abde3, 0xff48dbfb }, // │
    { "Tokyo Night",    0xff1a1b26, 0xff7aa2f7 }, // │
    { "Arctic",         0xff2e3440, 0xff88c0d0 }, // │
    { "Nordic Blues",   0xff2c3e50, 0xff3498db }, // │
    { "One Dark",       0xff282c34, 0xff61afef }, // │
    // ── Greens & nature ────────────────────────────────┤
    { "Forest Calm",    0xff6ab04c, 0xff22a6b3 }, // │
    { "Teal Mint",      0xff00b894, 0xff00cec9 }, // │
    { "Everforest",     0xff2b3339, 0xffa7c080 }, // │
    { "Aurora",         0xff00b894, 0xff6c5ce7 }, // │
    { "Monokai",        0xff272822, 0xffa6e22e }, // │
    // ── Purples & pinks ────────────────────────────────┤
    { "Lavender Sky",   0xff4834d4, 0xff9b59b6 }, // │
    { "Rose Blush",     0xffbe2edd, 0xfff368e0 }, // │
    { "Catppuccin",     0xff1e1e2e, 0xffcba6f7 }, // │
    { "Dracula",        0xff282a36, 0xffbd93f9 }, // │
    { "Rose Pine",      0xff191724, 0xffebbcba }, // │
    // ── Neutrals & dark ────────────────────────────────┤
    { "Midnight",       0xff1a1a2e, 0xff16213e }, // │
    { "Gruvbox",        0xff282828, 0xff8f3f1a }, // │
    { "Solarized",      0xff073642, 0xff268bd2 }, // │
    { "Mountain",       0xff636e72, 0xffb2bec3 }, // │
    { "Grayscale",      0xff444444, 0xffcccccc }, // │
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
    p = qBound(0, p, 24);
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

int WallpaperProcessor::gradientPresetCount() const { return 25; }

QString WallpaperProcessor::gradientPresetName(int index) const
{
    if (index < 0 || index >= 25) return {};
    return i18n(s_presets[index].name);
}

QString WallpaperProcessor::gradientPresetColor1(int index) const
{
    if (index < 0 || index >= 25) return {};
    return QColor(s_presets[index].color1).name();
}

QString WallpaperProcessor::gradientPresetColor2(int index) const
{
    if (index < 0 || index >= 25) return {};
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
    double zoom = qMax(W / (double)imgW, H / (double)imgH);

    QImage output(W, H, QImage::Format_ARGB32_Premultiplied);

    QPainter p;
    p.begin(&output);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_blurMode) {
        // ── Blur: fill with zoomed+centered source ──
        output.fill(Qt::white);

        double bgW = src.width() * zoom;
        double bgH = src.height() * zoom;
        double bgX = (W - bgW) / 2.0;
        double bgY = (H - bgH) / 2.0;
        p.save();
        p.translate(bgX, bgY);
        p.scale(zoom, zoom);
        p.drawImage(0, 0, src);
        p.restore();
        p.fillRect(0, 0, W, H, QColor(0, 0, 0, 25));
        p.end();

        QImage bg = output.copy();
        // Use manual blur radius if set, else auto-calc (adaptive 0.051×H)
        int blurRadius = m_blurRadius > 0 ? m_blurRadius : qBound(1, (int)(0.051 * H), 120);
        stackBlur(bg, blurRadius);
        boostSaturation(bg, m_saturationFactor);

        p.begin(&output);
        p.drawImage(0, 0, bg);
    } else {
        // ── Color / Gradient: fill background, then draw centered image ──
        switch (m_bgGradientStyle) {
        case 1: {
            // Gradient preset
            const auto &preset = s_presets[qBound(0, m_bgGradientPreset, 24)];
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
            auto colors = extractHarmonizedColors(src);
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

    // ── Shadow ──
    QImage sh(W, H, QImage::Format_ARGB32_Premultiplied);
    sh.fill(Qt::transparent);
    QPainter sp(&sh);
    sp.setRenderHint(QPainter::Antialiasing);
    QPainterPath shPath;
    shPath.addRoundedRect(cx, cy + 2, imgW, imgH, SHADOW_RADIUS, SHADOW_RADIUS);
    sp.fillPath(shPath, QColor(0, 0, 0, 102));
    sp.end();
    int shadowBlur = qBound(1, (int)(0.0046 * H), 30);
    stackBlur(sh, shadowBlur);
    p.drawImage(0, 0, sh);

    // ── Foreground image with rounded clip ──
    p.save();
    QPainterPath clipPath;
    clipPath.addRoundedRect(cx, cy, imgW, imgH, SHADOW_RADIUS, SHADOW_RADIUS);
    p.setClipPath(clipPath);
    p.drawImage(cx, cy, src);
    p.restore();
    p.end();

    return output;
}

// ── live preview (wallpaperize feature) ─────────────────────────────────

QString WallpaperProcessor::generatePreview(const QString &sourcePath)
{
    QImage srcImage(sourcePath);
    if (srcImage.isNull()) return {};

    // Scale output dimensions to ~400px on the longest edge
    int pw = m_targetWidth, ph = m_targetHeight;
    double ps = qMin(400.0 / qMax(pw, ph), 1.0);
    pw = qMax((int)(pw * ps), 200);
    ph = qMax((int)(ph * ps), 150);

    // Preserve target aspect ratio after min-clamp
    if (pw > 0 && ph > 0) {
        if (pw >= ph) {
            // Landscape/wide: width is authoritative
            ph = qMax(qRound(pw * m_targetHeight / (double)m_targetWidth), 1);
        } else {
            // Portrait: height is authoritative
            pw = qMax(qRound(ph * m_targetWidth / (double)m_targetHeight), 1);
        }
    }

    // Scale source for preview output
    int imgW = srcImage.width(), imgH = srcImage.height();
    if (imgW > pw || imgH > ph) {
        while (imgW > pw || imgH > ph) {
            imgW = imgW * 2 / 5;
            imgH = imgH * 2 / 5;
        }
        srcImage = srcImage.scaled(imgW, imgH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    QImage preview = renderWallpaper(srcImage, pw, ph);

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
// Uses three concepts from HIG / color-theory best practices:
//   Predominant — most common hue (from 16³ histogram)
//   Key         — most saturated pixel (vibrancy / character)
//   Harmonize   — desaturate both heavily for background subtlety, blend hue
//                 toward key by ~30°, then spread lightness for a gentle gradient.
// Falls back to a 30° analogous shift when hues would be imperceptible.

QPair<QColor, QColor> WallpaperProcessor::extractHarmonizedColors(const QImage &image)
{
    static const int BINS = 16;
    static const int BIN_SIZE = 256 / BINS;
    static const int TOTAL = BINS * BINS * BINS;

    int w = image.width(), h = image.height();
    int step = qMax(1, qMax(w, h) / 64);
    std::vector<int> hist(TOTAL, 0);

    // Track the most saturated pixel among those with reasonable lightness
    // (avoid picking a near-black saturated pixel that would pull everything muddy)
    float maxSat = 0.0f;
    int keyR = 128, keyG = 128, keyB = 128;

    for (int y = 0; y < h; y += step) {
        const QRgb *row = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < w; x += step) {
            QRgb px = row[x];
            int ri = qRed(px) / BIN_SIZE;
            int gi = qGreen(px) / BIN_SIZE;
            int bi = qBlue(px) / BIN_SIZE;
            hist[ri * BINS * BINS + gi * BINS + bi]++;

            int r = qRed(px), g = qGreen(px), b = qBlue(px);
            int mn = qMin(qMin(r, g), b);
            int mx = qMax(qMax(r, g), b);
            float sat = (mx == 0) ? 0.0f : (mx - mn) / (float)mx;
            float lgt = (mx + mn) / 510.0f;    // approximate HSL lightness 0..1
            // Prefer vivid but not-too-dark pixels
            float score = sat * qMax(0.0f, lgt - 0.15f) * 1.5f;
            if (score > maxSat) {
                maxSat = score;
                keyR = r; keyG = g; keyB = b;
            }
        }
    }

    // Find most populous bin = predominant
    int best1 = 0, max1 = 0;
    for (int i = 0; i < TOTAL; ++i) {
        if (hist[i] > max1) {
            max1 = hist[i];
            best1 = i;
        }
    }

    auto binToColor = [](int idx) -> QColor {
        int ri = idx / (BINS * BINS);
        int gi = (idx / BINS) % BINS;
        int bi = idx % BINS;
        return QColor(ri * BIN_SIZE + BIN_SIZE / 2,
                      gi * BIN_SIZE + BIN_SIZE / 2,
                      bi * BIN_SIZE + BIN_SIZE / 2);
    };

    QColor predominant = binToColor(best1);
    QColor key(keyR, keyG, keyB);

    // ── Harmonize ──────────────────────────────────────────────────────
    float hP, sP, lP, hK, sK, lK;
    predominant.getHslF(&hP, &sP, &lP);
    key.getHslF(&hK, &sK, &lK);

    // Gradient color A — start: predominant, moderately desaturated, slightly darker
    float sA = sP * 0.55f;                    // keep enough chroma to avoid muddy browns
    float lA = qBound(0.18f, lP * 0.88f, 0.72f);  // allow lighter backgrounds
    QColor colorA = QColor::fromHslF(hP, sA, lA);

    // Gradient color B — end: blend hue toward key, muted but chromatic, lighter
    float hDiff = hK - hP;
    if (hDiff > 0.5f) hDiff -= 1.0f;
    if (hDiff < -0.5f) hDiff += 1.0f;

    float hB = fmod(hP + hDiff * 0.3f + 1.0f, 1.0f);  // 30 % toward key
    float sB = qMax(sP, sK) * 0.45f;                  // keep more chroma
    float lB = qBound(0.30f, (lP + lK) * 0.5f + 0.15f, 0.85f);  // lighter
    QColor colorB = QColor::fromHslF(hB, sB, lB);

    // Failsafe: if hues are imperceptibly close (< 5°), add an
    // analogous shift so the gradient has visible depth.
    float hDist = qAbs(colorA.hslHueF() - colorB.hslHueF());
    if (hDist > 0.5f) hDist = 1.0f - hDist;
    if (hDist < 0.014f) {
        colorB = QColor::fromHslF(fmod(hP + 0.08f + 1.0f, 1.0f), sB, lB);
    }

    return {colorA, colorB};
}

// ── blur engine ──────────────────────────────────────────────────────────

void WallpaperProcessor::stackBlur(QImage &image, int radius)
{
    if (radius < 1) return;
    int kr = qMax(1, (int)(radius * 0.7));
    int passes = 3;
    for (int i = 0; i < passes; ++i) {
        boxBlurPass(image, kr);
    }
}

void WallpaperProcessor::boxBlurPass(QImage &image, int radius)
{
    if (radius < 1) return;
    int w = image.width(), h = image.height();
    int div = 2 * radius + 1;

    QImage temp(w, h, image.format());
    const uchar *src = image.constBits();
    uchar *dst = temp.bits();
    int bpl = image.bytesPerLine();
    int tbpl = temp.bytesPerLine();

    // ── Horizontal pass (image → temp) ──
    for (int y = 0; y < h; ++y) {
        const uchar *row = src + y * bpl;
        uchar *drow = dst + y * tbpl;
        for (int x = 0; x < w; ++x) {
            int b = 0, g = 0, r = 0, a = 0;
            for (int dx = -radius; dx <= radius; ++dx) {
                const uchar *p = row + std::clamp(x + dx, 0, w - 1) * 4;
                b += p[0]; g += p[1]; r += p[2]; a += p[3];
            }
            drow[x * 4 + 0] = b / div;
            drow[x * 4 + 1] = g / div;
            drow[x * 4 + 2] = r / div;
            drow[x * 4 + 3] = a / div;
        }
    }

    // ── Vertical pass (temp → image) ──
    uchar *imgData = image.bits();
    const uchar *tmpData = temp.constBits();

    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            int b = 0, g = 0, r = 0, a = 0;
            for (int dy = -radius; dy <= radius; ++dy) {
                const uchar *p = tmpData + std::clamp(y + dy, 0, h - 1) * tbpl + x * 4;
                b += p[0]; g += p[1]; r += p[2]; a += p[3];
            }
            uchar *p = imgData + y * bpl + x * 4;
            p[0] = b / div; p[1] = g / div; p[2] = r / div; p[3] = a / div;
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
