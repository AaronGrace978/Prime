#!/usr/bin/env bash
# Launch Prime Studio — AI-powered procedural workbench
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# Build C++ engine if needed
FORGE="$ROOT/build/tools/primeforge/primeforge"
if [[ ! -x "$FORGE" ]]; then
  echo "Building primeforge…"
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --target primeforge -j"$(nproc)"
fi

# Python deps
if ! python3 -c "import fastapi" 2>/dev/null; then
  echo "Installing studio dependencies…"
  pip install -q -r studio/requirements.txt
fi

export PRIME_FORGE_BIN="$FORGE"
export PYTHONPATH="$ROOT/studio:${PYTHONPATH:-}"

PORT="${PRIME_STUDIO_PORT:-8787}"
echo ""
echo "  ◈ Prime Studio"
echo "  AI × Werkkzeug procedural engine"
echo "  → http://127.0.0.1:${PORT}"
echo ""
echo "  Set OPENAI_API_KEY for GPT-powered material design."
echo "  Without it, the rules engine still works."
echo ""

exec python3 -m uvicorn server.main:app --host 0.0.0.0 --port "$PORT" --app-dir studio
