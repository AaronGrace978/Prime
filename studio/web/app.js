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

async function loadStatus() {
  try {
    const r = await fetch("/api/status");
    const s = await r.json();
    statusDot.className = "status__dot status__dot--ok";
    const ai = s.ai_openai ? "OpenAI + rules fallback" : "rules engine (set OPENAI_API_KEY for GPT)";
    statusText.textContent = s.forge ? `engine ready · ${ai}` : "engine offline — build primeforge";
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
  operators.forEach((op, i) => {
    const node = document.createElement("div");
    node.className = "op-node";
    node.style.animationDelay = `${i * 0.06}s`;
    node.innerHTML = `<div class="op-node__label">${op.label}</div><div class="op-node__desc">${op.desc}</div>`;
    graphEl.appendChild(node);
  });
}

function renderRecipe(recipe) {
  const lines = Object.entries(recipe).map(([k, v]) => `${k}=${v}`);
  recipeEl.textContent = lines.join("\n");
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
    const data = await r.json();

    previewEl.src = data.image_url + "?t=" + Date.now();
    previewEl.hidden = false;
    placeholderEl.hidden = true;

    previewStatsEl.hidden = false;
    previewStatsEl.textContent = `${data.width}×${data.height} · ${data.recipe.style} · seed ${data.recipe.seed} · ${data.ms}ms`;

    renderGraph(data.operators);
    renderRecipe(data.recipe);

    aiMetaEl.hidden = false;
    aiModeEl.textContent = data.mode === "openai" ? "GPT" : "rules AI";
    aiModeEl.className = "badge" + (data.mode === "openai" ? " badge--openai" : "");
    aiExplanationEl.textContent = data.explanation;

    timingEl.textContent = `rendered in ${data.ms}ms via ${data.mode}`;
  } catch (e) {
    alert("Generation failed: " + e.message);
  } finally {
    generateBtn.disabled = false;
    generateBtn.innerHTML = '<span class="btn__icon">⚡</span> Generate';
  }
}

generateBtn.addEventListener("click", generate);
promptEl.addEventListener("keydown", (e) => {
  if (e.key === "Enter" && (e.metaKey || e.ctrlKey)) generate();
});

loadStatus();
// auto-demo on load
promptEl.value = "dark kkrieger sci-fi floor grating with brushed metal and diagonal grid bumps";
setTimeout(generate, 600);
