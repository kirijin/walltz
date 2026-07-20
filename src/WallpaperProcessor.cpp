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
#include <QUuid>
#include <vector>
#include <KLocalizedString>

static const int SHADOW_RADIUS = 3;

// ── gradient presets (color-theory-based) ─────────────────────────────────

const WallpaperProcessor::GradientPreset WallpaperProcessor::s_presets[10] = {
    // name          color1        color2          harmony
    { "Sunset Warmth", 0xffff6b6b, 0xfffeca57 }, // analogous warm
    { "Ocean Depths",  0xff0abde3, 0xff48dbfb }, // analogous cool
    { "Midnight",      0xff1a1a2e, 0xff16213e }, // monochrome dark
    { "Forest Calm",   0xff6ab04c, 0xff22a6b3 }, // analogous green-teal
    { "Rose Blush",    0xffbe2edd, 0xfff368e0 }, // analogous purple
    { "Amber Glow",    0xfff0932b, 0xfffeca57 }, // analogous warm
    { "Nordic Blues",  0xff2c3e50, 0xff3498db }, // split-complementary
    { "Lavender Sky",  0xff4834d4, 0xff9b59b6 }, // analogous violet
    { "Teal Mint",     0xff00b894, 0xff00cec9 }, // monochrome teal
    { "Grayscale",     0xff444444, 0xffcccccc }, // monochrome value
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
    }
}

void WallpaperProcessor::setTargetHeight(int h)
{
    if (m_targetHeight != h && h > 0 && h < 15000) {
        m_targetHeight = h;
        Q_EMIT targetHeightChanged();
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

void WallpaperProcessor::setBgGradientPreset(int p)
{
    p = qBound(0, p, 9);
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

// ── gradient preset accessors ─────────────────────────────────────────

int WallpaperProcessor::gradientPresetCount() const { return 10; }

QString WallpaperProcessor::gradientPresetName(int index) const
{
    if (index < 0 || index >= 10) return {};
    return i18n(s_presets[index].name);
}

QString WallpaperProcessor::gradientPresetColor1(int index) const
{
    if (index < 0 || index >= 10) return {};
    return QColor(s_presets[index].color1).name();
}

QString WallpaperProcessor::gradientPresetColor2(int index) const
{
    if (index < 0 || index >= 10) return {};
    return QColor(s_presets[index].color2).name();
}

// ── window binding ──────────────────────────────────────────────────────

void WallpaperProcessor::setWindow(QWindow *window)
{
    m_window = window;
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
// Three converging paths:
//   1. C++ primaryScreen() + QTimer retry (original)
//   2. QWindow::screen() after window is mapped (main.cpp wires it)
//   3. QML Screen.width/height → detectFromQML (for Wayland)
//
// All converge to updateScreenSize() which sets the canonical m_screenWidth/m_screenHeight
// and optionally copies to m_targetWidth/m_targetHeight.

void WallpaperProcessor::detectScreenSize()
{
    static const int MAX_RETRIES = 10;
    static const int RETRY_MS = 200;

    // Try primaryScreen first
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen && screen->size().width() > 0 && screen->size().height() > 0) {
        QSize phys = screen->size() * screen->devicePixelRatio();
        updateScreenSize(phys.width(), phys.height());
        m_detectAttempt = 0;
        return;
    }

    // Try window's screen (more reliable on Wayland after window is mapped)
    if (m_window) {
        QScreen *winScreen = m_window->screen();
        if (winScreen && winScreen->size().width() > 0 && winScreen->size().height() > 0) {
            QSize phys = winScreen->size() * winScreen->devicePixelRatio();
            updateScreenSize(phys.width(), phys.height());
            m_detectAttempt = 0;
            return;
        }
    }

    // Retry with backoff (give compositor time to deliver wl_output events)
    if (++m_detectAttempt <= MAX_RETRIES) {
        int delay = qMin(RETRY_MS * (1 + m_detectAttempt / 3), 1000);
        QTimer::singleShot(delay, this, &WallpaperProcessor::detectScreenSize);
    } else {
        m_detectAttempt = 0;
        // Fallback: keep existing defaults (1920x1080) — user can type manually
    }
}

void WallpaperProcessor::detectFromQML(int qmlW, int qmlH, double dpr)
{
    // Called from QML when Screen.width/height become available
    // QML Screen properties return logical pixels — multiply by DPR for physical
    int physW = qRound(qmlW * dpr);
    int physH = qRound(qmlH * dpr);
    updateScreenSize(physW, physH);
    m_detectAttempt = 0;
}

void WallpaperProcessor::updateScreenSize(int w, int h)
{
    if (w < 1 || h < 1) return;
    if (m_screenWidth == w && m_screenHeight == h) return;

    m_screenWidth = w;
    m_screenHeight = h;
    Q_EMIT screenWidthChanged();
    Q_EMIT screenHeightChanged();

    // Auto-apply to target dimensions too (can override via QML binding)
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
            const auto &preset = s_presets[qBound(0, m_bgGradientPreset, 9)];
            double rad = m_gradientAngle * M_PI / 180.0;
            double dx = W * 0.5 * qAbs(qCos(rad)) + H * 0.5 * qAbs(qSin(rad));
            double dy = W * 0.5 * qAbs(qSin(rad)) + H * 0.5 * qAbs(qCos(rad));
            double cx2 = W / 2.0, cy2 = H / 2.0;
            QLinearGradient grad(cx2 - dx, cy2 - dy, cx2 + dx, cy2 + dy);
            grad.setColorAt(0.0, QColor(preset.color1));
            grad.setColorAt(1.0, QColor(preset.color2));
            p.fillRect(0, 0, W, H, grad);
            break;
        }
        case 2: {
            // Auto gradient from image dominant colors
            auto colors = extractDominantColors(src);
            double rad = m_gradientAngle * M_PI / 180.0;
            double dx = W * 0.5 * qAbs(qCos(rad)) + H * 0.5 * qAbs(qSin(rad));
            double dy = W * 0.5 * qAbs(qSin(rad)) + H * 0.5 * qAbs(qCos(rad));
            double cx2 = W / 2.0, cy2 = H / 2.0;
            QLinearGradient grad(cx2 - dx, cy2 - dy, cx2 + dx, cy2 + dy);
            grad.setColorAt(0.0, colors.first);
            grad.setColorAt(1.0, colors.second);
            p.fillRect(0, 0, W, H, grad);
            break;
        }
        default:
            // Solid color
            QColor bgColor = m_autoColor ? extractAverageColor(src) : m_bgColor;
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

    // Save to temp
    QString tmpDir = QDir::tempPath() + QStringLiteral("/walltz");
    QDir().mkpath(tmpDir);
    QString tmpPath = tmpDir + QStringLiteral("/preview_")
        + QUuid::createUuid().toString(QUuid::Id128)  // 32 hex chars
        + QStringLiteral(".png");
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

// ── dominant color extraction (for auto-gradient) ────────────────────────
//
// Downsamples image to ~64×64, quantizes into 16³ histogram, returns the
// two most populous bins. If too similar (Euclidean < ~80 per channel),
// the second color is shifted to a complementary hue.

QPair<QColor, QColor> WallpaperProcessor::extractDominantColors(const QImage &image)
{
    static const int BINS = 16;
    static const int BIN_SIZE = 256 / BINS;
    static const int TOTAL = BINS * BINS * BINS;

    int w = image.width(), h = image.height();
    int step = qMax(1, qMax(w, h) / 64);
    std::vector<int> hist(TOTAL, 0);

    for (int y = 0; y < h; y += step) {
        const QRgb *row = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < w; x += step) {
            QRgb px = row[x];
            int ri = qRed(px) / BIN_SIZE;
            int gi = qGreen(px) / BIN_SIZE;
            int bi = qBlue(px) / BIN_SIZE;
            hist[ri * BINS * BINS + gi * BINS + bi]++;
        }
    }

    // Find top 2 populated bins
    int best1 = 0, best2 = 0;
    int max1 = 0, max2 = 0;
    for (int i = 0; i < TOTAL; ++i) {
        if (hist[i] > max1) {
            max2 = max1; best2 = best1;
            max1 = hist[i]; best1 = i;
        } else if (hist[i] > max2) {
            max2 = hist[i]; best2 = i;
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

    QColor c1 = binToColor(best1);
    QColor c2 = binToColor(best2);

    // If too similar (Euclidean distance < ~80/channel), rotate c2 to complementary hue
    int dr = c1.red() - c2.red();
    int dg = c1.green() - c2.green();
    int db = c1.blue() - c2.blue();
    if (dr*dr + dg*dg + db*db < 6400) {
        float h = 0, s = 0, l = 0;
        c1.getHslF(&h, &s, &l);
        // Complementary: 180° hue rotation
        c2 = QColor::fromHslF(fmod(h + 0.5f, 1.0f), qMin(s * 1.2f, 1.0f), l);
    }

    return {c1, c2};
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
