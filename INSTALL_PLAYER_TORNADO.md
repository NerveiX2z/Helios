# Helios v0.5 — Player + Tornado (Termux/X11)

Full source package. Preserves GLES2 renderer and legacy raycaster. Uses the existing 0/1 map loader with the 64x64 storm map. No additional dependencies.

## Install

```sh
pkg install clang make sdl2
cd ~/Helios_Test
cp -a . ../Helios_Test_backup
# Extract this ZIP directly into ~/Helios_Test, overwriting matching source files.
make clean && make -j2
./helios
```

If you extract to `~/storage/downloads/helios_player_tornado_full`, copy the extracted folder's **contents** into `~/Helios_Test` using `cp -a .../. ~/Helios_Test/`.

## Controls

- W / Up: move forward
- S / Down: move backward
- A / Left: rotate left
- D / Right: rotate right
- Q: strafe left
- E: strafe right
- Shift: sprint
- Esc: exit

The 3D camera follows the player. The tornado is an animated funnel of cubes drifting through the map. No tornado wind, damage, collision, particles or weather physics yet. Legacy raycaster remains available using `./helios --raycaster`.

## Test

```sh
make clean && make -j2
./helios
./helios --raycaster
./helios --test
```

Expected: `[MAP] Reloaded 64 rows x 64 columns`, GPU identification, 3D character, animated funnel. Performance may fall on Mesa llvmpipe because the funnel uses 156 additional draw calls. This has not been verified on the user's Termux device.

Phase 2 caution: all state is updated on the main thread; no job system or event queue is introduced. The renderer retains SDL2/OpenGL types internally; there are no shared AI/physics interfaces.
