# winewayland : pilote Wayland de Wine modifié

Patch du pilote Wayland de Wine (`dlls/winewayland.drv`) pour jouer sans sway ni
rotation de l'écran du système. Il est appliqué sur le Wine de **GE-Proton11-12**
(ValveSoftware/wine, commit `fda79a5` + les patchs `wine-wayland`,
`wineland-child-rendering` et `em-fixups` de GE-Proton, dans l'ordre de son script
de préparation). Seul `winewayland.so` (côté Unix, aarch64) change : le workflow
`build-winewayland.yml` le compile (artefact `winewayland-rotate-aarch64`, à
récupérer dans la page du run sur GitHub). À copier dans le dossier de Proton :
`files/lib/wine/aarch64-unix/winewayland.so` (garder l'original en `.orig`).

Le patch courant est `ge/0001-winewayland-rotation.patch` (le dossier `obsolete/`
garde les anciennes versions).

## Variables d'environnement

Le lanceur les définit tout seul à partir des réglages du jeu (onglet « Affichage
et graphisme ») ; elles ne servent à la main que pour tester.

| Variable | Effet |
| --- | --- |
| `WINE_WAYLAND_ROTATE=90\|270` | Wine voit un écran paysage, les tampons sont rendus en paysage et KWin les pivote (`wl_surface.set_buffer_transform`). Absente : aucune rotation. |
| `WINE_WAYLAND_KEEP_ASPECT=1` | Garde les proportions de l'image du jeu quand il change de résolution : la surface cliente est réduite au rectangle ajusté, sur un fond noir opaque. Nécessite le `win32u.so` modifié (voir `../win32u/`), qui publie la taille de la swapchain. |
| `WINE_WAYLAND_TOUCH_MOUSE=1` | Le premier doigt émule la souris (déplacement absolu + clic gauche). Par défaut le tactile n'envoie que les événements tactiles Windows. |
| `WINE_WAYLAND_MAX_HEIGHT=<n>` | Plafonne la résolution d'écran annoncée à Wine (côté court, mêmes proportions ; ex. 576 → 1280×576 sur un écran 2712×1220). L'échelle fractionnaire des fenêtres est ajustée du même rapport pour que le compositeur affiche toujours un plein écran. Les jeux qui prennent la résolution de l'écran rendent donc plus petit. |
| `WINE_WAYLAND_TOUCH_FLIP=x\|y\|xy` | Test : inverse les axes du tactile. |

Avec `WINEDEBUG=+waylanddrv` (ou `+cursor` pour le tactile), le pilote trace la
rotation, le plafond de résolution (`capping mode …`), le rectangle ajusté
(`keep aspect: …`) et les touches (`touch down raw=… window=… xy=…`).

## Rotation et tactile

`wl_surface.set_buffer_transform(90)` signifie que le client a pré-pivoté le tampon :
KWin affiche le contenu pivoté dans le sens **horaire**. Les conversions de
coordonnées (tactile, souris, rectangles des sous-surfaces, découpe des tampons
GDI) suivent ce sens (`surface_geom_rotation`). Avec le plafond de résolution, la
conversion tient aussi compte de l'échelle ajustée.

## Limites connues

- Le curseur n'est pas pivoté.
- La souris physique n'est pas corrigée pour les bandes noires du rectangle ajusté (seul le tactile l'est).
- Le plafond de résolution est global au processus Wine (une seule sortie prise en compte).
