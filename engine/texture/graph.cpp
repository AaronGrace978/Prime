/****************************************************************************/
/***                                                                      ***/
/***   Prime operator graph implementation                                ***/
/***                                                                      ***/
/****************************************************************************/

#include "graph.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

// ---- internal helpers (shared with recipe.cpp logic) ------------------------

static void MatMult(Matrix44 &dest,const Matrix44 &a,const Matrix44 &b)
{
  for(sInt i=0;i<4;i++)
    for(sInt j=0;j<4;j++)
      dest[i][j] = a[i][0]*b[0][j] + a[i][1]*b[1][j] + a[i][2]*b[2][j] + a[i][3]*b[3][j];
}

static void MatScale(Matrix44 &dest,sF32 sx,sF32 sy,sF32 sz)
{
  sSetMem(dest,0,sizeof(dest));
  dest[0][0] = sx; dest[1][1] = sy; dest[2][2] = sz; dest[3][3] = 1.0f;
}

static void MatTranslate(Matrix44 &dest,sF32 tx,sF32 ty,sF32 tz)
{
  MatScale(dest,1.0f,1.0f,1.0f);
  dest[3][0] = tx; dest[3][1] = ty; dest[3][2] = tz;
}

static void MatRotateZ(Matrix44 &dest,sF32 angle)
{
  sF32 s = sFSin(angle), c = sFCos(angle);
  MatScale(dest,1.0f,1.0f,1.0f);
  dest[0][0] = c;  dest[0][1] = s;
  dest[1][0] = -s; dest[1][1] = c;
}

static GenTexture MakeGradient(sU32 startCol,sU32 endCol)
{
  GenTexture tex;
  tex.Init(2,1);
  tex.Data[0].Init(startCol);
  tex.Data[1].Init(endCol);
  return tex;
}

static sU32 LocalRNG(sU32 &state)
{
  state = state * 1664525u + 1013904223u;
  return state;
}

static void RandomVoronoi(GenTexture &dest,const GenTexture &grad,sInt intensity,sInt maxCount,sF32 minDist,sBool useGlobalRand,sU32 rngSeed)
{
  sVERIFY(maxCount <= 256);
  CellCenter centers[256];
  sU32 rng = rngSeed ? rngSeed : 1;

  for(sInt i=0;i<maxCount;i++)
  {
    int intens;
    sF32 x, y;
    if(useGlobalRand)
    {
      intens = (rand() * intensity) / RAND_MAX;
      x = 1.0f * rand() / RAND_MAX;
      y = 1.0f * rand() / RAND_MAX;
    }
    else
    {
      intens = (int)((LocalRNG(rng) >> 16) % (sU32)(intensity + 1));
      x = (LocalRNG(rng) & 0xffff) / 65535.0f;
      y = (LocalRNG(rng) & 0xffff) / 65535.0f;
    }
    centers[i].x = x;
    centers[i].y = y;
    centers[i].color.Init(intens,intens,intens,255);
  }

  sF32 minDistSq = minDist*minDist;
  for(sInt i=1;i<maxCount;)
  {
    sF32 x = centers[i].x, y = centers[i].y;
    sInt j;
    for(j=0;j<i;j++)
    {
      sF32 dx = centers[j].x - x, dy = centers[j].y - y;
      if(dx < 0.0f) dx += 1.0f;
      if(dy < 0.0f) dy += 1.0f;
      dx = sMin(dx,1.0f-dx); dy = sMin(dy,1.0f-dy);
      if(dx*dx + dy*dy < minDistSq) break;
    }
    if(j<i) centers[i] = centers[--maxCount];
    else i++;
  }

  dest.Cells(grad,centers,maxCount,0.0f,GenTexture::CellInner);
}

static void ApplyColorize(GenTexture &img,sU32 startCol,sU32 endCol)
{
  Matrix44 m; Pixel s,e;
  s.Init(startCol); e.Init(endCol);
  sSetMem(m,0,sizeof(m));
  m[0][0] = (e.r - s.r) / 65535.0f;
  m[1][1] = (e.g - s.g) / 65535.0f;
  m[2][2] = (e.b - s.b) / 65535.0f;
  m[3][3] = 1.0f;
  m[0][3] = s.r / 65535.0f;
  m[1][3] = s.g / 65535.0f;
  m[2][3] = s.b / 65535.0f;
  img.ColorMatrixTransform(img,m,sTRUE);
}

static sU32 ParseHexColor(const char *s)
{
  if(s[0]=='0' && (s[1]=='x' || s[1]=='X')) s += 2;
  return (sU32)strtoul(s,0,16);
}

static void Trim(char *s)
{
  while(*s && isspace((unsigned char)*s)) s++;
  char *e = s + strlen(s);
  while(e>s && isspace((unsigned char)e[-1])) *--e = 0;
}

// ---- public API -------------------------------------------------------------

void PrimeGraphDefaults(PrimeGraph &g)
{
  sSetMem(&g,0,sizeof(g));
  g.size = 256;
  g.seed = 1;
  strcpy(g.outputId,"output");
}

const char *PrimeOpTypeName(PrimeOpType t)
{
  static const char *names[] = {
    "gradient","noise","voronoi","combine","blur","colorize",
    "glowrect","coord_rotate","derive","bump","paste","constant","output"
  };
  if(t < 0 || t >= PRIME_OP_COUNT) return "output";
  return names[t];
}

PrimeOpType PrimeOpTypeFromString(const char *s)
{
  if(!s) return PRIME_OP_OUTPUT;
  for(int i=0;i<PRIME_OP_COUNT;i++)
    if(!strcasecmp(s, PrimeOpTypeName((PrimeOpType)i)))
      return (PrimeOpType)i;
  return PRIME_OP_OUTPUT;
}

sInt PrimeGraphFindNode(const PrimeGraph &g, const char *id)
{
  for(sInt i=0;i<g.nNodes;i++)
    if(!strcmp(g.nodes[i].id, id))
      return i;
  return -1;
}

static void PrimeGraphNodeDefaults(PrimeGraphNode &n)
{
  sSetMem(&n,0,sizeof(n));
  n.type = PRIME_OP_OUTPUT;
  n.f0 = 1.0f;
}

static sInt PrimeGraphAddNode(PrimeGraph &g, const char *id, PrimeOpType type)
{
  if(g.nNodes >= PRIME_MAX_GRAPH_NODES) return -1;
  sInt idx = g.nNodes++;
  PrimeGraphNodeDefaults(g.nodes[idx]);
  strncpy(g.nodes[idx].id, id, PRIME_NODE_ID_LEN-1);
  g.nodes[idx].type = type;
  return idx;
}

static void PrimeGraphConnect(PrimeGraphNode &n, sInt slot, const char *inputId)
{
  if(slot < 0 || slot >= PRIME_MAX_NODE_INPUTS) return;
  strncpy(n.inputs[slot], inputId, PRIME_NODE_ID_LEN-1);
  if(slot + 1 > n.nInputs) n.nInputs = slot + 1;
}

bool PrimeGraphLoad(const char *path, PrimeGraph &g)
{
  PrimeGraphDefaults(g);
  FILE *f = fopen(path,"r");
  if(!f) return false;

  PrimeGraphNode *current = 0;
  char line[512];

  while(fgets(line,sizeof(line),f))
  {
    char *p = line;
    while(*p && *p!='#' && *p!='\n') p++;
    *p = 0;
    Trim(line);
    if(!line[0]) continue;

    if(line[0]=='[')
    {
      char *end = strchr(line,']');
      if(!end) continue;
      *end = 0;
      char *nodeId = line + 1;
      Trim(nodeId);
      sInt idx = PrimeGraphFindNode(g, nodeId);
      if(idx < 0)
        idx = PrimeGraphAddNode(g, nodeId, PRIME_OP_OUTPUT);
      current = &g.nodes[idx];
      continue;
    }

    char *eq = strchr(line,'=');
    if(!eq) continue;
    *eq = 0;
    char *key = line, *val = eq+1;
    Trim(key); Trim(val);

    if(!current)
    {
      if(!strcmp(key,"size"))              g.size = atoi(val);
      else if(!strcmp(key,"seed"))         g.seed = (sU32)strtoul(val,0,10);
      else if(!strcmp(key,"output"))       strncpy(g.outputId, val, PRIME_NODE_ID_LEN-1);
      continue;
    }

    if(!strcmp(key,"type"))                current->type = PrimeOpTypeFromString(val);
    else if(!strcmp(key,"input"))          PrimeGraphConnect(*current, 0, val);
    else if(!strncmp(key,"input",5) && key[5]>='0' && key[5]<='9')
    {
      int slot = atoi(key+5);
      PrimeGraphConnect(*current, slot, val);
    }
    else if(!strcmp(key,"color_start"))  current->colorStart = ParseHexColor(val);
    else if(!strcmp(key,"color_end"))    current->colorEnd = ParseHexColor(val);
    else if(!strcmp(key,"combine_mode")) current->combineMode = atoi(val);
    else if(!strcmp(key,"f0"))             current->f0 = (sF32)atof(val);
    else if(!strcmp(key,"f1"))             current->f1 = (sF32)atof(val);
    else if(!strcmp(key,"f2"))             current->f2 = (sF32)atof(val);
    else if(!strcmp(key,"f3"))             current->f3 = (sF32)atof(val);
    else if(!strcmp(key,"f4"))             current->f4 = (sF32)atof(val);
    else if(!strcmp(key,"f5"))             current->f5 = (sF32)atof(val);
    else if(!strcmp(key,"f6"))             current->f6 = (sF32)atof(val);
    else if(!strcmp(key,"f7"))             current->f7 = (sF32)atof(val);
    else if(!strcmp(key,"i0"))             current->i0 = atoi(val);
    else if(!strcmp(key,"i1"))             current->i1 = atoi(val);
    else if(!strcmp(key,"i2"))             current->i2 = atoi(val);
    else if(!strcmp(key,"i3"))             current->i3 = atoi(val);
    else if(!strcmp(key,"intensity"))     current->i0 = atoi(val);
    else if(!strcmp(key,"count"))          current->i1 = atoi(val);
    else if(!strcmp(key,"min_dist"))      current->f0 = (sF32)atof(val);
    else if(!strcmp(key,"freq_x"))         current->i0 = atoi(val);
    else if(!strcmp(key,"freq_y"))         current->i1 = atoi(val);
    else if(!strcmp(key,"octaves"))        current->i2 = atoi(val);
    else if(!strcmp(key,"fade"))           current->f0 = (sF32)atof(val);
    else if(!strcmp(key,"amount"))         current->f0 = current->f1 = (sF32)atof(val);
    else if(!strcmp(key,"strength"))       current->f0 = (sF32)atof(val);
    else if(!strcmp(key,"rotation"))       current->f0 = (sF32)atof(val);
    else if(!strcmp(key,"weight"))         current->f0 = (sF32)atof(val);
    else if(!strcmp(key,"light_x"))        current->f4 = (sF32)atof(val);
    else if(!strcmp(key,"light_y"))        current->f5 = (sF32)atof(val);
    else if(!strcmp(key,"light_z"))        current->f6 = (sF32)atof(val);
    else if(!strcmp(key,"seed_offset"))  current->i3 = atoi(val);
    else if(!strcmp(key,"orgx"))         current->f0 = (sF32)atof(val);
    else if(!strcmp(key,"orgy"))         current->f1 = (sF32)atof(val);
    else if(!strcmp(key,"ux"))           current->f2 = (sF32)atof(val);
    else if(!strcmp(key,"uy"))           current->f3 = (sF32)atof(val);
    else if(!strcmp(key,"vx"))           current->f4 = (sF32)atof(val);
    else if(!strcmp(key,"vy"))           current->f5 = (sF32)atof(val);
    else if(!strcmp(key,"rectu"))        current->f6 = (sF32)atof(val);
    else if(!strcmp(key,"rectv"))        current->f7 = (sF32)atof(val);
    else if(!strcmp(key,"bg"))           current->i0 = !strcasecmp(val,"white") ? 1 : 0;
  }

  fclose(f);
  return g.nNodes > 0;
}

bool PrimeGraphSave(const PrimeGraph &g, const char *path)
{
  FILE *f = fopen(path,"w");
  if(!f) return false;

  fprintf(f,"# Prime operator graph\n");
  fprintf(f,"size=%d\n", g.size);
  fprintf(f,"seed=%u\n", g.seed);
  fprintf(f,"output=%s\n\n", g.outputId);

  for(sInt i=0;i<g.nNodes;i++)
  {
    const PrimeGraphNode &n = g.nodes[i];
    fprintf(f,"[%s]\n", n.id);
    fprintf(f,"type=%s\n", PrimeOpTypeName(n.type));
    for(sInt j=0;j<n.nInputs;j++)
      if(n.inputs[j][0])
        fprintf(f,"input%d=%s\n", j, n.inputs[j]);

    switch(n.type)
    {
    case PRIME_OP_GRADIENT:
    case PRIME_OP_COLORIZE:
    case PRIME_OP_CONSTANT:
      fprintf(f,"color_start=0x%08x\n", n.colorStart);
      fprintf(f,"color_end=0x%08x\n", n.colorEnd);
      break;
    case PRIME_OP_NOISE:
      fprintf(f,"freq_x=%d\nfreq_y=%d\noctaves=%d\nfade=%g\n", n.i0,n.i1,n.i2,n.f0);
      if(n.i3) fprintf(f,"seed_offset=%d\n", n.i3);
      break;
    case PRIME_OP_VORONOI:
      fprintf(f,"intensity=%d\ncount=%d\nmin_dist=%g\n", n.i0,n.i1,n.f0);
      break;
    case PRIME_OP_COMBINE:
      fprintf(f,"weight=%g\n", n.f0);
      break;
    case PRIME_OP_BLUR:
      fprintf(f,"amount=%g\n", n.f0);
      break;
    case PRIME_OP_GLOWRECT:
      if(n.i0) fprintf(f,"bg=white\n");
      fprintf(f,"f0=%g\nf1=%g\nf2=%g\nf3=%g\nf4=%g\nf5=%g\nf6=%g\nf7=%g\n",
              n.f0,n.f1,n.f2,n.f3,n.f4,n.f5,n.f6,n.f7);
      break;
    case PRIME_OP_COORD_ROTATE:
      fprintf(f,"rotation=%g\n", n.f0);
      break;
    case PRIME_OP_DERIVE:
      fprintf(f,"strength=%g\n", n.f0);
      break;
    case PRIME_OP_BUMP:
      fprintf(f,"light_x=%g\nlight_y=%g\nlight_z=%g\n", n.f4,n.f5,n.f6);
      break;
    case PRIME_OP_PASTE:
      fprintf(f,"combine_mode=%d\n", n.combineMode);
      break;
    default:
      break;
    }
    fprintf(f,"\n");
  }

  fclose(f);
  return true;
}

// Evaluate in document order (node index) so rand()-based ops match recipe.cpp.
static bool PrimeGraphBuildOrder(const PrimeGraph &g, sInt *order, sInt &nOrder)
{
  nOrder = g.nNodes;
  for(sInt i=0;i<g.nNodes;i++)
    order[i] = i;
  return g.nNodes > 0;
}

struct EvalCache
{
  GenTexture tex[PRIME_MAX_GRAPH_NODES];
  sBool      valid[PRIME_MAX_GRAPH_NODES];
};

static const GenTexture *EvalInput(const PrimeGraph &g, EvalCache &cache, const PrimeGraphNode &n, sInt slot)
{
  if(slot < 0 || slot >= n.nInputs || !n.inputs[slot][0]) return 0;
  sInt idx = PrimeGraphFindNode(g, n.inputs[slot]);
  if(idx < 0 || !cache.valid[idx]) return 0;
  return &cache.tex[idx];
}

static bool EvalNode(const PrimeGraph &g, sInt nodeIdx, EvalCache &cache)
{
  if(cache.valid[nodeIdx]) return true;

  const PrimeGraphNode &n = g.nodes[nodeIdx];
  const sInt size = g.size;
  GenTexture &out = cache.tex[nodeIdx];

  Pixel black,white;
  black.Init(0,0,0,255);
  white.Init(255,255,255,255);

  switch(n.type)
  {
  case PRIME_OP_GRADIENT:
    out = MakeGradient(n.colorStart, n.colorEnd);
    break;

  case PRIME_OP_CONSTANT:
    out.Init(size,size);
    {
      Pixel c; c.Init(n.colorStart);
      out.LinearCombine(c, 1.0f, 0, 0);
    }
    break;

  case PRIME_OP_NOISE:
  {
    const GenTexture *grad = EvalInput(g, cache, n, 0);
    GenTexture localGrad;
    if(!grad)
    {
      localGrad = MakeGradient(0xff000000, 0xffffffff);
      grad = &localGrad;
    }
    out.Init(size,size);
    sInt seed = n.i3 ? n.i3 : (sInt)g.seed;
    out.Noise(*grad, n.i0 ? n.i0 : 2, n.i1 ? n.i1 : 2, n.i2 ? n.i2 : 6,
              n.f0 ? n.f0 : 0.5f, seed,
              GenTexture::NoiseDirect|GenTexture::NoiseBandlimit|GenTexture::NoiseNormalize);
    break;
  }

  case PRIME_OP_VORONOI:
  {
    const GenTexture *grad = EvalInput(g, cache, n, 0);
    GenTexture localGrad;
    if(!grad)
    {
      localGrad = MakeGradient(0xffffffff, 0xffffffff);
      grad = &localGrad;
    }
    out.Init(size,size);
    RandomVoronoi(out, *grad, n.i0, n.i1, n.f0, sTRUE, 0);
    break;
  }

  case PRIME_OP_COMBINE:
  {
    LinearInput inputs[PRIME_MAX_NODE_INPUTS];
    sInt nIn = 0;
    for(sInt i=0;i<n.nInputs && i<PRIME_MAX_NODE_INPUTS;i++)
    {
      const GenTexture *t = EvalInput(g, cache, n, i);
      if(!t) continue;
      inputs[nIn].Tex = t;
      inputs[nIn].Weight = n.f0 ? n.f0 : 1.5f;
      inputs[nIn].UShift = 0.0f;
      inputs[nIn].VShift = 0.0f;
      inputs[nIn].FilterMode = GenTexture::WrapU|GenTexture::WrapV|GenTexture::FilterNearest;
      nIn++;
    }
    out.Init(size,size);
    out.LinearCombine(black, 0.0f, inputs, nIn);
    break;
  }

  case PRIME_OP_BLUR:
  {
    const GenTexture *in = EvalInput(g, cache, n, 0);
    if(!in) return false;
    out.Init(size,size);
    sF32 amt = n.f0 ? n.f0 : 0.01f;
    out.Blur(*in, amt, amt, 1, GenTexture::WrapU|GenTexture::WrapV);
    break;
  }

  case PRIME_OP_COLORIZE:
  {
    const GenTexture *in = EvalInput(g, cache, n, 0);
    if(!in) return false;
    out = *in;
    ApplyColorize(out, n.colorStart, n.colorEnd);
    break;
  }

  case PRIME_OP_GLOWRECT:
  {
    const GenTexture *grad = EvalInput(g, cache, n, 0);
    if(!grad) grad = EvalInput(g, cache, n, 1);
    if(!grad) return false;
    out.Init(size,size);
    if(n.i0 == 1)
      out.LinearCombine(white, 1.0f, 0, 0);
    else
      out.LinearCombine(black, 1.0f, 0, 0);
    out.GlowRect(out, *grad, n.f0, n.f1, n.f2, n.f3, n.f4, n.f5, n.f6, n.f7);
    break;
  }

  case PRIME_OP_COORD_ROTATE:
  {
    const GenTexture *in = EvalInput(g, cache, n, 0);
    if(!in) return false;
    Matrix44 m1,m2,m3;
    MatTranslate(m1,-0.5f,-0.5f,0.0f);
    MatScale(m2,3.0f * sSQRT2F,3.0f * sSQRT2F,1.0f);
    MatMult(m3,m2,m1);
    MatRotateZ(m1, n.f0 * sPI2F);
    MatMult(m2,m1,m3);
    MatTranslate(m1,0.5f,0.5f,0.0f);
    MatMult(m3,m1,m2);
    out.Init(size,size);
    out.CoordMatrixTransform(*in, m3, GenTexture::WrapU|GenTexture::WrapV|GenTexture::FilterBilinear);
    break;
  }

  case PRIME_OP_DERIVE:
  {
    const GenTexture *in = EvalInput(g, cache, n, 0);
    if(!in) return false;
    out.Init(size,size);
    out.Derive(*in, GenTexture::DeriveNormals, n.f0 ? n.f0 : 2.5f);
    break;
  }

  case PRIME_OP_BUMP:
  {
    const GenTexture *surface = EvalInput(g, cache, n, 0);
    const GenTexture *normals = EvalInput(g, cache, n, 1);
    if(!surface || !normals) return false;
    out.Init(size,size);
    Pixel amb,diff;
    amb.Init(0xff101010);
    diff.Init(0xffffffff);
    out.Bump(*surface, *normals, 0, 0, 0.0f, 0.0f, 0.0f,
             n.f4, n.f5, n.f6, amb, diff, sTRUE);
    break;
  }

  case PRIME_OP_PASTE:
  {
    const GenTexture *bg = EvalInput(g, cache, n, 0);
    const GenTexture *snippet = EvalInput(g, cache, n, 1);
    if(!bg || !snippet) return false;
    out.Init(size,size);
    GenTexture::CombineOp mode = (GenTexture::CombineOp)(n.combineMode >= 0 ? n.combineMode : GenTexture::CombineMultiply);
    out.Paste(*bg, *snippet, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, mode, 0);
    break;
  }

  case PRIME_OP_OUTPUT:
  {
    const GenTexture *in = EvalInput(g, cache, n, 0);
    if(!in) return false;
    out = *in;
    break;
  }

  default:
    return false;
  }

  cache.valid[nodeIdx] = sTRUE;
  return true;
}

bool PrimeGraphEvaluate(const PrimeGraph &g, GenTexture &out)
{
  sInt order[PRIME_MAX_GRAPH_NODES];
  sInt nOrder = 0;
  if(!PrimeGraphBuildOrder(g, order, nOrder))
    return false;

  EvalCache cache;
  sSetMem(&cache.valid, 0, sizeof(cache.valid));

  for(sInt pass=0; pass<g.nNodes; pass++)
  {
  for(sInt i=0;i<nOrder;i++)
  {
    sInt nodeIdx = order[i];
    if(cache.valid[nodeIdx]) continue;

    const PrimeGraphNode &n = g.nodes[nodeIdx];
    sBool ready = sTRUE;
    for(sInt j=0;j<n.nInputs;j++)
    {
      if(!n.inputs[j][0]) continue;
      sInt dep = PrimeGraphFindNode(g, n.inputs[j]);
      if(dep < 0 || !cache.valid[dep]) { ready = sFALSE; break; }
    }
    if(!ready) continue;

    if(!EvalNode(g, nodeIdx, cache))
      return false;
  }
  }

  sInt outIdx = PrimeGraphFindNode(g, g.outputId);
  if(outIdx < 0 || !cache.valid[outIdx])
    return false;

  out = cache.tex[outIdx];
  return true;
}

void PrimeGraphFromRecipe(const PrimeRecipe &recipe, PrimeGraph &g)
{
  PrimeGraphDefaults(g);
  g.size = recipe.size;
  g.seed = recipe.seed;

  const bool hasBump = (recipe.style >= PRIME_STYLE_PANEL);

  // Gradients
  PrimeGraphAddNode(g, "grad_bw", PRIME_OP_GRADIENT);
  g.nodes[g.nNodes-1].colorStart = 0xff000000;
  g.nodes[g.nNodes-1].colorEnd   = 0xffffffff;

  PrimeGraphAddNode(g, "grad_wb", PRIME_OP_GRADIENT);
  g.nodes[g.nNodes-1].colorStart = 0xffffffff;
  g.nodes[g.nNodes-1].colorEnd   = 0xff000000;

  PrimeGraphAddNode(g, "grad_white", PRIME_OP_GRADIENT);
  g.nodes[g.nNodes-1].colorStart = 0xffffffff;
  g.nodes[g.nNodes-1].colorEnd   = 0xffffffff;

  PrimeGraphAddNode(g, "grad_noise", PRIME_OP_GRADIENT);
  g.nodes[g.nNodes-1].colorStart = 0xff000000;
  g.nodes[g.nNodes-1].colorEnd   = 0xff646464;

  if(recipe.style == PRIME_STYLE_NOISE)
  {
    PrimeGraphAddNode(g, "noise0", PRIME_OP_NOISE);
    PrimeGraphNode &nn = g.nodes[g.nNodes-1];
    PrimeGraphConnect(nn, 0, "grad_bw");
    nn.i0 = 2; nn.i1 = 2; nn.i2 = 6; nn.f0 = 0.5f;

    PrimeGraphAddNode(g, "color0", PRIME_OP_COLORIZE);
    PrimeGraphNode &cn = g.nodes[g.nNodes-1];
    PrimeGraphConnect(cn, 0, "noise0");
    cn.colorStart = recipe.colorStart;
    cn.colorEnd   = recipe.colorEnd;

    PrimeGraphAddNode(g, "output", PRIME_OP_OUTPUT);
    PrimeGraphConnect(g.nodes[g.nNodes-1], 0, "color0");
    strcpy(g.outputId, "output");
    return;
  }

  // Voronoi layers
  char voroIds[4][PRIME_NODE_ID_LEN];
  for(sInt i=0;i<4;i++)
  {
    snprintf(voroIds[i], sizeof(voroIds[i]), "voro%d", (int)i);
    PrimeGraphAddNode(g, voroIds[i], PRIME_OP_VORONOI);
    PrimeGraphNode &vn = g.nodes[g.nNodes-1];
    PrimeGraphConnect(vn, 0, "grad_white");
    vn.i0 = recipe.voroIntensity[i];
    vn.i1 = recipe.voroCount[i];
    vn.f0 = recipe.voroDist[i];
  }

  if(recipe.style == PRIME_STYLE_VORONOI)
  {
    PrimeGraphAddNode(g, "color0", PRIME_OP_COLORIZE);
    PrimeGraphNode &cn = g.nodes[g.nNodes-1];
    PrimeGraphConnect(cn, 0, "voro0");
    cn.colorStart = recipe.colorStart;
    cn.colorEnd   = recipe.colorEnd;

    PrimeGraphAddNode(g, "output", PRIME_OP_OUTPUT);
    PrimeGraphConnect(g.nodes[g.nNodes-1], 0, "color0");
    strcpy(g.outputId, "output");
    return;
  }

  PrimeGraphAddNode(g, "combine0", PRIME_OP_COMBINE);
  PrimeGraphNode &comb = g.nodes[g.nNodes-1];
  for(sInt i=0;i<4;i++) PrimeGraphConnect(comb, i, voroIds[i]);
  comb.f0 = 1.5f;

  PrimeGraphAddNode(g, "blur0", PRIME_OP_BLUR);
  PrimeGraphNode &bl = g.nodes[g.nNodes-1];
  PrimeGraphConnect(bl, 0, "combine0");
  bl.f0 = recipe.blurAmount;

  PrimeGraphAddNode(g, "noise_overlay", PRIME_OP_NOISE);
  PrimeGraphNode &no = g.nodes[g.nNodes-1];
  PrimeGraphConnect(no, 0, "grad_noise");
  no.i0 = 4; no.i1 = 4; no.i2 = 5; no.f0 = 0.995f; no.i3 = 3;

  PrimeGraphAddNode(g, "paste_noise", PRIME_OP_PASTE);
  PrimeGraphNode &pn = g.nodes[g.nNodes-1];
  PrimeGraphConnect(pn, 0, "blur0");
  PrimeGraphConnect(pn, 1, "noise_overlay");
  pn.combineMode = GenTexture::CombineAdd;

  PrimeGraphAddNode(g, "base_colored", PRIME_OP_COLORIZE);
  PrimeGraphNode &bc = g.nodes[g.nNodes-1];
  PrimeGraphConnect(bc, 0, "paste_noise");
  bc.colorStart = recipe.colorStart;
  bc.colorEnd   = recipe.colorEnd;

  if(recipe.style == PRIME_STYLE_METAL)
  {
    PrimeGraphAddNode(g, "output", PRIME_OP_OUTPUT);
    PrimeGraphConnect(g.nodes[g.nNodes-1], 0, "base_colored");
    strcpy(g.outputId, "output");
    return;
  }

  if(!hasBump)
  {
    PrimeGraphAddNode(g, "output", PRIME_OP_OUTPUT);
    PrimeGraphConnect(g.nodes[g.nNodes-1], 0, "base_colored");
    strcpy(g.outputId, "output");
    return;
  }

  // Grid bump pipeline
  PrimeGraphAddNode(g, "rect1", PRIME_OP_GLOWRECT);
  PrimeGraphNode &r1 = g.nodes[g.nNodes-1];
  PrimeGraphConnect(r1, 0, "grad_wb");
  r1.i0 = 0;
  r1.f0=0.5f; r1.f1=0.5f; r1.f2=0.41f; r1.f5=0.25f; r1.f6=0.7805f; r1.f7=0.64f;

  PrimeGraphAddNode(g, "rect1x", PRIME_OP_COORD_ROTATE);
  PrimeGraphNode &r1x = g.nodes[g.nNodes-1];
  PrimeGraphConnect(r1x, 0, "rect1");
  r1x.f0 = recipe.gridRotation;

  PrimeGraphAddNode(g, "rect1n", PRIME_OP_DERIVE);
  PrimeGraphNode &r1n = g.nodes[g.nNodes-1];
  PrimeGraphConnect(r1n, 0, "rect1x");
  r1n.f0 = recipe.bumpStrength;

  PrimeGraphAddNode(g, "bump0", PRIME_OP_BUMP);
  PrimeGraphNode &bp = g.nodes[g.nNodes-1];
  PrimeGraphConnect(bp, 0, "base_colored");
  PrimeGraphConnect(bp, 1, "rect1n");
  bp.f4 = recipe.lightX;
  bp.f5 = recipe.lightY;
  bp.f6 = recipe.lightZ;

  PrimeGraphAddNode(g, "rect2", PRIME_OP_GLOWRECT);
  PrimeGraphNode &r2 = g.nodes[g.nNodes-1];
  PrimeGraphConnect(r2, 0, "grad_bw");
  r2.i0 = 1;
  r2.f0=0.5f; r2.f1=0.5f; r2.f2=0.36f; r2.f5=0.20f; r2.f6=0.8805f; r2.f7=0.74f;

  PrimeGraphAddNode(g, "rect2x", PRIME_OP_COORD_ROTATE);
  PrimeGraphNode &r2x = g.nodes[g.nNodes-1];
  PrimeGraphConnect(r2x, 0, "rect2");
  r2x.f0 = recipe.gridRotation;

  PrimeGraphAddNode(g, "paste_grid", PRIME_OP_PASTE);
  PrimeGraphNode &pg = g.nodes[g.nNodes-1];
  PrimeGraphConnect(pg, 0, "bump0");
  PrimeGraphConnect(pg, 1, "rect2x");
  pg.combineMode = GenTexture::CombineMultiply;

  PrimeGraphAddNode(g, "output", PRIME_OP_OUTPUT);
  PrimeGraphConnect(g.nodes[g.nNodes-1], 0, "paste_grid");
  strcpy(g.outputId, "output");
}
