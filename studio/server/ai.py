"""AI prompt → Prime texture recipe.

Uses OpenAI when OPENAI_API_KEY is set; otherwise a deterministic
keyword/rule parser that still produces valid Werkkzeug-style recipes.
"""

from __future__ import annotations

import hashlib
import json
import os
import re
from dataclasses import dataclass, asdict
from typing import Any

import httpx


STYLES = ("noise", "voronoi", "metal", "panel", "organic", "ice", "lava", "rust")

DEFAULT_RECIPE = {
    "size": 512,
    "seed": 1,
    "style": "panel",
    "color_start": "0xff747d8e",
    "color_end": "0xfff1feff",
    "blur": 0.0074,
    "grid_rotation": 0.125,
    "bump_strength": 2.5,
    "light_x": -2.518,
    "light_y": 0.719,
    "light_z": -3.10,
    "voro_intensity_0": 37,
    "voro_count_0": 90,
    "voro_dist_0": 0.125,
    "voro_intensity_1": 42,
    "voro_count_1": 132,
    "voro_dist_1": 0.063,
    "voro_intensity_2": 37,
    "voro_count_2": 240,
    "voro_dist_2": 0.063,
    "voro_intensity_3": 37,
    "voro_count_3": 255,
    "voro_dist_3": 0.063,
}

PRESETS: dict[str, str] = {
    "kkrieger sci-fi grating": "dark brushed metal floor grating, kkrieger style sci-fi panel with diagonal grid bumps",
    "debris asteroid rock": "weathered grey asteroid rock surface with crystalline voronoi cracks, debris demo style",
    "molten lava flow": "glowing orange red molten lava with heat cracks and emissive veins",
    "frozen ice crystal": "pale blue frozen ice surface with sharp crystalline cell patterns",
    "rusted industrial": "oxidized rusty brown orange corroded industrial metal with heavy wear",
    "alien organic flesh": "wet organic alien flesh surface with purple green voronoi cells, biomechanical",
    "clean chrome metal": "smooth polished chrome steel brushed metal without grid pattern",
    "static noise field": "pure procedural grayscale noise texture with cool blue tint",
}


@dataclass
class AIResult:
    recipe: dict[str, Any]
    explanation: str
    mode: str  # "openai" | "rules"
    operators: list[dict[str, str]]


def _seed_from_prompt(prompt: str) -> int:
    h = hashlib.sha256(prompt.encode()).hexdigest()
    return int(h[:8], 16) % 2_000_000_000


def _clamp_recipe(raw: dict[str, Any], prompt: str) -> dict[str, Any]:
    r = dict(DEFAULT_RECIPE)
    r.update({k: v for k, v in raw.items() if k in DEFAULT_RECIPE or k.startswith("voro_")})
    r["seed"] = int(raw.get("seed", _seed_from_prompt(prompt)))
    size = int(raw.get("size", r["size"]))
    # snap to power of two
    valid = [16, 32, 64, 128, 256, 512, 1024, 2048]
    r["size"] = min(valid, key=lambda x: abs(x - size))
    style = str(raw.get("style", r["style"])).lower()
    r["style"] = style if style in STYLES else "panel"
    for key in ("color_start", "color_end"):
        val = str(r[key])
        if not val.startswith("0x"):
            r[key] = "0xff" + val.lstrip("#")
    return r


def _operators_for(style: str) -> list[dict[str, str]]:
    ops = [
        {"id": "noise", "label": "Band-limited Noise", "desc": "Perlin-style frequency synthesis"},
        {"id": "voronoi", "label": "Voronoi Cells", "desc": "Random cell centers → distance field"},
        {"id": "combine", "label": "Linear Combine", "desc": "Weighted sum of 4 voronoi layers"},
        {"id": "blur", "label": "Gaussian Blur", "desc": "Separable blur pass"},
        {"id": "colorize", "label": "Color Matrix", "desc": "Map grayscale → gradient"},
    ]
    if style in ("panel", "organic", "ice", "lava", "rust"):
        ops += [
            {"id": "glowrect", "label": "Glow Rect", "desc": "SDF rounded-rect grid pattern"},
            {"id": "derive", "label": "Derive Normals", "desc": "Height → tangent-space normal map"},
            {"id": "bump", "label": "Bump Light", "desc": "Phong-style bump mapping"},
            {"id": "paste", "label": "Paste Multiply", "desc": "Overlay grid detail"},
        ]
    return ops


def prompt_to_recipe_rules(prompt: str) -> AIResult:
    p = prompt.lower()
    raw: dict[str, Any] = {}
    notes: list[str] = []

    if any(w in p for w in ("lava", "magma", "fire", "molten", "ember")):
        raw["style"] = "lava"
        raw["color_start"] = "0xff8b1a00"
        raw["color_end"] = "0xffffaa33"
        raw["grid_rotation"] = 0.05
        notes.append("Lava palette with subtle heat cracks")
    elif any(w in p for w in ("ice", "frost", "frozen", "snow", "glacier")):
        raw["style"] = "ice"
        raw["color_start"] = "0xffa8c8e8"
        raw["color_end"] = "0xfff0f8ff"
        raw["blur"] = 0.004
        notes.append("Cool ice palette, tighter cells")
    elif any(w in p for w in ("rust", "oxid", "corrod", "weathered")):
        raw["style"] = "rust"
        raw["color_start"] = "0xff5c3a1e"
        raw["color_end"] = "0xffc87840"
        raw["blur"] = 0.012
        notes.append("Oxidized rust tones")
    elif any(w in p for w in ("organic", "flesh", "alien", "biomech", "slime")):
        raw["style"] = "organic"
        raw["color_start"] = "0xff3a2848"
        raw["color_end"] = "0xff88cc66"
        raw["grid_rotation"] = 0.2
        notes.append("Organic biomechanical palette")
    elif any(w in p for w in ("noise", "static", "grain", "perlin")):
        raw["style"] = "noise"
        notes.append("Detected noise/static keywords")
    elif any(w in p for w in ("voronoi", "cell", "crack", "crystal")):
        raw["style"] = "voronoi"
        notes.append("Detected cellular/voronoi keywords")
    elif any(w in p for w in ("kkrieger", "sci-fi", "grating", "panel", "floor", "grid", "diagonal")):
        raw["style"] = "panel"
        notes.append("Classic kkrieger-style sci-fi panel")
    elif any(w in p for w in ("chrome", "steel", "brushed", "smooth metal", "clean metal")):
        raw["style"] = "metal"
        raw["color_start"] = "0xff606068"
        raw["color_end"] = "0xffd0d0d8"
        notes.append("Smooth metal — skipping grid bump pass")
    else:
        raw["style"] = "panel"
        notes.append("Default panel pipeline")

    if "dark" in p:
        notes.append("Darkened palette")
        cs = raw.get("color_start", DEFAULT_RECIPE["color_start"])
        raw["color_start"] = cs  # keep style-specific unless generic

    if any(w in p for w in ("bright", "glow", "emissive")):
        raw["light_z"] = -2.0
        notes.append("Brighter lighting")

    if "large" in p or "4k" in p:
        raw["size"] = 1024
    elif "small" in p or "thumbnail" in p:
        raw["size"] = 256

    recipe = _clamp_recipe(raw, prompt)
    return AIResult(
        recipe=recipe,
        explanation="Rule engine: " + "; ".join(notes),
        mode="rules",
        operators=_operators_for(recipe["style"]),
    )


async def prompt_to_recipe_openai(prompt: str) -> AIResult | None:
    api_key = os.environ.get("OPENAI_API_KEY")
    if not api_key:
        return None

    system = (
        "You are the AI layer of Prime, a procedural texture engine descended from "
        "Farbrausch Werkkzeug3 / OpenKTG. Given a natural-language material description, "
        "output ONLY valid JSON with these keys: style (one of "
        + ", ".join(STYLES)
        + "), color_start and color_end as 0xaarrggbb hex strings, "
        "optional size (256/512/1024), blur (0.002-0.02), grid_rotation (0-0.5), "
        "bump_strength (1-4), light_x/y/z floats, and explanation (one sentence). "
        "Pick style and colors that match the prompt. No markdown."
    )

    try:
        async with httpx.AsyncClient(timeout=30.0) as client:
            resp = await client.post(
                "https://api.openai.com/v1/chat/completions",
                headers={"Authorization": f"Bearer {api_key}"},
                json={
                    "model": os.environ.get("OPENAI_MODEL", "gpt-4o-mini"),
                    "messages": [
                        {"role": "system", "content": system},
                        {"role": "user", "content": prompt},
                    ],
                    "temperature": 0.7,
                    "response_format": {"type": "json_object"},
                },
            )
            resp.raise_for_status()
            content = resp.json()["choices"][0]["message"]["content"]
            data = json.loads(content)
    except Exception:
        return None

    explanation = str(data.pop("explanation", "Generated by OpenAI"))
    recipe = _clamp_recipe(data, prompt)
    return AIResult(
        recipe=recipe,
        explanation=explanation,
        mode="openai",
        operators=_operators_for(recipe["style"]),
    )


async def prompt_to_recipe(prompt: str) -> AIResult:
    prompt = prompt.strip()
    if not prompt:
        prompt = "kkrieger sci-fi metal panel"

    ai = await prompt_to_recipe_openai(prompt)
    if ai:
        return ai
    return prompt_to_recipe_rules(prompt)


def recipe_to_file_lines(recipe: dict[str, Any]) -> str:
  lines = [f"{k}={v}" for k, v in recipe.items()]
  return "\n".join(lines) + "\n"
