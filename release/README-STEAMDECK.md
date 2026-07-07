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

Your browser opens to the AI workbench. Describe a material, hit Generate.

## Add to Steam (optional)

1. Switch to **Desktop Mode** (Power → Switch to Desktop).
2. Open **Steam** → **Games** → **Add a Non-Steam Game to My Library**.
3. Browse to `~/.local/share/Prime/bin/prime-deck` and add it.
4. Rename to **Prime Studio** in your library if you like.

Launching from Steam starts the local server and opens the UI.

## What's included

| Binary | What it does |
|--------|--------------|
| `prime-studio` | AI workbench (web UI on port 8787) |
| `primeforge` | Recipe → texture (C++ engine) |
| `primetex` | Showcase texture batch |
| `primeplay` | Render .v2m tunes to WAV |

## Requirements

- Steam Deck / Linux **x86_64** (amd64)
- `python3` + `venv` (pre-installed on SteamOS 3.x)
- A browser (Firefox in Desktop Mode)

## GPT mode (optional)

```bash
export OPENAI_API_KEY=sk-your-key
prime-studio
```

Without a key, the built-in rules AI still generates textures.

## Troubleshooting

**Port in use:** Prime auto-picks the next free port. Check the terminal output for the URL.

**Python errors on first run:** Run `~/.local/share/Prime/install.sh` again, or:
```bash
~/.local/share/Prime/.venv/bin/pip install -r ~/.local/share/Prime/share/prime/studio/requirements.txt
```

**Read-only filesystem on Deck:** Install to `~/` (default). Don't install to system paths.

## Uninstall

```bash
rm -rf ~/.local/share/Prime ~/.local/bin/prime-*
rm ~/.local/share/applications/Prime.desktop
```
