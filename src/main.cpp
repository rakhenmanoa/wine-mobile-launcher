#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTextStream>
#include <QUrl>

#include "virtualgamepad.h"

#ifdef HAVE_LAYERSHELLQT
#include <LayerShellQt/Shell>
#endif

// Mode « --gamepad-overlay <rotation> » : processus séparé qui crée la manette
// virtuelle (uinput) et affiche la manette à l'écran dans une couche layer-shell
// au-dessus des fenêtres plein écran. Séparé du launcher pour que seule cette
// fenêtre-là soit une surface layer-shell.
static int runGamepadOverlay(int argc, char *argv[], int rotation, bool editMode, bool standalone)
{
#ifdef HAVE_LAYERSHELLQT
    LayerShellQt::Shell::useLayerShell();
#endif
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("wine-mobile-launcher-gamepad"));

    VirtualGamepad pad;
    QString error;
    // Mode édition : pas besoin de manette uinput, on ne fait que déplacer les boutons.
    if (!standalone && !pad.open(&error)) {
        QTextStream(stderr) << error << Qt::endl;
        return 3;
    }
    if (!VirtualGamepad::layerShellAvailable())
        QTextStream(stderr) << "Manette à l'écran : layer-shell-qt absent, l'overlay peut "
                               "rester sous les fenêtres plein écran." << Qt::endl;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("pad"), &pad);
    engine.rootContext()->setContextProperty(QStringLiteral("overlayRotation"), rotation);
    engine.rootContext()->setContextProperty(QStringLiteral("overlayEdit"), editMode);
    engine.rootContext()->setContextProperty(QStringLiteral("overlayStandalone"), standalone);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral(
        "qrc:/qt/qml/org/rakhenmanoa/wineLauncher/qml/GamepadOverlay.qml")));
    return app.exec();
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "--gamepad-overlay") == 0) {
            bool edit = false, standalone = false;
            for (int j = i + 1; j < argc; ++j) {
                edit = edit || qstrcmp(argv[j], "--edit") == 0;
                standalone = standalone || qstrcmp(argv[j], "--standalone") == 0;
            }
            return runGamepadOverlay(argc, argv, i + 1 < argc ? QByteArray(argv[i + 1]).toInt() : 0,
                                     edit || standalone, standalone);
        }
    }

    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("org.rakhenmanoa"));
    QGuiApplication::setApplicationName(QStringLiteral("wine-mobile-launcher"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.2.18"));

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
