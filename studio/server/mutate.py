"""AI-driven recipe mutations — evolve materials without full regeneration."""

from __future__ import annotations

import random
from dataclasses import dataclass
from typing import Any


MUTATIONS: dict[str, dict[str, Any]] = {
    "darker": {
        "light_z": -1.8,
        "bump_strength": "*1.1",
        "explanation": "Deepened shadows and strengthened bump contrast",
    },
    "brighter": {
        "light_z": -4.5,
        "bump_strength": "*0.9",
        "explanation": "Lifted key light and softened relief",
    },
    "sharper": {
        "blur": "*0.5",
        "bump_strength": "*1.25",
        "explanation": "Reduced blur, crisper surface detail",
    },
    "softer": {
        "blur": "*1.8",
        "bump_strength": "*0.8",
        "explanation": "Increased blur for softer, worn surfaces",
    },
    "more_grid": {
        "grid_rotation": "+0.03",
        "bump_strength": "*1.15",
        "explanation": "Emphasized grid rotation and bump depth",
    },
    "less_grid": {
        "bump_strength": "*0.65",
        "explanation": "Flattened grid relief toward smooth metal",
    },
    "menacing": {
        "color_start": "0xff1a0a14",
        "color_end": "0xff4a2030",
        "light_x": -3.5,
        "light_z": -2.2,
        "bump_strength": "*1.3",
        "explanation": "Blood-dark palette, harsh side lighting, aggressive relief",
    },
    "organic": {
        "style": "organic",
        "color_start": "0xff2a1838",
        "color_end": "0xff70cc55",
        "grid_rotation": 0.22,
        "blur": 0.009,
        "explanation": "Shifted to organic biomechanical palette and looser cells",
    },
    "crystalline": {
        "style": "ice",
        "color_start": "0xff90b8e0",
        "color_end": "0xfff4fcff",
        "blur": 0.003,
        "voro_count_0": 140,
        "explanation": "Ice-crystal palette with tighter voronoi structure",
    },
    "chaos": {
        "seed": "random",
        "grid_rotation": "random",
        "explanation": "Randomized seed and grid angle — same graph, new universe",
    },
}


@dataclass
class MutationResult:
    recipe: dict[str, Any]
    explanation: str
    mutation: str


def _apply_delta(value: Any, delta: Any) -> Any:
    if isinstance(delta, str):
        if delta == "random":
            if isinstance(value, int):
                return random.randint(1, 2_000_000_000)
            return value + random.uniform(-0.15, 0.15)
        if delta.startswith("*"):
            return type(value)(float(value) * float(delta[1:]))
        if delta.startswith("+") or delta.startswith("-"):
            return type(value)(float(value) + float(delta))
    return delta


def mutate_recipe(recipe: dict[str, Any], mutation: str) -> MutationResult:
    """Apply a named mutation to a recipe dict. Returns updated recipe."""
    mutation = mutation.lower().strip()
    if mutation not in MUTATIONS:
        available = ", ".join(sorted(MUTATIONS))
        raise ValueError(f"Unknown mutation '{mutation}'. Try: {available}")

    spec = MUTATIONS[mutation]
    out = dict(recipe)
    explanation = str(spec.get("explanation", f"Applied {mutation}"))

    for key, delta in spec.items():
        if key == "explanation":
            continue
        if key == "seed" and delta == "random":
            out["seed"] = random.randint(1, 2_000_000_000)
        elif key in out and isinstance(delta, str) and (delta.startswith("*") or delta[0] in "+-"):
            out[key] = _apply_delta(out[key], delta)
        elif key == "grid_rotation" and delta == "random":
            out[key] = round(random.uniform(0.02, 0.45), 4)
        else:
            out[key] = delta

    # clamp
    if "blur" in out:
        out["blur"] = max(0.001, min(0.03, float(out["blur"])))
    if "bump_strength" in out:
        out["bump_strength"] = max(0.5, min(5.0, float(out["bump_strength"])))

    return MutationResult(recipe=out, explanation=explanation, mutation=mutation)


def suggest_mutations(recipe: dict[str, Any]) -> list[dict[str, str]]:
    """Return mutation buttons appropriate for the current recipe."""
    style = recipe.get("style", "panel")
    base = [
        {"id": "darker", "label": "Darker", "icon": "🌑"},
        {"id": "brighter", "label": "Brighter", "icon": "☀️"},
        {"id": "sharper", "label": "Sharper", "icon": "◆"},
        {"id": "softer", "label": "Softer", "icon": "○"},
        {"id": "chaos", "label": "Chaos Seed", "icon": "🎲"},
    ]
    if style in ("panel", "organic", "ice", "lava", "rust"):
        base.insert(2, {"id": "more_grid", "label": "More Grid", "icon": "▦"})
        base.insert(3, {"id": "less_grid", "label": "Less Grid", "icon": "▢"})
        base.append({"id": "menacing", "label": "Menacing", "icon": "☠️"})
    if style != "organic":
        base.append({"id": "organic", "label": "Go Organic", "icon": "🫀"})
    if style != "ice":
        base.append({"id": "crystalline", "label": "Crystalline", "icon": "❄️"})
    return base
