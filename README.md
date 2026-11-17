# Wine Mobile Launcher

![Build](https://github.com/rakhenmanoa/wine-mobile-launcher/actions/workflows/build.yml/badge.svg)

Un lanceur d'applications Windows (Wine) léger, dans l'esprit de **Lutris**, conçu pour **Plasma Mobile** et les écrans tactiles. Interface **Qt 6 / QML / Kirigami**.

## Fonctionnalités (v0.1)

- **Bibliothèque de jeux** : liste de vos applications Windows (.exe, .msi, .bat, .lnk), avec icône, exécutable et préfixe
- **Préfixes Wine isolés** : un « C: » virtuel par jeu (comme Lutris) ; création en un clic via `wineboot -i`, ou préfixe automatique nommé d'après le jeu
- **Lancement / arrêt** : lance le jeu avec `WINEPREFIX` correctement défini ; bouton Arrêter (envoie `wineserver -k` au préfixe du jeu)
- **Binaire Wine configurable** : système (`wine`) ou personnalisé (Wine-GE, Proton…), avec détection automatique
- **Journal intégré** : sortie stdout/stderr de Wine consultable depuis la page du jeu
- **Persistance** : bibliothèque en JSON dans `~/.local/share/wine-mobile-launcher/` (chemin AppData standard)

## Dépendances

**Runtime** : `wine` (paquet de votre distribution), Qt 6.5+, Kirigami (modules QML KDE).

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
- **Paquet ALARM aarch64** : chroot ALARM émulé avec QEMU (`qemu-user-static` + binfmt), puis `makepkg` avec le PKGBUILD — le commit compilé est fixé par la variable `WML_COMMIT` (`source=(...#commit=...)`), donc l'artefact correspond toujours au push qui l'a déclenché. Les `.pkg.tar.zst` et le `.SRCINFO` sont déposés en artefacts téléchargeables dans la page du run.

## Utilisation

1. **Ajouter un jeu** : bouton « Ajouter un jeu » → nom, exécutable `.exe` (sélecteur de fichiers), préfixe Wine (laisser vide pour un préfixe automatique)
2. **Créer le préfixe** (première fois) : bouton « Créer / initialiser ce préfixe » → lance `wineboot -i`
3. **Jouer** : bouton Jouer (fiche du jeu ou de la bibliothèque) ; **Arrêter** envoie `wineserver -k`
4. **Paramètres** : binaire Wine par défaut, dossier parent des préfixes

## Architecture du code

```
src/main.cpp            point d'entrée Qt
src/gamemodel.{h,cpp}   modèle de bibliothèque (QAbstractListModel)
src/gamemanager.{h,cpp} logique : JSON, QProcess/wine, wineserver, préfixes
qml/                    UI Kirigami (bibliothèque, fiche jeu, édition, paramètres)
assets/                 icône SVG, .desktop, metainfo (AppStream)
```

## Limitations connues (v0.1)

- Les jeux lancés sont suivis par le launcher : si celui-ci est fermé, les processus enfants sont arrêtés (limite QProcess ; une version détachée est prévue)
- Pas encore de gestion des runners Lutris (installation de Wine-GE, Proton), ni de winetricks graphique
- Non packagé Flatpak ; sous Flatpak, le lancement de Wine hôte nécessiterait `flatpak-spawn --host`

## Feuille de route

- [ ] Détachement propre des processus à la fermeture du launcher
- [ ] Gestion des runners (Wine-GE / Proton téléchargeables, comme Lutris)
- [ ] Intégration winetricks (VC++ redist, fonts…)
- [ ] Packaging Flatpak + opensuse kiwi
- [ ] Traductions (i18n KF6)

## Statut

Prototype v0.1 — le code suit les API stables Qt 6.5 / Kirigami 6 et n'a pas encore été testé sur du matériel Plasma Mobile réel. Retours de compilation et rapports de bug bienvenus.

## Licence

[MIT](LICENSE) — © 2026 rakhenmanoa
