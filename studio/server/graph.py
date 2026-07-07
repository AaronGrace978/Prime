"""Recipe → operator graph conversion for Prime Studio.

The C++ engine evaluates graphs via primegraph; this module builds the
same graph structure in JSON for the interactive Studio UI.
"""

from __future__ import annotations

from typing import Any

OP_LABELS: dict[str, tuple[str, str]] = {
    "gradient": ("Gradient", "2×1 color ramp"),
    "noise": ("Band-limited Noise", "Perlin-style frequency synthesis"),
    "voronoi": ("Voronoi Cells", "Random cell centers → distance field"),
    "combine": ("Linear Combine", "Weighted sum of voronoi layers"),
    "blur": ("Gaussian Blur", "Separable blur pass"),
    "colorize": ("Color Matrix", "Map grayscale → gradient"),
    "glowrect": ("Glow Rect", "SDF rounded-rect grid pattern"),
    "coord_rotate": ("Coord Rotate", "UV matrix transform"),
    "derive": ("Derive Normals", "Height → tangent-space normal map"),
    "bump": ("Bump Light", "Phong-style bump mapping"),
    "paste": ("Paste", "Rotozoom overlay with combine mode"),
    "constant": ("Constant", "Solid color fill"),
    "output": ("Output", "Final texture sink"),
}


def _node(
    nid: str,
    op: str,
    inputs: list[str] | None = None,
    params: dict[str, Any] | None = None,
) -> dict[str, Any]:
    label, desc = OP_LABELS.get(op, (op, ""))
    return {
        "id": nid,
        "type": op,
        "label": label,
        "desc": desc,
        "inputs": inputs or [],
        "params": params or {},
    }


def recipe_to_graph(recipe: dict[str, Any]) -> dict[str, Any]:
    """Expand a high-level recipe dict into an explicit operator graph."""
    style = recipe.get("style", "panel")
    nodes: list[dict[str, Any]] = []

    nodes.append(_node("grad_bw", "gradient", params={
        "color_start": "0xff000000", "color_end": "0xffffffff",
    }))
    nodes.append(_node("grad_wb", "gradient", params={
        "color_start": "0xffffffff", "color_end": "0xff000000",
    }))
    nodes.append(_node("grad_white", "gradient", params={
        "color_start": "0xffffffff", "color_end": "0xffffffff",
    }))
    nodes.append(_node("grad_noise", "gradient", params={
        "color_start": "0xff000000", "color_end": "0xff646464",
    }))

    if style == "noise":
        nodes.append(_node("noise0", "noise", ["grad_bw"], {
            "freq_x": 2, "freq_y": 2, "octaves": 6, "fade": 0.5,
        }))
        nodes.append(_node("color0", "colorize", ["noise0"], {
            "color_start": recipe.get("color_start"),
            "color_end": recipe.get("color_end"),
        }))
        nodes.append(_node("output", "output", ["color0"]))
        return {"nodes": nodes, "output": "output", "size": recipe.get("size", 512), "seed": recipe.get("seed", 1)}

    voro_ids = []
    for i in range(4):
        vid = f"voro{i}"
        voro_ids.append(vid)
        nodes.append(_node(vid, "voronoi", ["grad_white"], {
            "intensity": recipe.get(f"voro_intensity_{i}", 37),
            "count": recipe.get(f"voro_count_{i}", 90),
            "min_dist": recipe.get(f"voro_dist_{i}", 0.125),
        }))

    if style == "voronoi":
        nodes.append(_node("color0", "colorize", ["voro0"], {
            "color_start": recipe.get("color_start"),
            "color_end": recipe.get("color_end"),
        }))
        nodes.append(_node("output", "output", ["color0"]))
        return {"nodes": nodes, "output": "output", "size": recipe.get("size", 512), "seed": recipe.get("seed", 1)}

    nodes.append(_node("combine0", "combine", voro_ids, {"weight": 1.5}))
    nodes.append(_node("blur0", "blur", ["combine0"], {"amount": recipe.get("blur", 0.0074)}))
    nodes.append(_node("noise_overlay", "noise", ["grad_noise"], {
        "freq_x": 4, "freq_y": 4, "octaves": 5, "fade": 0.995, "seed_offset": 3,
    }))
    nodes.append(_node("paste_noise", "paste", ["blur0", "noise_overlay"], {"combine_mode": "add"}))
    nodes.append(_node("base_colored", "colorize", ["paste_noise"], {
        "color_start": recipe.get("color_start"),
        "color_end": recipe.get("color_end"),
    }))

    if style == "metal":
        nodes.append(_node("output", "output", ["base_colored"]))
        return {"nodes": nodes, "output": "output", "size": recipe.get("size", 512), "seed": recipe.get("seed", 1)}

    if style not in ("panel", "organic", "ice", "lava", "rust"):
        nodes.append(_node("output", "output", ["base_colored"]))
        return {"nodes": nodes, "output": "output", "size": recipe.get("size", 512), "seed": recipe.get("seed", 1)}

    nodes.append(_node("rect1", "glowrect", ["grad_wb"], {
        "bg": "black", "orgx": 0.5, "orgy": 0.5, "ux": 0.41,
        "rectu": 0.25, "rectv": 0.64,
    }))
    nodes.append(_node("rect1x", "coord_rotate", ["rect1"], {
        "rotation": recipe.get("grid_rotation", 0.125),
    }))
    nodes.append(_node("rect1n", "derive", ["rect1x"], {
        "strength": recipe.get("bump_strength", 2.5),
    }))
    nodes.append(_node("bump0", "bump", ["base_colored", "rect1n"], {
        "light_x": recipe.get("light_x", -2.518),
        "light_y": recipe.get("light_y", 0.719),
        "light_z": recipe.get("light_z", -3.10),
    }))
    nodes.append(_node("rect2", "glowrect", ["grad_bw"], {
        "bg": "white", "orgx": 0.5, "orgy": 0.5, "ux": 0.36,
        "vy": 0.20, "rectu": 0.8805, "rectv": 0.74,
    }))
    nodes.append(_node("rect2x", "coord_rotate", ["rect2"], {
        "rotation": recipe.get("grid_rotation", 0.125),
    }))
    nodes.append(_node("paste_grid", "paste", ["bump0", "rect2x"], {"combine_mode": "multiply"}))
    nodes.append(_node("output", "output", ["paste_grid"]))

    return {
        "nodes": nodes,
        "output": "output",
        "size": recipe.get("size", 512),
        "seed": recipe.get("seed", 1),
    }


def graph_to_pgraph(graph: dict[str, Any]) -> str:
    """Serialize graph JSON to .pgraph text for primegraph."""
    lines = [
        "# Prime operator graph",
        f"size={graph.get('size', 512)}",
        f"seed={graph.get('seed', 1)}",
        f"output={graph.get('output', 'output')}",
        "",
    ]
    for node in graph["nodes"]:
        lines.append(f"[{node['id']}]")
        lines.append(f"type={node['type']}")
        for i, inp in enumerate(node.get("inputs", [])):
            lines.append(f"input{i}={inp}")
        for k, v in node.get("params", {}).items():
            if node["type"] == "glowrect":
                if k == "bg":
                    lines.append(f"i0={1 if v == 'white' else 0}")
                elif k in ("orgx", "orgy", "ux", "uy", "vx", "vy", "rectu", "rectv"):
                    slot = {"orgx": "f0", "orgy": "f1", "ux": "f2", "uy": "f3",
                            "vx": "f4", "vy": "f5", "rectu": "f6", "rectv": "f7"}.get(k, k)
                    lines.append(f"{slot}={v}")
                continue
            if k == "combine_mode" and isinstance(v, str):
                modes = {"add": 0, "multiply": 8}
                v = modes.get(v, v)
            lines.append(f"{k}={v}")
        lines.append("")
    return "\n".join(lines) + "\n"


def graph_for_display(graph: dict[str, Any]) -> list[dict[str, str]]:
    """Topological order for UI — output last."""
    nodes = {n["id"]: n for n in graph["nodes"]}
    output_id = graph.get("output", "output")
    order: list[str] = []
    visited: set[str] = set()

    def visit(nid: str) -> None:
        if nid in visited or nid not in nodes:
            return
        visited.add(nid)
        for inp in nodes[nid].get("inputs", []):
            visit(inp)
        order.append(nid)

    visit(output_id)
    return [
        {
            "id": n["id"],
            "type": n["type"],
            "label": n["label"],
            "desc": n["desc"],
            "inputs": n.get("inputs", []),
        }
        for nid in order
        for n in [nodes[nid]]
    ]
