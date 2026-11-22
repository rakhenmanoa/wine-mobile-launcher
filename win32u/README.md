# win32u : keep-aspect et FSR forcé

Patchs de `dlls/win32u/vulkan.c` du Wine de GE-Proton11-12. Le workflow
`build-win32u.yml` reproduit la préparation de GE-Proton (Wine, Wine-Staging,
patchs GE), applique ces patchs dans l'ordre alphabétique et compile
`win32u.so` (artefact `win32u-aarch64`, à récupérer dans la page du run). À
copier dans `files/lib/wine/aarch64-unix/win32u.so` de Proton (garder le
`win32u.so.orig`).

- `0001-win32u-keep-aspect.patch` : publie la taille de la swapchain du jeu dans la
  propriété de fenêtre `__wine_vk_extent`, que le pilote Wayland utilise pour
  `WINE_WAYLAND_KEEP_ASPECT` (proportions conservées, tactile compris). Ajoute aussi
  `WINE_FULLSCREEN_KEEP_ASPECT=1` pour le chemin « fullscreen hack ». Contient des
  traces `FIXME` de diagnostic.
- `0002-win32u-force-fshack.patch` : `WINE_FULLSCREEN_FORCE_FSHACK=1` force le chemin
  « fullscreen hack » (donc l'agrandissement **FSR 1** avec `WINE_FULLSCREEN_FSR=1`) même
  quand le jeu ne change pas de mode d'affichage, et malgré
  `WINE_DISABLE_FULLSCREEN_HACK=1` que Proton pose avec le pilote Wayland. La taille de
  sortie est celle de la zone cliente, ajustée aux proportions du jeu avec
  `WINE_WAYLAND_KEEP_ASPECT`. Le pilote Wayland annonce une taille minimale de swapchain
  de 1×1 : la taille n'est donc pas lue dans les capacités de la surface.

Le FSR est spatial (FSR 1 : EASU puis RCAS) : il agrandit l'image finale du jeu,
sans vecteurs de mouvement. Le facteur dépend de la résolution choisie dans le jeu
(ou du plafond `WINE_WAYLAND_MAX_HEIGHT`) par rapport à celle de l'écran.
