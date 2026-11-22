#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class QWindow;

// Manette virtuelle (uinput) pilotée par la manette à l'écran. Elle se présente
// comme une manette Xbox 360 : Wine / Proton la détectent comme une vraie manette.
// Il faut avoir accès à /dev/uinput (règle udev fournie par le paquet).
class VirtualGamepad : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool opened READ isOpen NOTIFY openedChanged)
    // Vrai si l'overlay peut être posé au-dessus des fenêtres plein écran (layer-shell).
    Q_PROPERTY(bool layerShell READ layerShell CONSTANT)

public:
    explicit VirtualGamepad(QObject *parent = nullptr);
    ~VirtualGamepad() override;

    bool isOpen() const { return m_fd >= 0; }
    static bool layerShellAvailable();
    bool layerShell() const { return layerShellAvailable(); }

    // Crée la manette. Renvoie false (raison dans *error) si /dev/uinput est inaccessible.
    bool open(QString *error = nullptr);
    void close();

    // code : constantes du noyau (BTN_*, ABS_*), voir GamepadOverlay.qml
    Q_INVOKABLE void button(int code, bool pressed);
    // valeur normalisée : sticks et hat -1..1, gâchettes 0..1
    Q_INVOKABLE void axis(int code, double value);
    // Bouton Home : retour à l'accueil de Plasma (Afficher le bureau), le jeu continue.
    Q_INVOKABLE void goHome();

    // Fenêtre de l'overlay : à appeler avant son premier affichage
    // (layer-shell : couche « overlay », sans prise de focus, plein écran).
    Q_INVOKABLE void setupOverlay(QWindow *window);
    // Zone qui reçoit les touches : le reste de l'écran traverse l'overlay.
    // rects : liste de { x, y, w, h } en pixels de la fenêtre.
    Q_INVOKABLE void setInputMask(QWindow *window, const QVariantList &rects);

signals:
    void openedChanged();

private:
    void emitEvent(int type, int code, int value);

    int m_fd = -1;
};
