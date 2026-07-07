"""Prime Studio — AI-powered procedural content workbench."""

from __future__ import annotations

import os
import time
import uuid
from pathlib import Path

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel, Field

from .ai import PRESETS, prompt_to_recipe
from .engine import FORGE_BIN, GRAPH_BIN, forge_available, graph_available, render_texture
from .graph import graph_for_display, recipe_to_graph
from .mutate import mutate_recipe, suggest_mutations

ROOT = Path(__file__).resolve().parents[2]
WEB = Path(__file__).resolve().parents[1] / "web"
OUTPUT = Path(os.environ.get("PRIME_STUDIO_OUTPUT", "/tmp/prime-studio"))
OUTPUT.mkdir(parents=True, exist_ok=True)

app = FastAPI(title="Prime Studio", version="0.3.0")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


class GenerateRequest(BaseModel):
    prompt: str = Field(..., min_length=1, max_length=2000)
    size: int | None = None


class MutateRequest(BaseModel):
    recipe: dict
    mutation: str = Field(..., min_length=1, max_length=64)


class GenerateResponse(BaseModel):
    id: str
    prompt: str
    recipe: dict
    explanation: str
    mode: str
    graph: dict
    operators: list[dict]
    mutations: list[dict]
    image_url: str
    width: int
    height: int
    ms: int
    renderer: str


@app.get("/api/status")
def status():
    return {
        "engine": "Prime / Werkkzeug resurrection",
        "forge": forge_available(),
        "graph": graph_available(),
        "forge_bin": FORGE_BIN,
        "graph_bin": GRAPH_BIN,
        "ai_openai": bool(os.environ.get("OPENAI_API_KEY")),
        "ai_fallback": "rules",
        "presets": list(PRESETS.keys()),
        "version": "0.3.0",
    }


@app.get("/api/presets")
def presets():
    return PRESETS


@app.post("/api/generate", response_model=GenerateResponse)
async def generate(req: GenerateRequest):
    if not forge_available():
        raise HTTPException(
            503,
            detail="primegraph binary not built. Run: cmake -S . -B build && cmake --build build",
        )

    t0 = time.perf_counter()
    ai = await prompt_to_recipe(req.prompt)
    recipe = dict(ai.recipe)
    if req.size:
        recipe["size"] = req.size

    job_id = uuid.uuid4().hex[:12]
    png_path = OUTPUT / f"{job_id}.png"

    try:
        meta = render_texture(recipe, png_path)
    except Exception as e:
        raise HTTPException(500, detail=str(e)) from e

    graph = recipe_to_graph(recipe)
    ms = int((time.perf_counter() - t0) * 1000)
    return GenerateResponse(
        id=job_id,
        prompt=req.prompt,
        recipe=recipe,
        explanation=ai.explanation,
        mode=ai.mode,
        graph=graph,
        operators=graph_for_display(graph),
        mutations=suggest_mutations(recipe),
        image_url=f"/api/image/{job_id}.png",
        width=meta["width"],
        height=meta["height"],
        ms=ms,
        renderer=meta.get("renderer", "primeforge"),
    )


@app.post("/api/mutate", response_model=GenerateResponse)
async def mutate(req: MutateRequest):
    if not forge_available():
        raise HTTPException(503, detail="primegraph binary not built")

    t0 = time.perf_counter()
    try:
        result = mutate_recipe(req.recipe, req.mutation)
    except ValueError as e:
        raise HTTPException(400, detail=str(e)) from e

    recipe = result.recipe
    job_id = uuid.uuid4().hex[:12]
    png_path = OUTPUT / f"{job_id}.png"

    try:
        meta = render_texture(recipe, png_path)
    except Exception as e:
        raise HTTPException(500, detail=str(e)) from e

    graph = recipe_to_graph(recipe)
    ms = int((time.perf_counter() - t0) * 1000)
    return GenerateResponse(
        id=job_id,
        prompt=f"mutation:{result.mutation}",
        recipe=recipe,
        explanation=result.explanation,
        mode="mutation",
        graph=graph,
        operators=graph_for_display(graph),
        mutations=suggest_mutations(recipe),
        image_url=f"/api/image/{job_id}.png",
        width=meta["width"],
        height=meta["height"],
        ms=ms,
        renderer=meta.get("renderer", "primeforge"),
    )


@app.get("/api/image/{filename}")
def image(filename: str):
    path = OUTPUT / filename
    if not path.is_file() or path.suffix != ".png":
        raise HTTPException(404)
    return FileResponse(path, media_type="image/png")


# static UI last
if WEB.is_dir():
    app.mount("/", StaticFiles(directory=str(WEB), html=True), name="web")
