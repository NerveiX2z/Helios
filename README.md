# Helios — modular multi-language project scaffold (v0.5)

This package reorganizes the **working v0.4 GLES test code** into folders and provides optional Rust/Python/Lua development examples. **It does not yet add instancing or embed Lua/Rust into the engine.**

## Languages
- C++17: SDL2, OpenGL ES renderer, main loop, legacy raycaster and player
- GLSL ES 1.00: external vertex and fragment shaders (loaded at runtime)
- Python 3: offline benchmark log summarizer (standard library only)
- Rust: optional standalone map validator (`cargo` required to use)
- Lua: sample game script; **not integrated into runtime**

## Android / Termux setup
Install your existing C++/SDL2/OpenGL ES dependencies. For optional tools, install `python` and `rust` separately. No new dependencies are needed for the main engine compared to v0.4.

**Important:** Run from the project root so `shaders/basic.vert`, `shaders/basic.frag`, and `assets/maps/map.txt` can be found.

```sh
make clean && make
./helios
./helios --test
./helios --stress
./helios --raycaster
```

`--test` runs 0, 100, 500, 1000, 5000 extra cubes, progressing every 10 seconds. The 3D map is reloaded every 5 seconds. `--stress` is an alias.

Save and summarize benchmark logs:

```sh
./helios --test 2>&1 | tee test.log
python tools/benchmark.py test.log
```

Optional Rust validator:

```sh
cargo run --manifest-path rust/assets/Cargo.toml -- assets/maps/map.txt
```

## Migration
The archive includes the full source and a sample 12x16 map. **Back up your existing `assets/maps/map.txt` before extracting**, or keep your custom map rather than replacing it. Extract this project into a fresh directory if you want to avoid conflicts with the old layout. The new Makefile compiles only the new paths; stale `src/main.cpp` and `src/GLRenderer.cpp` from the old layout are not used.

## Status
This is a **folder-structure migration**, not a measured performance upgrade. The GLES code is inherited from v0.4. Compilation/runtime on Termux must be verified on-device. No GitHub commits were made.
