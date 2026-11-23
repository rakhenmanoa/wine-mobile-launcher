# Wine Mobile Launcher

![Build](https://github.com/rakhenmanoa/wine-mobile-launcher/actions/workflows/build.yml/badge.svg)

Un lanceur d'applications Windows (Wine) léger, dans l'esprit de **Lutris**, conçu pour **Plasma Mobile** et les écrans tactiles. Interface **Qt 6 / QML / Kirigami**.

## Fonctionnalités

- **Bibliothèque de jeux** : liste de vos applications Windows (.exe, .msi, .bat, .lnk), avec icône extraite automatiquement de l'exécutable (via `icoutils`, la plus grande variante du .ico jusqu'à 256×256, cache PNG par jeu ; ré-extraction possible depuis la fiche du jeu) ou image choisie manuellement, exécutable et préfixe
- **Affichage** : liste détaillée, grille d'icônes moyennes ou grandes icônes (préférence mémorisée) ; dans les grilles, appui sur une icône = lancement du jeu, appui long = fiche du jeu, sous-icône crayon sur chaque tuile = modifier le jeu
- **Préfixes Wine isolés** : un « C: » virtuel par jeu (comme Lutris) ; création en un clic via `wineboot -i`, ou préfixe automatique nommé d'après le jeu
- **Lancement / arrêt** : la mise en veille et l'extinction de l'écran sont suspendues tant qu'un jeu tourne (inhibition D-Bus KDE) ; lance le jeu avec `WINEPREFIX` correctement défini ; bouton Arrêter (envoie `wineserver -k` au préfixe du jeu) ; option par jeu « Démarrer en plein écran (paysage) » — le jeu tourne dans un `sway` imbriqué dont la sortie est pivotée de 90° (seul le jeu est pivoté, pas l'écran du système ; sens réglable avec `WML_ROTATION=right|left|inverted`), le jeu démarre en fenêtré, puis la fenêtre de sway est passée en vrai plein écran (sans barre de navigation) par un petit script KWin chargé en D-Bus avec `qdbus6` (paquet `qt6-tools`), sans règle de fenêtre persistante ; sans `sway`, bureau virtuel Wine en plein écran sans rotation d'image.
- **Plein écran sans sway (Wayland)** : avec le pilote Wayland de Wine (option par jeu), l'image est pivotée par Wine lui-même, garde ses proportions quand le jeu change de résolution, avec tactile (et option « tactile émulé en souris »), plafond de résolution d'écran et agrandissement FSR 1 — voir « Affichage et graphisme » plus bas
- **Manette à l'écran** : manette tactile virtuelle (uinput) à disposition personnalisable, bouton ⌂ vers l'accueil de Plasma
- **Réglages enregistrés automatiquement** (fiche du jeu et paramètres)
- **Runner configurable** : Wine système (`wine`), binaire personnalisé (Wine-GE…) ou **GE-Proton / Proton** (y compris les builds **aarch64**), avec détection automatique
- **Journal intégré** : sortie stdout/stderr de Wine consultable depuis la page du jeu
- **Journal verbose** : si l'option est cochée, le journal est aussi recopié dans le terminal et enregistré dans `~/.local/share/wine-mobile-launcher/wine-mobile-launcher.log` (rotation à 5 Mo)
- **Persistance** : bibliothèque en JSON dans `~/.local/share/wine-mobile-launcher/` (chemin AppData standard)

## Dépendances

**Runtime** : `wine` (paquet de votre distribution), Qt 6.5+, Kirigami (modules QML KDE), `icoutils` (extraction des icônes des exécutables).

**Compilation** (Debian / Ubuntu / KDE Neon) :

```bash
sudo apt install build-essential cmake \
    qt6-base-dev qt6-declarative-dev \
    kirigami qml6-module-org-kde-kirigami \
    qml6-module-qtquick-controls qml6-module-qtquick-layouts \
    qml6-module-qtquick-dialogs qml6-module-qtquick-templates \
    qml6-module-qtqml-workerscript qqc2-desktop-style
```

*(Les noms de paquets peuvent varier selon la distribution ; sous Arch : `qt6-base`, `qt6-declarative`, `kirigami`, `qqc2-desktop-style`.)*

## Compiler et installer

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Exécution directe depuis le dossier de build :
./build/wine-mobile-launcher

# Ou installation système :
sudo cmake --install build
```

## Packaging Arch Linux / Arch Linux ARM

Un [PKGBUILD](packaging/arch/PKGBUILD) est fourni, ciblé `aarch64` (ALARM) et `x86_64`. Les dépendances listées existent dans le dépôt `extra` d'ALARM pour aarch64 : `qt6-base`, `qt6-declarative`, `kirigami`, `qqc2-desktop-style`, `breeze-icons`.

**Sur un appareil ALARM / Arch** :

```bash
cd packaging/arch
makepkg -f
sudo pacman -U wine-mobile-launcher-*.pkg.tar.zst
```

**En CI (GitHub Actions)** : le workflow [build.yml](.github/workflows/build.yml) tourne à chaque push et fournit deux jobs :

- **Build natif Arch x86_64** (conteneur `archlinux:base-devel`) : vérification rapide de la compilation ;
- **Paquet ALARM aarch64** : rootfs ALARM importé comme image Docker et exécuté sous QEMU (`docker/setup-qemu-action` pour l'émulation, indépendant des paquets qemu-* de l'image runner), avec `makepkg` et le PKGBUILD — le commit compilé est fixé par la variable `WML_COMMIT` (`source=(...#commit=...)`), donc l'artefact correspond toujours au push qui l'a déclenché. Les `.pkg.tar.zst` et le `.SRCINFO` sont déposés en artefacts téléchargeables dans la page du run.

## GE-Proton (aarch64)

Dans **Paramètres → Dossier GE-Proton par défaut** (ou, par jeu, dans le champ **GE-Proton** de la fiche d'édition), indiquez le **dossier GE-Proton extrait** (celui qui contient le script `proton` et `files/`). Ordre de priorité : GE-Proton du jeu, Wine du jeu, GE-Proton par défaut, Wine par défaut, `wine` système. Le launcher détecte alors Proton et :

- lance le jeu avec `python3 <dossier>/proton run jeu.exe` (`STEAM_COMPAT_DATA_PATH` = préfixe du jeu, `STEAM_COMPAT_CLIENT_INSTALL_PATH` défini, `PROTONFIXES_DISABLE=1`) ;
- utilise le sous-dossier `pfx/` du préfixe du jeu comme `WINEPREFIX` réel ; « Créer le préfixe » passe aussi par `proton run wineboot` ;
- arrête le jeu avec le `wineserver -k` du dossier GE-Proton ;
- avertit dans le journal si le runner est un binaire x86_64 sur un hôte aarch64 (il faut alors box64/FEX, ou une build GE-Proton aarch64).

`python3` doit être installé. Aucun runtime Steam / pressure-vessel n'est utilisé.

## Utilisation

1. **Ajouter un jeu** : bouton « Ajouter un jeu » → nom, exécutable `.exe` (sélecteur de fichiers), préfixe Wine (laisser vide pour un préfixe automatique)
2. **Créer le préfixe** (première fois) : bouton « Créer / initialiser ce préfixe » → lance `wineboot -i`
3. **Jouer** : bouton Jouer (fiche du jeu ou de la bibliothèque) ; **Arrêter** envoie `wineserver -k`
4. **Paramètres** : binaire Wine par défaut, dossier parent des préfixes

## Architecture du code

```
src/main.cpp               point d'entrée Qt (et mode --gamepad-overlay)
src/gamemodel.{h,cpp}      modèle de bibliothèque (QAbstractListModel)
src/gamemanager.{h,cpp}    logique : JSON, QProcess/wine/proton, wineserver, préfixes, environnement des jeux
src/virtualgamepad.{h,cpp} manette uinput, overlay layer-shell, disposition (JSON)
qml/                       UI Kirigami (bibliothèque, fiche jeu, édition, paramètres, GamepadOverlay)
winewayland/               patch du pilote Wayland de Wine (rotation, tactile, plafond de résolution)
win32u/                    patchs win32u (proportions, FSR forcé)
.github/workflows/         build du paquet, de winewayland.so et de win32u.so
packaging/arch/            PKGBUILD, règle udev uinput
assets/                    icône SVG, .desktop, metainfo (AppStream)
```

## Limitations connues

- Les jeux lancés sont suivis par le launcher : si celui-ci est fermé, les processus enfants sont arrêtés (limite QProcess ; une version détachée est prévue)
- Pas encore de téléchargement automatique des runners (Wine-GE, GE-Proton à extraire manuellement), ni de winetricks graphique
- Non packagé Flatpak ; sous Flatpak, le lancement de Wine hôte nécessiterait `flatpak-spawn --host`

## Feuille de route

- [ ] Détachement propre des processus à la fermeture du launcher
- [ ] Gestion des runners (Wine-GE / Proton téléchargeables, comme Lutris)
- [ ] Intégration winetricks (VC++ redist, fonts…)
- [ ] Packaging Flatpak + opensuse kiwi
- [ ] Traductions (i18n KF6)

## Statut

Version 0.2.x, testée sur un Xiaomi 12T Pro (ALARM, Plasma Mobile, GE-Proton aarch64). Les options Wayland, tactile, résolution maximale et FSR demandent les fichiers Wine modifiés (`winewayland.so`, `win32u.so`), à copier à la main dans GE-Proton ; le reste fonctionne avec un Proton standard. Retours de compilation et rapports de bug bienvenus.

## Licence

[MIT](LICENSE) — © 2026 rakhenmanoa

## Affichage et graphisme (onglet de la fiche du jeu)

Réglages par jeu, enregistrés automatiquement :

- **Démarrer en plein écran** et **Wayland** : avec Wayland, le jeu tourne avec `PROTON_ENABLE_WAYLAND=1`, sans sway ; en plein écran l'image est pivotée par Wine (`WINE_WAYLAND_ROTATE`, sens réglable avec `WML_ROTATION=left|inverted`) et garde ses proportions (`WINE_WAYLAND_KEEP_ASPECT`).
- **Tactile émulé en souris** : le premier doigt déplace le curseur et clique (`WINE_WAYLAND_TOUCH_MOUSE`).
- **Pavé tactile pendant le contrôle à la souris** : pour les jeux qui lisent la souris par recentrage du curseur (combats de Child of Light), le doigt devient un pavé tactile relatif (déplacements relatifs, tap = clic, doigt immobile = clic maintenu) ; hors de ce mode, le tactile reste direct (`WINE_WAYLAND_TOUCH_PAD_COMBAT`).
- **Résolution maximale vue par le jeu** : aucune limite, 1080p, 900p, 720p, 576p, 480p ou 407p (taille logique de Plasma) ; plafonne la résolution d'écran annoncée à Wine (`WINE_WAYLAND_MAX_HEIGHT`) pour que les jeux qui s'adaptent à l'écran rendent plus petit.
- **Agrandissement FSR** : quand le jeu rend plus petit que l'écran, l'image est agrandie en FSR 1 plutôt qu'étirée (`WINE_FULLSCREEN_FORCE_FSHACK` + `WINE_FULLSCREEN_FSR`).
- **OpenGL via Zink**, **Manette à l'écran** (voir plus bas ; seulement en plein écran).

Les options Wayland, tactile, résolution maximale et FSR demandent les fichiers Wine modifiés, à copier à la main dans GE-Proton aarch64 : [`winewayland.so`](winewayland/README.md) (rotation, tactile, plafond de résolution) et [`win32u.so`](win32u/README.md) (proportions, FSR). Ils sont compilés par les workflows GitHub Actions `build-winewayland.yml` et `build-win32u.yml` (déclenchés sur les branches `feature/**` ou à la demande) ; les artefacts se téléchargent dans la page du run.

## Manette à l'écran

Option par jeu (« Manette à l'écran » dans les réglages du jeu) : une manette tactile (sticks, croix, ABXY, L/R/ZL/ZR, −/Home/+) est superposée au jeu et présentée à Wine comme une manette Xbox 360 virtuelle (uinput). Elle tourne dans un processus séparé (`wine-mobile-launcher --gamepad-overlay`), posé dans une couche layer-shell (paquet `layer-shell-qt`) pour rester au-dessus du plein écran, et pivoté comme le jeu en mode paysage.

Le bouton ⌂ ramène à l'accueil de Plasma (le jeu continue) ; A et B suivent la disposition Nintendo (A à droite, B en bas).

**Disposition personnalisable** : le bouton « Modifier la disposition des boutons… » (onglet Affichage et graphisme) ouvre l'éditeur par-dessus le lanceur ; la case « Éditer la disposition au prochain lancement » l'ouvre dans le jeu. On glisse chaque contrôle (croix, sticks, A, B, X, Y, ZL, L, ZR, R, −, ⌂, +), on le sélectionne puis **− / +** change sa taille (0,5× à 2×), **Réinitialiser** revient à la disposition par défaut. Elle est commune à tous les jeux et enregistrée dans `~/.config/wine-mobile-launcher/gamepad-layout.json`.

Prérequis : accès à `/dev/uinput` (règle udev `70-wml-uinput.rules` fournie ; recharger udev ou se reconnecter) et le module `uinput`.

Les réglages d'un jeu et les paramètres généraux sont enregistrés automatiquement (plus de bouton « Enregistrer »).
