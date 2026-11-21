#include "virtualgamepad.h"

#include <QProcess>
#include <QRegion>
#include <QWindow>

#include <cmath>
#include <cstring>

#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#ifdef HAVE_LAYERSHELLQT
#include <LayerShellQt/Window>
#endif

VirtualGamepad::VirtualGamepad(QObject *parent)
    : QObject(parent)
{
}

VirtualGamepad::~VirtualGamepad()
{
    close();
}

bool VirtualGamepad::layerShellAvailable()
{
#ifdef HAVE_LAYERSHELLQT
    return true;
#else
    return false;
#endif
}

#ifdef Q_OS_LINUX
namespace {
void setupAxis(int fd, int code, int minimum, int maximum, int fuzz, int flat)
{
    uinput_abs_setup setup;
    std::memset(&setup, 0, sizeof(setup));
    setup.code = code;
    setup.absinfo.minimum = minimum;
    setup.absinfo.maximum = maximum;
    setup.absinfo.fuzz = fuzz;
    setup.absinfo.flat = flat;
    ioctl(fd, UI_ABS_SETUP, &setup);
}
} // namespace
#endif

bool VirtualGamepad::open(QString *error)
{
#ifdef Q_OS_LINUX
    if (m_fd >= 0)
        return true;
    const int fd = ::open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        if (error)
            *error = QStringLiteral("Manette à l'écran : /dev/uinput inaccessible (%1). "
                                    "Installez la règle udev du paquet puis redémarrez, ou ajoutez-vous au groupe « input ».")
                         .arg(QString::fromLocal8Bit(std::strerror(errno)));
        return false;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    const int keys[] = {BTN_SOUTH, BTN_EAST, BTN_NORTH, BTN_WEST, BTN_TL, BTN_TR, BTN_SELECT,
                        BTN_START, BTN_MODE, BTN_THUMBL, BTN_THUMBR};
    for (int key : keys)
        ioctl(fd, UI_SET_KEYBIT, key);

    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    for (int code : {ABS_X, ABS_Y, ABS_RX, ABS_RY, ABS_Z, ABS_RZ, ABS_HAT0X, ABS_HAT0Y})
        ioctl(fd, UI_SET_ABSBIT, code);
    setupAxis(fd, ABS_X, -32768, 32767, 16, 128);
    setupAxis(fd, ABS_Y, -32768, 32767, 16, 128);
    setupAxis(fd, ABS_RX, -32768, 32767, 16, 128);
    setupAxis(fd, ABS_RY, -32768, 32767, 16, 128);
    setupAxis(fd, ABS_Z, 0, 255, 0, 0);
    setupAxis(fd, ABS_RZ, 0, 255, 0, 0);
    setupAxis(fd, ABS_HAT0X, -1, 1, 0, 0);
    setupAxis(fd, ABS_HAT0Y, -1, 1, 0, 0);

    uinput_setup setup;
    std::memset(&setup, 0, sizeof(setup));
    setup.id.bustype = BUS_USB;
    setup.id.vendor = 0x045e;  // Microsoft
    setup.id.product = 0x028e; // Xbox 360 Controller
    setup.id.version = 0x0110;
    std::strncpy(setup.name, "Microsoft X-Box 360 pad", UINPUT_MAX_NAME_SIZE - 1);
    if (ioctl(fd, UI_DEV_SETUP, &setup) < 0 || ioctl(fd, UI_DEV_CREATE) < 0) {
        if (error)
            *error = QStringLiteral("Manette à l'écran : création de la manette virtuelle impossible (%1).")
                         .arg(QString::fromLocal8Bit(std::strerror(errno)));
        ::close(fd);
        return false;
    }
    m_fd = fd;
    Q_EMIT openedChanged();
    return true;
#else
    if (error)
        *error = QStringLiteral("Manette à l'écran : uniquement sous Linux.");
    return false;
#endif
}

void VirtualGamepad::close()
{
#ifdef Q_OS_LINUX
    if (m_fd < 0)
        return;
    ioctl(m_fd, UI_DEV_DESTROY);
    ::close(m_fd);
    m_fd = -1;
    Q_EMIT openedChanged();
#endif
}

void VirtualGamepad::emitEvent(int type, int code, int value)
{
#ifdef Q_OS_LINUX
    if (m_fd < 0)
        return;
    input_event event;
    std::memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code;
    event.value = value;
    if (::write(m_fd, &event, sizeof(event)) < 0)
        return;
    event.type = EV_SYN;
    event.code = SYN_REPORT;
    event.value = 0;
    if (::write(m_fd, &event, sizeof(event)) < 0)
        return;
#else
    Q_UNUSED(type)
    Q_UNUSED(code)
    Q_UNUSED(value)
#endif
}

void VirtualGamepad::button(int code, bool pressed)
{
#ifdef Q_OS_LINUX
    emitEvent(EV_KEY, code, pressed ? 1 : 0);
#else
    Q_UNUSED(code)
    Q_UNUSED(pressed)
#endif
}

void VirtualGamepad::goHome()
{
    // Plasma Mobile : « Afficher le bureau » ramène à l'écran d'accueil.
    QProcess::startDetached(QStringLiteral("qdbus6"),
                            {QStringLiteral("org.kde.kglobalaccel"),
                             QStringLiteral("/component/kwin"),
                             QStringLiteral("org.kde.kglobalaccel.Component.invokeShortcut"),
                             QStringLiteral("Show Desktop")});
}

void VirtualGamepad::axis(int code, double value)
{
#ifdef Q_OS_LINUX
    int raw = 0;
    switch (code) {
    case ABS_Z:
    case ABS_RZ:
        raw = int(std::lround(qBound(0.0, value, 1.0) * 255.0));
        break;
    case ABS_HAT0X:
    case ABS_HAT0Y:
        raw = int(std::lround(qBound(-1.0, value, 1.0)));
        break;
    default:
        raw = int(std::lround(qBound(-1.0, value, 1.0) * 32767.0));
        break;
    }
    emitEvent(EV_ABS, code, raw);
#else
    Q_UNUSED(code)
    Q_UNUSED(value)
#endif
}

void VirtualGamepad::setupOverlay(QWindow *window)
{
    if (!window)
        return;
#ifdef HAVE_LAYERSHELLQT
    if (auto *layer = LayerShellQt::Window::get(window)) {
        layer->setLayer(LayerShellQt::Window::LayerOverlay);
        layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
        layer->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop
                                                        | LayerShellQt::Window::AnchorBottom
                                                        | LayerShellQt::Window::AnchorLeft
                                                        | LayerShellQt::Window::AnchorRight));
        layer->setExclusiveZone(-1);
        layer->setScope(QStringLiteral("wml-gamepad"));
    }
#endif
}

void VirtualGamepad::setInputMask(QWindow *window, const QVariantList &rects)
{
    if (!window)
        return;
    QRegion region;
    for (const QVariant &entry : rects) {
        const QVariantMap map = entry.toMap();
        region += QRect(qRound(map.value(QStringLiteral("x")).toDouble()),
                        qRound(map.value(QStringLiteral("y")).toDouble()),
                        qRound(map.value(QStringLiteral("w")).toDouble()),
                        qRound(map.value(QStringLiteral("h")).toDouble()));
    }
    window->setMask(region);
}
