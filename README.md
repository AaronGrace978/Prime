# Prime

**A resurrection of the Farbrausch Werkkzeug procedural content pipeline.**

Between 2001 and 2011 the German demoscene group Farbrausch built a toolchain
that generated entire worlds from math: [.kkrieger](https://en.wikipedia.org/wiki/.kkrieger)
(a full FPS in a 96KB executable), *fr-041: debris* (a legendary demo in 180KB),
textures synthesized from generation history instead of stored pixels, meshes
grown from deformed boxes instead of asset files, and music played in real time
by the V2 software synthesizer.

They open-sourced everything in 2012 as [farbrausch/fr_public](https://github.com/farbrausch/fr_public)
with the note *"we'd just have to clean it up a bit first..."* — and never did.
The code was written for 32-bit MSVC, full of inline x86 assembly, and most of
it never built again.

**Prime picks up where they left off.** The goal: a modern, portable,
production-grade procedural content engine built on the original Farbrausch
technology — cleaned up, tested, and shipping.

## What works right now

Two engine libraries and two command line tools, building and running on
Linux/macOS/Windows with GCC, Clang or MSVC — no Windows-only code paths, no
inline assembly, no 32-bit assumptions:

| Component | Origin | Status |
|-----------|--------|--------|
| `engine/texture` | OpenKTG texture generator | builds, tested |
| `engine/synth` | V2 synthesizer + .v2m player | builds, tested, renders original tunes end-to-end on 64-bit |
| `tools/primetex` | new | generates showcase textures from pure math |
| `tools/primeplay` | new | renders .v2m modules to WAV offline |

The texture engine synthesizes noise, voronoi cells, normal maps, bump-lit
composites and more — from *zero* bytes of shipped pixel data. The synth engine
plays the actual music from the Farbrausch era: two original V2 tunes are
included under `assets/v2m/` (CC-BY, by Leonard "paniq" Ritter and
Melwyn+Little Bitchard).

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Requirements: CMake 3.16+, any C++17 compiler.

## Try it

```bash
# generate 5 procedural textures (TGA) into ./out
mkdir -p out
./build/tools/primetex/primetex -o out -s 512

# render an original Farbrausch-era V2 tune to WAV
./build/tools/primeplay/primeplay assets/v2m/pzero_new.v2m -o patient_zero.wav
```

## Repository layout

```
engine/
  texture/       procedural texture generator (from OpenKTG)
  synth/         V2 synthesizer core + .v2m module player (from v2)
tools/
  primetex/      texture generator CLI
  primeplay/     .v2m -> WAV renderer CLI
tests/           smoke tests (run via ctest)
assets/v2m/      sample V2 modules (CC-BY)
docs/            architecture notes and resurrection roadmap
licenses/        original upstream license texts
```

## The road ahead

This is phase one of the resurrection: the portable, deterministic core.
See [docs/ROADMAP.md](docs/ROADMAP.md) for the full plan — the Werkkzeug3
texture operator stacks (`w3texlib`), mesh generation, the operator-graph
document model, a real-time preview tool, and yes, the kkrieger game mode.

## Licensing

Prime is derived from code released by Farbrausch under BSD-style licenses,
the Artistic License 2.0, and the public domain. Original license texts are
preserved in [licenses/](licenses/), and every derived file keeps its original
copyright header. New code in this repository is released under the same terms
as the component it extends. Sample tunes in `assets/v2m/` are CC-BY 3.0 by
their original authors.

## Credits

Standing on the shoulders of: Fabian "ryg" Giesen, Tammo "kb" Hinrichs,
Dierk "Chaos" Ohlerich, Leonard "paniq" Ritter, and everyone else in the
[fr_public contributor list](https://github.com/farbrausch/fr_public#readme).
