#!/usr/bin/env bash
# Install Prime on Linux (Steam Deck / desktop).
# Run from inside the extracted Prime-*-linux-x86_64 folder.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
INSTALL_DIR="${PRIME_INSTALL_DIR:-$HOME/.local/share/Prime}"

echo "◈ Prime installer"
echo "  From: $HERE"
echo "  To:   $INSTALL_DIR"
echo ""

mkdir -p "$INSTALL_DIR"
# rsync if available, else cp
if command -v rsync >/dev/null 2>&1; then
  rsync -a --delete "$HERE/" "$INSTALL_DIR/"
else
  cp -a "$HERE/." "$INSTALL_DIR/"
fi

chmod +x "$INSTALL_DIR/bin/"* "$INSTALL_DIR/install.sh" 2>/dev/null || true

# Python environment for Prime Studio
echo "Setting up Python environment…"
if python3 -m venv "$INSTALL_DIR/.venv" 2>/dev/null; then
  "$INSTALL_DIR/.venv/bin/pip" install -q --upgrade pip
  "$INSTALL_DIR/.venv/bin/pip" install -q -r "$INSTALL_DIR/share/prime/studio/requirements.txt"
  echo "  Python venv ready."
else
  echo "  python3-venv not found — Prime Studio will bootstrap on first launch."
  echo "  (On Steam Deck this is pre-installed; on Ubuntu: sudo apt install python3-venv)"
fi

# Desktop entry
DESKTOP_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
mkdir -p "$DESKTOP_DIR"
sed "s|@INSTALL_DIR@|$INSTALL_DIR|g" "$INSTALL_DIR/share/applications/Prime.desktop" \
  > "$DESKTOP_DIR/Prime.desktop"
chmod +x "$DESKTOP_DIR/Prime.desktop"

# CLI symlinks
BIN_DIR="${HOME}/.local/bin"
mkdir -p "$BIN_DIR"
for tool in prime-studio prime-deck primegraph primeforge primetex primeplay; do
  ln -sf "$INSTALL_DIR/bin/$tool" "$BIN_DIR/$tool"
done

echo ""
echo "✓ Prime installed!"
echo ""
echo "  Launch Studio:  prime-studio"
echo "  CLI tools:      primegraph, primeforge, primetex, primeplay"
echo "  Or find 'Prime Studio' in your app menu (Desktop Mode)"
echo ""
echo "  Steam Deck tip: add $INSTALL_DIR/bin/prime-deck as a Non-Steam game"
echo "  Switch to Desktop Mode first, then return to Gaming Mode if you like."
echo ""
echo "  Free Llama:       export GROQ_API_KEY=gsk_...  (console.groq.com)"
echo "  Optional GPT:     export OPENAI_API_KEY=sk-...  before launching"
echo ""
