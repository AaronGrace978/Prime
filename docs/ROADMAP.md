# Resurrection roadmap

The plan, phase by phase. Each phase produces something that builds, runs and
is tested — no big-bang rewrites.

## Phase 1 — Portable core (shipped)

- [x] Vendor OpenKTG texture generator, build as `prime_texture`
- [x] Vendor V2 synthesizer + .v2m player, port off 32-bit MSVC
- [x] `primetex` / `primeplay` CLI tools
- [x] Smoke tests + CI

## Phase 1.5 — AI layer (shipped)

- [x] Parameterized texture recipes (`engine/texture/recipe.*`)
- [x] `primeforge` — recipe-driven single-texture generator
- [x] **Prime Studio** — web UI + FastAPI
  - Natural language → operator recipe → C++ engine → live preview
  - Groq Llama when `GROQ_API_KEY` is set (free tier); OpenAI or custom LLM optional
  - Operator graph visualization + machine-readable recipe export

## Phase 2 — The Werkkzeug3 texture pipeline

`fr_public/werkkzeug3/w3texlib` is the *actual* texture library used for
fr-033 and close kin to what kkrieger used — richer than OpenKTG (FRIED image
codec, DXT compression, vector rasterizer) but written against the wz3 base
layer.

- [ ] Port `w3texlib` (genbitmap, genvector, rygdxt) to the Prime build
- [ ] Golden-image tests: fixed op-chains with checksummed outputs
- [ ] Windows and macOS CI legs

## Phase 3 — Operator graphs (the "Werkkzeug" part)

The heart of the tool: documents are graphs of operators with parameters,
evaluated lazily with caching. This is what turned artists loose on 96KB.

- [x] Operator/document model (`graph.hpp` — `.pgraph` serialization)
- [x] Texture operator set on top of `engine/texture` (13 op types)
- [x] Deterministic graph evaluator with memoization
- [x] `primegraph` — headless render from graph or recipe files
- [x] Prime Studio: real evaluatable graphs + AI mutation buttons
- [ ] Full `kdoc` editor UI with per-node parameter editing
- [ ] Lazy graph evaluator with disk cache

## Phase 4 — Meshes and materials

- [ ] Port `genminmesh`/`genmesh` mesh generators (boxes + catmull-clark
      subdivision + deformers = kkrieger geometry)
- [ ] Export to glTF so outputs plug into any modern pipeline
- [ ] Material/shader model on a modern GPU API

## Phase 5 — Real-time

- [ ] Real-time audio output for the V2 player (miniaudio or similar)
- [ ] Preview renderer for textures/meshes
- [ ] Editor UI: page/op editing in the spirit of wz3, on a modern stack

## Phase 6 — The moonshot

- [ ] Revive the kkrieger game mode from `werkkzeug3_kkrieger`
      ("might work" — upstream README, 2012)

## Deliberately out of scope

- `lekktor/` — the dead-code-elimination instrumenter. Upstream warning:
  "May summon Eldritch Abominations. Handle with care." It rewrote C++ by
  regex-adjacent parsing to strip unused code paths for 96KB builds. Modern
  LTO + `--gc-sections` does this job now. The abominations stay summoned
  elsewhere.
- `wz_mobile/` — "Never got used for anything." It stays that way.
- kkrunchy — the executable packer is a separate (working) project and only
  matters for the size-coding sport, not for the engine.
