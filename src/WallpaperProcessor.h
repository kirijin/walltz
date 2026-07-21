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
    Q_PROPERTY(int aspectMode READ aspectMode WRITE setAspectMode NOTIFY aspectModeChanged)
    // ── New tweakable parameters ──
    Q_PROPERTY(int blurRadius READ blurRadius WRITE setBlurRadius NOTIFY blurRadiusChanged)
    Q_PROPERTY(double saturationFactor READ saturationFactor WRITE setSaturationFactor NOTIFY saturationFactorChanged)
    Q_PROPERTY(int bgGradientStyle READ bgGradientStyle WRITE setBgGradientStyle NOTIFY bgGradientStyleChanged)
    Q_PROPERTY(int bgGradientPreset READ bgGradientPreset WRITE setBgGradientPreset NOTIFY bgGradientPresetChanged)
    Q_PROPERTY(double gradientAngle READ gradientAngle WRITE setGradientAngle NOTIFY gradientAngleChanged)
    Q_PROPERTY(double bgZoom READ bgZoom WRITE setBgZoom NOTIFY bgZoomChanged)
    Q_PROPERTY(double bgBlurAngle READ bgBlurAngle WRITE setBgBlurAngle NOTIFY bgBlurAngleChanged)
    Q_PROPERTY(int autoMood READ autoMood WRITE setAutoMood NOTIFY autoMoodChanged)
    Q_PROPERTY(bool useV2 READ useV2 WRITE setUseV2 NOTIFY useV2Changed)
    Q_PROPERTY(double vignetteStrength READ vignetteStrength WRITE setVignetteStrength NOTIFY vignetteStrengthChanged)
    Q_PROPERTY(double grainStrength READ grainStrength WRITE setGrainStrength NOTIFY grainStrengthChanged)

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
    int aspectMode() const { return m_aspectMode; }
    // ── New getters ──
    int blurRadius() const { return m_blurRadius; }
    double saturationFactor() const { return m_saturationFactor; }
    int bgGradientStyle() const { return m_bgGradientStyle; }
    int bgGradientPreset() const { return m_bgGradientPreset; }
    double gradientAngle() const { return m_gradientAngle; }
    double bgZoom() const { return m_bgZoom; }
    double bgBlurAngle() const { return m_bgBlurAngle; }
    int autoMood() const { return m_autoMood; }
    bool useV2() const { return m_useV2; }
    double vignetteStrength() const { return m_vignetteStrength; }
    double grainStrength() const { return m_grainStrength; }

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
    void setBgZoom(double z);
    void setBgBlurAngle(double a);
    void setAutoMood(int m);
    void setUseV2(bool v2);
    void setVignetteStrength(double s);
    void setGrainStrength(double s);

    /// Generate a small processed preview (400px max) — returns file:// URL
    Q_INVOKABLE QString generatePreview(const QString &sourcePath);

    /// Gradient preset access
    Q_INVOKABLE int gradientPresetCount() const;
    Q_INVOKABLE QString gradientPresetName(int index) const;
    Q_INVOKABLE QString gradientPresetColor1(int index) const;
    Q_INVOKABLE QString gradientPresetColor2(int index) const;
    Q_INVOKABLE double aspectRatioForMode(int mode) const;

    /// Mood palette access (auto-gradient variants)
    Q_INVOKABLE int moodCount() const { return 6; }
    Q_INVOKABLE QString moodName(int index) const;
    Q_INVOKABLE QString moodColorA(int index) const;
    Q_INVOKABLE QString moodColorB(int index) const;

    /// V2 mood palette access (3D RGB histogram — second row)
    Q_INVOKABLE QString moodNameV2(int index) const;
    Q_INVOKABLE QString moodColorV2A(int index) const;
    Q_INVOKABLE QString moodColorV2B(int index) const;

public Q_SLOTS:
    void detectScreenSize();
    void setWindow(QWindow *window);
    void setKeepAbove(bool keep);
    void setAspectMode(int mode);
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
    void aspectModeChanged();
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
    void bgZoomChanged();
    void bgBlurAngleChanged();
    void autoMoodChanged();
    void useV2Changed();
    void vignetteStrengthChanged();
    void grainStrengthChanged();

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
    bool m_keepAbove = false;
    int m_aspectMode = 0;
    double m_aspectRatio = 0.0;
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
    double m_bgZoom = 1.0;          // background zoom multiplier (0.5–3.0, 1.0 = fill)
    double m_bgBlurAngle = 0.0;     // blur background rotation (degrees, 0 = normal)
    int m_autoMood = 0;             // 0=Auto, 1=Soft, 2=Vivid, 3=Warm, 4=Cool, 5=Deep
    bool m_useV2 = false;           // use V2 (3D RGB histogram) instead of V1
    double m_vignetteStrength = 0.0; // vignette: 0 = off, 1 = max
    double m_grainStrength = 0.0;    // grain: 0 = off, 1 = max
    QColor m_moodColorsA[6];        // cached mood gradient color A (index = mood)
    QColor m_moodColorsB[6];        // cached mood gradient color B
    QColor m_moodColorsV2A[6];      // V2 mood gradient color A (second row)
    QColor m_moodColorsV2B[6];      // V2 mood gradient color B
    bool m_moodsComputed = false;   // true after computeAllMoods()

    bool processSingleImage(const QString &sourcePath, QString &outPath);
    QImage renderWallpaper(const QImage &src, int W, int H);
    QColor extractAverageColor(const QImage &image);
    QPair<QColor, QColor> extractHarmonizedColors(const QImage &image, int mood = 0);
    void computeMoodPalettes(const QImage &image);
    void computeMoodPalettesV2(const QImage &image);

    /// 3D RGB histogram centroid (for V2)
    struct Centroid3D {
        double r, g, b;
        double score;
        int ri, gi, bi;
        int count;
    };

    static void stackBlur(QImage &image, int radius);
    static void boostSaturation(QImage &image, double factor);

    /// Gradient preset data
    struct GradientPreset {
        const char *name;   // i18n key
        QRgb color1;
        QRgb color2;
    };
    static const GradientPreset s_presets[25];
};

#endif // WALLPAPERPROCESSOR_H
