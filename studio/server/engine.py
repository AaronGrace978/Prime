"""Bridge between Prime Studio and the C++ primegraph / primeforge binaries."""

from __future__ import annotations

import os
import subprocess
import tempfile
from pathlib import Path

from PIL import Image

from .ai import recipe_to_file_lines
from .graph import graph_to_pgraph, recipe_to_graph


ROOT = Path(__file__).resolve().parents[2]
GRAPH_BIN = os.environ.get(
    "PRIME_GRAPH_BIN",
    str(ROOT / "build" / "tools" / "primegraph" / "primegraph"),
)
FORGE_BIN = os.environ.get(
    "PRIME_FORGE_BIN",
    str(ROOT / "build" / "tools" / "primeforge" / "primeforge"),
)


def graph_available() -> bool:
    return Path(GRAPH_BIN).is_file()


def forge_available() -> bool:
    return Path(FORGE_BIN).is_file() or graph_available()


def render_texture(recipe: dict, out_png: Path) -> dict:
    """Render recipe via primegraph (preferred) or primeforge fallback."""
    if not forge_available():
        raise FileNotFoundError(
            f"primegraph/primeforge not found. Build with: cmake --build build"
        )

    with tempfile.TemporaryDirectory(prefix="prime_") as tmp:
        tmpdir = Path(tmp)
        recipe_path = tmpdir / "recipe.txt"
        tga_path = tmpdir / "out.tga"
        recipe_path.write_text(recipe_to_file_lines(recipe))

        if graph_available():
            graph_path = tmpdir / "graph.pgraph"
            graph = recipe_to_graph(recipe)
            graph_path.write_text(graph_to_pgraph(graph))
            proc = subprocess.run(
                [GRAPH_BIN, "-g", str(graph_path), "-o", str(tga_path)],
                capture_output=True,
                text=True,
                check=False,
            )
            renderer = "primegraph"
        else:
            proc = subprocess.run(
                [FORGE_BIN, "-r", str(recipe_path), "-o", str(tga_path)],
                capture_output=True,
                text=True,
                check=False,
            )
            renderer = "primeforge"

        if proc.returncode != 0:
            raise RuntimeError(proc.stderr or proc.stdout or f"{renderer} failed")

        img = Image.open(tga_path)
        img = img.convert("RGBA")
        out_png.parent.mkdir(parents=True, exist_ok=True)
        img.save(out_png, format="PNG", optimize=True)

        return {
            "width": img.width,
            "height": img.height,
            "bytes": out_png.stat().st_size,
            "renderer": renderer,
            "forge_log": proc.stdout.strip(),
        }
