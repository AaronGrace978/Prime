# Prime on Steam Deck (Linux x86_64)

This build runs natively on Steam Deck in **Desktop Mode**. No Proton needed.

## Quick install

1. Download `Prime-*-linux-x86_64.tar.gz` from [Releases](https://github.com/AaronGrace978/Prime/releases).
2. Extract anywhere (Downloads is fine):
   ```bash
   tar -xzf Prime-*-linux-x86_64.tar.gz
   cd Prime-*-linux-x86_64
   ./install.sh
   ```
3. Launch:
   ```bash
   prime-studio
   ```
   Or open **Prime Studio** from the application menu.

Your browser opens to the AI workbench. Describe a material, hit Generate, then use the **mutation buttons** (Darker, Menacing, Chaos Seed, etc.) to evolve it live.

## Add to Steam (optional)

1. Switch to **Desktop Mode** (Power → Switch to Desktop).
2. Open **Steam** → **Games** → **Add a Non-Steam Game to My Library**.
3. Browse to `~/.local/share/Prime/bin/prime-deck` and add it.
4. Rename to **Prime Studio** in your library if you like.

Launching from Steam starts the local server and opens the UI.

## What's included

| Binary | What it does |
|--------|--------------|
| `prime-studio` | AI workbench — operator graphs + live mutations (web UI on port 8787) |
| `primegraph` | Operator graph → texture (C++ engine, preferred render path) |
| `primeforge` | Recipe → texture (C++ engine, fallback) |
| `primetex` | Showcase texture batch |
| `primeplay` | Render .v2m tunes to WAV |

## Requirements

- Steam Deck / Linux **x86_64** (amd64)
- `python3` + `venv` (pre-installed on SteamOS 3.x)
- A browser (Firefox in Desktop Mode)

## GPT / Llama mode (optional)

**Free Llama cloud (recommended)** — Groq, no credit card:

```bash
# Get a free key at https://console.groq.com
export GROQ_API_KEY=gsk_your_key
prime-studio
```

Optional model override (default: `llama-3.3-70b-versatile`):

```bash
export GROQ_MODEL=llama-3.1-8b-instant   # faster, more free requests/day
```

**OpenAI** (paid):

```bash
export OPENAI_API_KEY=sk-your-key
prime-studio
```

Without any API key, the built-in rules AI still generates textures.

## Headless CLI (optional)

From any terminal after install:

```bash
primegraph -r myrecipe.txt -o panel.tga --save-graph panel.pgraph
primeplay assets/v2m/tune.v2m -o tune.wav -t 30
```

## Troubleshooting

**Port in use:** Prime auto-picks the next free port. Check the terminal output for the URL.

**Python errors on first run:** Run `~/.local/share/Prime/install.sh` again, or:
```bash
~/.local/share/Prime/.venv/bin/pip install -r ~/.local/share/Prime/share/prime/studio/requirements.txt
```

**Engine offline in Studio:** Re-run `./install.sh` from the extracted folder, or rebuild from source:
```bash
cmake -S . -B build && cmake --build build
```

**Read-only filesystem on Deck:** Install to `~/` (default). Don't install to system paths.

## Uninstall

```bash
rm -rf ~/.local/share/Prime ~/.local/bin/prime-*
rm ~/.local/share/applications/Prime.desktop
```
