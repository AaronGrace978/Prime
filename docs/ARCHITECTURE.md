# Prime architecture

## Design principles

1. **Everything from math.** The engine ships generators, not assets. A texture
   is a short program (a chain of operators), not a grid of pixels. This is the
   core idea that made 96KB games possible, and it is preserved everywhere.
2. **Deterministic.** Same inputs, same seed, same output — on every platform.
   No wall-clock, no filesystem state, no platform-dependent float paths in the
   generators.
3. **Portable core, thin platform shells.** Engine libraries are pure C++17
   with zero platform dependencies. Anything OS-specific (audio output, window,
   GPU) lives in tools, behind small interfaces.
4. **Honest resurrection.** Original algorithms and behaviors are preserved,
   including deliberately kept original quirks (e.g. the V2 synth's
   `BUG_V2_FM_RANGE` flag that reproduces the historical FM oscillator sound).
   Modernization changes *how* the code builds, not *what* it computes.

## Components

### engine/texture — procedural texture generator

Derived from **OpenKTG** (`fr_public/ktg`), Fabian Giesen's 2007 clean-room
redesign of the Werkkzeug texture generation subset. Public domain.

- 16 bits per color channel throughout the pipeline (the original insight:
  8-bit intermediates destroy gradients after a few operations).
- Operators: band-limited noise (perlin-style, derivative of the actual
  kkrieger noise), voronoi/cell patterns, glow rects, linear combines, blurs,
  color/coordinate matrix transforms, normal-map derivation, bump lighting,
  rotozoom paste with combine modes.
- A texture is produced by chaining operators; the demo tool `primetex` shows
  a full material built this way (base metal -> grid normal map -> bump-lit
  panel).

Upstream state: this was the cleanest code in fr_public and needed no
functional changes — only a build system.

### engine/synth — V2 synthesizer system

Derived from **V2** (`fr_public/v2`), Tammo Hinrichs' software synthesizer
used in *every* Farbrausch intro, kkrieger and debris. BSD / Artistic 2.0.

- `synth_core.cpp` — the full synth: 3-oscillator voices, dual filters,
  modulation matrix, LFOs, envelopes, distortion, chorus/flanger, compressor,
  reverb, stereo delay. This is ryg's portable C++ rewrite of the original
  x86 assembly (`synth.asm`), which upstream had already verified against the
  assembly version.
- `ronan.cpp` — "Ronan", the formant-filter speech synthesizer that sings in
  several Farbrausch productions. Enabled via the `RONAN` compile definition
  (on by default).
- `v2mplayer.cpp` — the .v2m module player: decodes the delta-compressed event
  stream and drives the synth via its MIDI interface.
- `v2mconv.cpp` + `sounddef.cpp` — converts .v2m files from older format
  versions to the current one, so tunes from any era load.

Porting notes (see git history for exact diffs):

- All inline x86 assembly replaced with portable C++ (64-bit mul/div in the
  player timing code, x87 `pow`/`exp`/`ftol` helpers in Ronan, one `rep stosd`
  that is now `memset`).
- Type headers fixed for LP64: `sU32` was `unsigned long` (8 bytes on 64-bit
  Linux); everything now uses `<stdint.h>` exact-width types. This matters
  because the .v2m loader reads the binary format through these types.
- `__stdcall`/`__cdecl`/`__declspec` stubbed out on non-Windows platforms.
- `sounddef.cpp` trimmed to the parameter tables + version bookkeeping the
  converter needs; the original file's Windows editor/clipboard/file-I/O code
  was dropped, not ported.
- `V2MPlayer::Open` now verifies that the synth state fits its internal
  buffer (struct sizes grow on 64-bit).

### tools

- **primetex** — generates a showcase texture set (noise, voronoi, composite
  material, normal map, bump-lit panel) as .tga files. Options: output dir,
  size, seed.
- **primeplay** — offline-renders a .v2m to a 16-bit stereo WAV at 44.1-192
  kHz, converting old-format files automatically. This is the exact audio
  path of the original intros, minus the DirectSound output.

### tests

`tests/` contains a smoke test binary (run via `ctest`) that:

- generates textures in memory and checks statistical properties (noise is
  non-constant, derived normal maps point outward), and
- renders 5 seconds of an original tune and checks the output is sane
  (no NaNs, no silence, plausible energy).

The two CLI tools also run as tests with real inputs.

## Layering rules

```
tools/*        -> engine/*   (allowed)
tests/*        -> engine/*   (allowed)
engine/texture -> (nothing)
engine/synth   -> (nothing)
```

The two engines intentionally do not share their legacy type headers
(`engine/texture/types.hpp` vs `engine/synth/types.h`); they define clashing
`sInt`/`sBool` families. Unifying them is a roadmap item — until then, one
translation unit must not include both engines' headers.
