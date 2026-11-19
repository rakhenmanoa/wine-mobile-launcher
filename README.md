# Wine Mobile Launcher

![Build](https://github.com/rakhenmanoa/wine-mobile-launcher/actions/workflows/build.yml/badge.svg)

Un lanceur d'applications Windows (Wine) léger, dans l'esprit de **Lutris**, conçu pour **Plasma Mobile** et les écrans tactiles. Interface **Qt 6 / QML / Kirigami**.

## Fonctionnalités (v0.1)

- **Bibliothèque de jeux** : liste de vos applications Windows (.exe, .msi, .bat, .lnk), avec icône extraite automatiquement de l'exécutable (via `icoutils`, la plus grande variante du .ico jusqu'à 256×256, cache PNG par jeu ; ré-extraction possible depuis la fiche du jeu) ou image choisie manuellement, exécutable et préfixe
- **Affichage** : liste détaillée, grille d'icônes moyennes ou grandes icônes (préférence mémorisée) ; dans les grilles, appui sur une icône = lancement du jeu, appui long = fiche du jeu, sous-icône crayon sur chaque tuile = modifier le jeu
- **Préfixes Wine isolés** : un « C: » virtuel par jeu (comme Lutris) ; création en un clic via `wineboot -i`, ou préfixe automatique nommé d'après le jeu
- **Lancement / arrêt** : lance le jeu avec `WINEPREFIX` correctement défini ; bouton Arrêter (envoie `wineserver -k` au préfixe du jeu) ; option par jeu « Démarrer en plein écran (paysage) » — bureau virtuel Wine aux dimensions inversées de l'écran (90°), passé en plein écran par KWin via un script one-shot (ni `kscreen-doctor` ni `gamescope`, l'orientation du système n'est jamais modifiée)
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
src/main.cpp            point d'entrée Qt
src/gamemodel.{h,cpp}   modèle de bibliothèque (QAbstractListModel)
src/gamemanager.{h,cpp} logique : JSON, QProcess/wine, wineserver, préfixes
qml/                    UI Kirigami (bibliothèque, fiche jeu, édition, paramètres)
assets/                 icône SVG, .desktop, metainfo (AppStream)
```

## Limitations connues (v0.1)

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

Prototype v0.1 — le code suit les API stables Qt 6.5 / Kirigami 6 et n'a pas encore été testé sur du matériel Plasma Mobile réel. Retours de compilation et rapports de bug bienvenus.

## Licence

[MIT](LICENSE) — © 2026 rakhenmanoa
