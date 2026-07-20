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
#include <KLocalizedString>

static const int SHADOW_RADIUS = 3;

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
        Q_EMIT backgroundColorChanged();
    }
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

    // Center coordinates for the source image overlay
    int cx = qMax(0, (W - imgW) / 2);
    int cy = qMax(0, (H - imgH) / 2);

    // Zoom to fill background (match original: max(W/imgW, H/imgH))
    double zoom = qMax(W / (double)imgW, H / (double)imgH);

    // ── Build output ──
    QImage output(W, H, QImage::Format_ARGB32_Premultiplied);
    output.fill(Qt::white);

    QPainter p;
    p.begin(&output);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_blurMode) {
        // ── BLUR MODE: zoomed + darkened + blurred background ──
        // Zoom to fill the output canvas, then center (not top-left)
        double bgW = srcImage.width() * zoom;
        double bgH = srcImage.height() * zoom;
        double bgX = (W - bgW) / 2.0;
        double bgY = (H - bgH) / 2.0;
        p.save();
        p.translate(bgX, bgY);
        p.scale(zoom, zoom);
        p.drawImage(0, 0, srcImage);
        p.restore();
        // Dark overlay (original: rgba(0,0,0,0.1))
        p.fillRect(0, 0, W, H, QColor(0, 0, 0, 25));
        p.end();

        QImage bg = output.copy();
        // Scaled blur radius: proportional to output height (0.051 ≈ 55@1080p),
        // capped at 120 per backgroundifier's maximumBlurRadius
        int blurRadius = qBound(1, (int)(0.051 * H), 120);
        stackBlur(bg, blurRadius);
        // Saturation boost (backgroundifier uses 1.8x — makes blur pop)
        boostSaturation(bg, 1.8);

        p.begin(&output);
        p.drawImage(0, 0, bg);
    } else {
        // ── COLOR MODE: solid background ──
        QColor bgColor = m_autoColor ? extractAverageColor(srcImage) : m_bgColor;
        p.end();
        output.fill(bgColor);
        p.begin(&output);
    }

    // ── SHADOW (matching original: rounded rect, offset +2, alpha 0.4) ──
    QImage sh(W, H, QImage::Format_ARGB32_Premultiplied);
    sh.fill(Qt::transparent);
    QPainter sp(&sh);
    sp.setRenderHint(QPainter::Antialiasing);
    QPainterPath shPath;
    shPath.addRoundedRect(cx, cy + 2, imgW, imgH, SHADOW_RADIUS, SHADOW_RADIUS);
    sp.fillPath(shPath, QColor(0, 0, 0, 102)); // ~0.4 alpha
    sp.end();
    // Scaled shadow blur: proportional to output height (0.0046 ≈ 5@1080p)
    int shadowBlur = qBound(1, (int)(0.0046 * H), 30);
    stackBlur(sh, shadowBlur);
    p.drawImage(0, 0, sh);

    // ── FOREGROUND IMAGE (rounded clip, matching original) ──
    p.save();
    QPainterPath clipPath;
    clipPath.addRoundedRect(cx, cy, imgW, imgH, SHADOW_RADIUS, SHADOW_RADIUS);
    p.setClipPath(clipPath);
    p.drawImage(cx, cy, srcImage);
    p.restore();
    p.end();

    // Write output
    QFileInfo fi(sourcePath);
    outPath = fi.absolutePath() + QDir::separator() + fi.completeBaseName() + QStringLiteral(".wp.png");
    if (!output.save(outPath, "PNG")) {
        Q_EMIT errorOccurred(i18n("Failed to save: %1", QFileInfo(outPath).fileName()));
        return false;
    }
    return true;
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
