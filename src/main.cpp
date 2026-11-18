#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("org.rakhenmanoa"));
    QGuiApplication::setApplicationName(QStringLiteral("wine-mobile-launcher"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.2.1"));

    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral(
        "qrc:/qt/qml/org/rakhenmanoa/wineLauncher/qml/main.qml")));

    return app.exec();
}
