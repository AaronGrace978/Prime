# Prime Studio — AI layer

Prime Studio is the AI-powered workbench on top of the procedural engine.
You describe a material in natural language; the AI turns that into a
**recipe** (key=value parameters); the C++ `primeforge` binary executes
the real OpenKTG operator chain and returns a texture.

```
"molten lava with glowing cracks"
        │
        ▼
   ┌─────────┐     OPENAI_API_KEY set?
   │ AI layer │ ──► GPT-4o-mini → JSON recipe
   └─────────┘     else → rules engine (keyword parser)
        │
        ▼
   recipe.txt  (size, style, colors, voronoi params, lighting…)
        │
        ▼
   primeforge  (C++ / OpenKTG operators)
        │
        ▼
   output.png
```

## Launch

```bash
# one-shot (builds primeforge if needed)
./studio/launch.sh

# open http://127.0.0.1:8787
```

## AI modes

| Mode | When | What it does |
|------|------|--------------|
| **GPT** | `OPENAI_API_KEY` is set | Sends your prompt to OpenAI; model returns a JSON recipe tuned to your description |
| **Rules** | always available as fallback | Keyword parser maps material words → style presets (lava, ice, rust, panel, …) |

```bash
export OPENAI_API_KEY=sk-...
export OPENAI_MODEL=gpt-4o-mini   # optional, this is the default
./studio/launch.sh
```

Without an API key the rules engine still works and is fully deterministic.

## Recipe file format

`primeforge -r recipe.txt -o out.tga` reads simple `key=value` lines:

```
size=512
seed=42
style=lava
color_start=0xff8b1a00
color_end=0xffffaa33
blur=0.0074
grid_rotation=0.05
bump_strength=2.5
light_x=-2.518
light_y=0.719
light_z=-3.10
voro_intensity_0=37
voro_count_0=90
voro_dist_0=0.125
```

### Styles

| style | pipeline |
|-------|----------|
| `noise` | band-limited noise → colorize |
| `voronoi` | single voronoi layer → colorize |
| `metal` | 4× voronoi combine → blur → noise → colorize |
| `panel` | metal base → glow-rect grid → normal derive → bump → multiply overlay |
| `organic` / `ice` / `lava` / `rust` | panel pipeline with style-specific default colors |

## HTTP API

```
GET  /api/status
GET  /api/presets
POST /api/generate   { "prompt": "...", "size": 512 }
GET  /api/image/{id}.png
```

## Architecture note

The AI layer never touches pixels. It only produces the **generation history**
(parameters for the operator graph). The actual math runs in the same C++ code
that powers `primetex` and will eventually power the full Werkkzeug operator
graph. This is the Werkkzeug philosophy: textures from history, not from files.
