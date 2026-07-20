#ifndef WALLPAPERPROCESSOR_H
#define WALLPAPERPROCESSOR_H

#include <QObject>
#include <QImage>
#include <QSize>
#include <QString>
#include <QColor>
#include <QStringList>
#include <QVariantList>
#include <QPair>

class QWindow;

class WallpaperProcessor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int targetWidth READ targetWidth WRITE setTargetWidth NOTIFY targetWidthChanged)
    Q_PROPERTY(int targetHeight READ targetHeight WRITE setTargetHeight NOTIFY targetHeightChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString outputPath READ outputPath NOTIFY outputPathChanged)
    Q_PROPERTY(bool blurMode READ blurMode WRITE setBlurMode NOTIFY blurModeChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(bool autoColor READ autoColor WRITE setAutoColor NOTIFY autoColorChanged)
    Q_PROPERTY(int queueSize READ queueSize NOTIFY queueChanged)
    Q_PROPERTY(int queueProgress READ queueProgress NOTIFY queueProgressChanged)
    Q_PROPERTY(int screenWidth READ screenWidth NOTIFY screenWidthChanged)
    Q_PROPERTY(int screenHeight READ screenHeight NOTIFY screenHeightChanged)
    Q_PROPERTY(double windowDpr READ windowDpr NOTIFY windowDprChanged)
    Q_PROPERTY(bool keepAbove READ keepAbove NOTIFY keepAboveChanged)
    // ── New tweakable parameters ──
    Q_PROPERTY(int blurRadius READ blurRadius WRITE setBlurRadius NOTIFY blurRadiusChanged)
    Q_PROPERTY(double saturationFactor READ saturationFactor WRITE setSaturationFactor NOTIFY saturationFactorChanged)
    Q_PROPERTY(int bgGradientStyle READ bgGradientStyle WRITE setBgGradientStyle NOTIFY bgGradientStyleChanged)
    Q_PROPERTY(int bgGradientPreset READ bgGradientPreset WRITE setBgGradientPreset NOTIFY bgGradientPresetChanged)
    Q_PROPERTY(double gradientAngle READ gradientAngle WRITE setGradientAngle NOTIFY gradientAngleChanged)

public:
    explicit WallpaperProcessor(QObject *parent = nullptr);

    // ── Existing getters ──
    int targetWidth() const { return m_targetWidth; }
    int targetHeight() const { return m_targetHeight; }
    QString statusMessage() const { return m_statusMessage; }
    QString outputPath() const { return m_outputPath; }
    bool blurMode() const { return m_blurMode; }
    bool busy() const { return m_busy; }
    QColor backgroundColor() const { return m_bgColor; }
    bool autoColor() const { return m_autoColor; }
    int queueSize() const { return m_queue.size(); }
    int queueProgress() const { return m_queueProgress; }
    int screenWidth() const { return m_screenWidth; }
    int screenHeight() const { return m_screenHeight; }
    bool keepAbove() const { return m_keepAbove; }
    double windowDpr() const { return m_windowDpr; }
    // ── New getters ──
    int blurRadius() const { return m_blurRadius; }
    double saturationFactor() const { return m_saturationFactor; }
    int bgGradientStyle() const { return m_bgGradientStyle; }
    int bgGradientPreset() const { return m_bgGradientPreset; }
    double gradientAngle() const { return m_gradientAngle; }

    // ── Existing setters ──
    void setTargetWidth(int w);
    void setTargetHeight(int h);
    void setBlurMode(bool blur);
    void setBackgroundColor(const QColor &c);
    void setAutoColor(bool autoC);
    // ── New setters ──
    void setBlurRadius(int r);
    void setSaturationFactor(double f);
    void setBgGradientStyle(int s);
    void setBgGradientPreset(int p);
    void setGradientAngle(double a);

    /// Generate a small processed preview (400px max) — returns file:// URL
    Q_INVOKABLE QString generatePreview(const QString &sourcePath);

    /// Gradient preset access
    Q_INVOKABLE int gradientPresetCount() const;
    Q_INVOKABLE QString gradientPresetName(int index) const;
    Q_INVOKABLE QString gradientPresetColor1(int index) const;
    Q_INVOKABLE QString gradientPresetColor2(int index) const;

public Q_SLOTS:
    void detectScreenSize();
    void setWindow(QWindow *window);
    void setKeepAbove(bool keep);
    void processImage(const QString &sourcePath);
    void processQueue(const QStringList &paths);
    void cancelProcessing();
    void updateScreenSize(int w, int h);

private Q_SLOTS:
    void processNext();
    void detectFromWindow();
    void pollDpr();

Q_SIGNALS:
    void targetWidthChanged();
    void targetHeightChanged();
    void statusMessageChanged();
    void outputPathChanged();
    void blurModeChanged();
    void backgroundColorChanged();
    void autoColorChanged();
    void queueChanged();
    void queueProgressChanged();
    void busyChanged();
    void screenWidthChanged();
    void screenHeightChanged();
    void keepAboveChanged();
    void windowDprChanged();
    void processingStarted();
    void processingFinished();
    void errorOccurred(const QString &message);
    // ── New signals ──
    void blurRadiusChanged();
    void saturationFactorChanged();
    void bgGradientStyleChanged();
    void bgGradientPresetChanged();
    void gradientAngleChanged();

private:
    int m_targetWidth = 1920;
    int m_targetHeight = 1080;
    int m_screenWidth = 1920;
    int m_screenHeight = 1080;
    QString m_statusMessage;
    QString m_outputPath;
    bool m_blurMode = true;
    bool m_busy = false;
    QColor m_bgColor = Qt::white;
    bool m_autoColor = true;
    bool m_cancelRequested = false;
    QWindow *m_window = nullptr;
    double m_windowDpr = 1.0;
    int m_dprPollCount = 0;
    bool m_keepAbove = false;
    int m_detectAttempt = 0;

    QStringList m_queue;
    int m_queueProgress = 0;
    int m_currentIndex = 0;

    // ── New tweakable parameters ──
    int m_blurRadius = 0;          // 0 = auto (adaptive 0.051×H), 1–120 = manual
    double m_saturationFactor = 1.8;
    int m_bgGradientStyle = 0;      // 0 = Solid, 1 = Preset, 2 = Auto
    int m_bgGradientPreset = 0;     // index into s_presets[]
    double m_gradientAngle = 0.0;   // degrees (0 = horizontal, 90 = vertical, 45 = diagonal ↘)

    bool processSingleImage(const QString &sourcePath, QString &outPath);
    QImage renderWallpaper(const QImage &src, int W, int H);
    QColor extractAverageColor(const QImage &image);
    QPair<QColor, QColor> extractDominantColors(const QImage &image);

    static void stackBlur(QImage &image, int radius);
    static void boxBlurPass(QImage &image, int radius);
    static void boostSaturation(QImage &image, double factor);

    /// Gradient preset data
    struct GradientPreset {
        const char *name;   // i18n key
        QRgb color1;
        QRgb color2;
    };
    static const GradientPreset s_presets[10];
};

#endif // WALLPAPERPROCESSOR_H
