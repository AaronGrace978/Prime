"""Bridge between Prime Studio and the C++ primeforge binary."""

from __future__ import annotations

import os
import subprocess
import tempfile
from pathlib import Path

from PIL import Image

from .ai import recipe_to_file_lines


ROOT = Path(__file__).resolve().parents[2]
FORGE_BIN = os.environ.get(
    "PRIME_FORGE_BIN",
    str(ROOT / "build" / "tools" / "primeforge" / "primeforge"),
)


def forge_available() -> bool:
    return Path(FORGE_BIN).is_file()


def render_texture(recipe: dict, out_png: Path) -> dict:
    """Run primeforge, convert TGA → PNG. Returns timing/size metadata."""
    if not forge_available():
        raise FileNotFoundError(
            f"primeforge not found at {FORGE_BIN}. Build with: cmake --build build"
        )

    with tempfile.TemporaryDirectory(prefix="prime_") as tmp:
        tmpdir = Path(tmp)
        recipe_path = tmpdir / "recipe.txt"
        tga_path = tmpdir / "out.tga"
        recipe_path.write_text(recipe_to_file_lines(recipe))

        proc = subprocess.run(
            [FORGE_BIN, "-r", str(recipe_path), "-o", str(tga_path)],
            capture_output=True,
            text=True,
            check=False,
        )
        if proc.returncode != 0:
            raise RuntimeError(proc.stderr or proc.stdout or "primeforge failed")

        img = Image.open(tga_path)
        img = img.convert("RGBA")
        out_png.parent.mkdir(parents=True, exist_ok=True)
        img.save(out_png, format="PNG", optimize=True)

        return {
            "width": img.width,
            "height": img.height,
            "bytes": out_png.stat().st_size,
            "forge_log": proc.stdout.strip(),
        }
