const $ = (sel) => document.querySelector(sel);

const promptEl = $("#prompt");
const generateBtn = $("#generate");
const previewEl = $("#preview");
const placeholderEl = $("#placeholder");
const previewStatsEl = $("#previewStats");
const graphEl = $("#graph");
const recipeEl = $("#recipe");
const aiMetaEl = $("#aiMeta");
const aiModeEl = $("#aiMode");
const aiExplanationEl = $("#aiExplanation");
const statusDot = $("#statusDot");
const statusText = $("#statusText");
const timingEl = $("#timing");
const presetsEl = $("#presets");
const sizeEl = $("#size");
const mutationsEl = $("#mutations");

let currentRecipe = null;
let currentGraph = null;

async function loadStatus() {
  try {
    const r = await fetch("/api/status");
    const s = await r.json();
    statusDot.className = "status__dot status__dot--ok";
    const graph = s.graph ? "primegraph" : "primeforge";
    const ai = s.ai_openai ? "OpenAI + rules" : "rules engine";
    statusText.textContent = s.forge
      ? `v${s.version || "0.3"} · ${graph} · ${ai}`
      : "engine offline — build primegraph";
    if (!s.forge) statusDot.className = "status__dot";

    const presets = await (await fetch("/api/presets")).json();
    presetsEl.innerHTML = "";
    for (const [name, text] of Object.entries(presets)) {
      const chip = document.createElement("button");
      chip.className = "preset-chip";
      chip.textContent = name;
      chip.title = text;
      chip.onclick = () => { promptEl.value = text; };
      presetsEl.appendChild(chip);
    }
  } catch {
    statusText.textContent = "server offline";
  }
}

function renderGraph(operators) {
  graphEl.innerHTML = "";
  if (!operators?.length) {
    graphEl.innerHTML = '<div class="graph-empty">Generate to see the operator chain</div>';
    return;
  }

  operators.forEach((op, i) => {
    const node = document.createElement("div");
    node.className = "op-node";
    node.style.animationDelay = `${i * 0.04}s`;
    node.dataset.nodeId = op.id;

    const inputs = (op.inputs || [])
      .map((id) => `<span class="op-node__wire" title="input from ${id}">${id}</span>`)
      .join("");

    node.innerHTML = `
      <div class="op-node__type">${op.type}</div>
      <div class="op-node__label">${op.label}</div>
      <div class="op-node__desc">${op.desc}</div>
      ${inputs ? `<div class="op-node__inputs">${inputs}</div>` : ""}
    `;
    graphEl.appendChild(node);
  });
}

function renderRecipe(recipe) {
  const lines = Object.entries(recipe).map(([k, v]) => `${k}=${v}`);
  recipeEl.textContent = lines.join("\n");
}

function renderMutations(mutations) {
  mutationsEl.innerHTML = "";
  if (!mutations?.length) {
    mutationsEl.hidden = true;
    return;
  }
  mutationsEl.hidden = false;
  mutations.forEach((m) => {
    const btn = document.createElement("button");
    btn.className = "mut-btn";
    btn.innerHTML = `<span>${m.icon}</span> ${m.label}`;
    btn.title = `Mutate: ${m.label}`;
    btn.onclick = () => applyMutation(m.id);
    mutationsEl.appendChild(btn);
  });
}

function applyResponse(data) {
  currentRecipe = data.recipe;
  currentGraph = data.graph;

  previewEl.src = data.image_url + "?t=" + Date.now();
  previewEl.hidden = false;
  placeholderEl.hidden = true;

  previewStatsEl.hidden = false;
  previewStatsEl.textContent =
    `${data.width}×${data.height} · ${data.recipe.style} · seed ${data.recipe.seed} · ${data.graph?.nodes?.length || "?"} nodes · ${data.ms}ms · ${data.renderer}`;

  renderGraph(data.operators);
  renderRecipe(data.recipe);
  renderMutations(data.mutations);

  aiMetaEl.hidden = false;
  const modeLabels = { openai: "GPT", rules: "rules AI", mutation: "mutation" };
  aiModeEl.textContent = modeLabels[data.mode] || data.mode;
  aiModeEl.className = "badge" + (data.mode === "openai" ? " badge--openai" : data.mode === "mutation" ? " badge--mutate" : "");
  aiExplanationEl.textContent = data.explanation;

  timingEl.textContent = `rendered in ${data.ms}ms via ${data.renderer}`;
}

async function generate() {
  const prompt = promptEl.value.trim();
  if (!prompt) {
    promptEl.focus();
    return;
  }

  generateBtn.disabled = true;
  generateBtn.textContent = "Generating…";
  timingEl.textContent = "";

  try {
    const r = await fetch("/api/generate", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ prompt, size: parseInt(sizeEl.value, 10) }),
    });
    if (!r.ok) {
      const err = await r.json();
      throw new Error(err.detail || r.statusText);
    }
    applyResponse(await r.json());
  } catch (e) {
    alert("Generation failed: " + e.message);
  } finally {
    generateBtn.disabled = false;
    generateBtn.innerHTML = '<span class="btn__icon">⚡</span> Generate';
  }
}

async function applyMutation(mutation) {
  if (!currentRecipe) return;

  generateBtn.disabled = true;
  timingEl.textContent = "mutating…";

  try {
    const r = await fetch("/api/mutate", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ recipe: currentRecipe, mutation }),
    });
    if (!r.ok) {
      const err = await r.json();
      throw new Error(err.detail || r.statusText);
    }
    applyResponse(await r.json());
  } catch (e) {
    alert("Mutation failed: " + e.message);
  } finally {
    generateBtn.disabled = false;
  }
}

generateBtn.addEventListener("click", generate);
promptEl.addEventListener("keydown", (e) => {
  if (e.key === "Enter" && (e.metaKey || e.ctrlKey)) generate();
});

loadStatus();
promptEl.value = "dark kkrieger sci-fi floor grating with brushed metal and diagonal grid bumps";
setTimeout(generate, 600);
