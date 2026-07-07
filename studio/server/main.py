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
from .engine import FORGE_BIN, forge_available, render_texture

ROOT = Path(__file__).resolve().parents[2]
WEB = Path(__file__).resolve().parents[1] / "web"
OUTPUT = Path(os.environ.get("PRIME_STUDIO_OUTPUT", "/tmp/prime-studio"))
OUTPUT.mkdir(parents=True, exist_ok=True)

app = FastAPI(title="Prime Studio", version="0.2.0")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


class GenerateRequest(BaseModel):
    prompt: str = Field(..., min_length=1, max_length=2000)
    size: int | None = None


class GenerateResponse(BaseModel):
    id: str
    prompt: str
    recipe: dict
    explanation: str
    mode: str
    operators: list[dict]
    image_url: str
    width: int
    height: int
    ms: int


@app.get("/api/status")
def status():
    return {
        "engine": "Prime / Werkkzeug resurrection",
        "forge": forge_available(),
        "forge_bin": FORGE_BIN,
        "ai_openai": bool(os.environ.get("OPENAI_API_KEY")),
        "ai_fallback": "rules",
        "presets": list(PRESETS.keys()),
    }


@app.get("/api/presets")
def presets():
    return PRESETS


@app.post("/api/generate", response_model=GenerateResponse)
async def generate(req: GenerateRequest):
    if not forge_available():
        raise HTTPException(
            503,
            detail="primeforge binary not built. Run: cmake -S . -B build && cmake --build build",
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

    ms = int((time.perf_counter() - t0) * 1000)
    return GenerateResponse(
        id=job_id,
        prompt=req.prompt,
        recipe=recipe,
        explanation=ai.explanation,
        mode=ai.mode,
        operators=ai.operators,
        image_url=f"/api/image/{job_id}.png",
        width=meta["width"],
        height=meta["height"],
        ms=ms,
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
