# winewayland : rotation du jeu

Patch du pilote Wayland de Wine (`dlls/winewayland.drv`) pour faire tourner
l'image d'un jeu sans sway ni rotation de l'écran du système, comme dans notre
fork de Cemu : Wine voit un écran **paysage**, les tampons sont rendus en
paysage et le compositeur les pivote (`wl_surface.set_buffer_transform`).

- Variable d'environnement : `WINE_WAYLAND_ROTATE=90` ou `270` (sens de rotation
  à choisir selon l'orientation du téléphone). Absente : aucun changement.
- Base : Wine de **GE-Proton11-12** (ValveSoftware/wine, commit `fda79a5`) + les patchs `wine-wayland`, `wineland-child-rendering` et `em-fixups` de GE-Proton, appliqués dans le même ordre que son script de préparation.
- Seul `winewayland.so` (côté Unix, aarch64) change : le CI le compile et le
  publie en artefact. À copier dans le dossier de Proton :
  `files/lib/wine/aarch64-unix/winewayland.so` (garder une copie de l'original).
- Test : lancer un jeu avec le pilote Wayland et `WINE_WAYLAND_ROTATE=90`.

Limites connues (version de test) :
- Pas encore de tactile (`wl_touch`) : à ajouter après validation de l'image.
- Le curseur n'est pas pivoté.
- Les surfaces Vulkan/OpenGL dépendent de Mesa : à vérifier.
