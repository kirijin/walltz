#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QUrl>
#include <QQuickStyle>
#include <KLocalizedString>
#include <KLocalizedQmlContext>
#include <KIconTheme>

#include "WallpaperProcessor.h"

int main(int argc, char *argv[])
{
    KIconTheme::initTheme();
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("walltz");

    QApplication::setOrganizationName(QStringLiteral("Walltz"));
    QApplication::setOrganizationDomain(QStringLiteral("walltz.app"));
    QApplication::setApplicationName(QStringLiteral("walltz"));
    QApplication::setApplicationDisplayName(i18n("Walltz"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    qmlRegisterType<WallpaperProcessor>("org.walltz.processor", 1, 0, "WallpaperProcessor");

    QQmlApplicationEngine engine;

    auto ctx = new KLocalizedQmlContext(&engine);
    engine.rootContext()->setContextObject(ctx);

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/org/walltz/walltz/Main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
