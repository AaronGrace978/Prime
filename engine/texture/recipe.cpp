/****************************************************************************/
/***                                                                      ***/
/***   Prime texture recipe implementation                                ***/
/***                                                                      ***/
/****************************************************************************/

#include "recipe.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

// ---- matrix helpers (from OpenKTG demo) -----------------------------------

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

static GenTexture LinearGradient(sU32 startCol,sU32 endCol)
{
  GenTexture tex;
  tex.Init(2,1);
  tex.Data[0].Init(startCol);
  tex.Data[1].Init(endCol);
  return tex;
}

static void RandomVoronoi(GenTexture &dest,const GenTexture &grad,sInt intensity,sInt maxCount,sF32 minDist)
{
  sVERIFY(maxCount <= 256);
  CellCenter centers[256];

  for(sInt i=0;i<maxCount;i++)
  {
    int intens = (rand() * intensity) / RAND_MAX;
    centers[i].x = 1.0f * rand() / RAND_MAX;
    centers[i].y = 1.0f * rand() / RAND_MAX;
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

static void Colorize(GenTexture &img,sU32 startCol,sU32 endCol)
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

// ---- public API -------------------------------------------------------------

void PrimeRecipeDefaults(PrimeRecipe &r)
{
  sSetMem(&r,0,sizeof(r));
  r.size = 256;
  r.seed = 1;
  r.style = PRIME_STYLE_PANEL;
  r.colorStart = 0xff747d8e;
  r.colorEnd   = 0xfff1feff;
  r.voroIntensity[0]=37; r.voroCount[0]=90;  r.voroDist[0]=0.125f;
  r.voroIntensity[1]=42; r.voroCount[1]=132; r.voroDist[1]=0.063f;
  r.voroIntensity[2]=37; r.voroCount[2]=240; r.voroDist[2]=0.063f;
  r.voroIntensity[3]=37; r.voroCount[3]=255; r.voroDist[3]=0.063f;
  r.blurAmount = 0.0074f;
  r.gridRotation = 0.125f;
  r.bumpStrength = 2.5f;
  r.lightX = -2.518f; r.lightY = 0.719f; r.lightZ = -3.10f;
}

const char *PrimeStyleName(PrimeStyle s)
{
  static const char *names[] = {
    "noise","voronoi","metal","panel","organic","ice","lava","rust"
  };
  if(s < 0 || s >= PRIME_STYLE_COUNT) return "panel";
  return names[s];
}

PrimeStyle PrimeStyleFromString(const char *s)
{
  if(!s) return PRIME_STYLE_PANEL;
  for(int i=0;i<PRIME_STYLE_COUNT;i++)
    if(!strcasecmp(s, PrimeStyleName((PrimeStyle)i)))
      return (PrimeStyle)i;
  return PRIME_STYLE_PANEL;
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

bool PrimeRecipeLoad(const char *path, PrimeRecipe &r)
{
  PrimeRecipeDefaults(r);
  FILE *f = fopen(path,"r");
  if(!f) return false;

  char line[512];
  while(fgets(line,sizeof(line),f))
  {
    char *p = line;
    while(*p && *p!='#' && *p!='\n') p++;
    *p = 0;
    Trim(line);
    if(!line[0]) continue;

    char *eq = strchr(line,'=');
    if(!eq) continue;
    *eq = 0;
    char *key = line, *val = eq+1;
    Trim(key); Trim(val);

    if(!strcmp(key,"size"))              r.size = atoi(val);
    else if(!strcmp(key,"seed"))         r.seed = (sU32)strtoul(val,0,10);
    else if(!strcmp(key,"style"))        r.style = PrimeStyleFromString(val);
    else if(!strcmp(key,"color_start"))  r.colorStart = ParseHexColor(val);
    else if(!strcmp(key,"color_end"))    r.colorEnd = ParseHexColor(val);
    else if(!strcmp(key,"blur"))         r.blurAmount = (sF32)atof(val);
    else if(!strcmp(key,"grid_rotation"))r.gridRotation = (sF32)atof(val);
    else if(!strcmp(key,"bump_strength"))r.bumpStrength = (sF32)atof(val);
    else if(!strcmp(key,"light_x"))       r.lightX = (sF32)atof(val);
    else if(!strcmp(key,"light_y"))      r.lightY = (sF32)atof(val);
    else if(!strcmp(key,"light_z"))      r.lightZ = (sF32)atof(val);
    else if(!strncmp(key,"voro_intensity_",15)) { int i=atoi(key+15); if(i>=0&&i<4) r.voroIntensity[i]=atoi(val); }
    else if(!strncmp(key,"voro_count_",11))     { int i=atoi(key+11); if(i>=0&&i<4) r.voroCount[i]=atoi(val); }
    else if(!strncmp(key,"voro_dist_",10))    { int i=atoi(key+10); if(i>=0&&i<4) r.voroDist[i]=(sF32)atof(val); }
  }

  fclose(f);
  return true;
}

void PrimeRecipeGenerate(const PrimeRecipe &recipe, GenTexture &out)
{
  const sInt size = recipe.size;
  Pixel black,white;
  black.Init(0,0,0,255);
  white.Init(255,255,255,255);

  GenTexture gradBW = LinearGradient(0xff000000,0xffffffff);
  GenTexture gradWB = LinearGradient(0xffffffff,0xff000000);
  GenTexture gradWhite = LinearGradient(0xffffffff,0xffffffff);

  // --- style: noise only ---------------------------------------------------
  if(recipe.style == PRIME_STYLE_NOISE)
  {
    out.Init(size,size);
    out.Noise(gradBW,2,2,6,0.5f,(sInt)recipe.seed,
              GenTexture::NoiseDirect|GenTexture::NoiseBandlimit|GenTexture::NoiseNormalize);
    Colorize(out,recipe.colorStart,recipe.colorEnd);
    return;
  }

  // --- build voronoi layers ------------------------------------------------
  GenTexture voro[4];
  for(sInt i=0;i<4;i++)
  {
    voro[i].Init(size,size);
    RandomVoronoi(voro[i],gradWhite,recipe.voroIntensity[i],recipe.voroCount[i],recipe.voroDist[i]);
  }

  if(recipe.style == PRIME_STYLE_VORONOI)
  {
    out = voro[0];
    Colorize(out,recipe.colorStart,recipe.colorEnd);
    return;
  }

  LinearInput inputs[4];
  for(sInt i=0;i<4;i++)
  {
    inputs[i].Tex = &voro[i];
    inputs[i].Weight = 1.5f;
    inputs[i].UShift = 0.0f;
    inputs[i].VShift = 0.0f;
    inputs[i].FilterMode = GenTexture::WrapU|GenTexture::WrapV|GenTexture::FilterNearest;
  }

  GenTexture baseTex;
  baseTex.Init(size,size);
  baseTex.LinearCombine(black,0.0f,inputs,4);
  baseTex.Blur(baseTex,recipe.blurAmount,recipe.blurAmount,1,GenTexture::WrapU|GenTexture::WrapV);

  GenTexture noiseLayer;
  noiseLayer.Init(size,size);
  noiseLayer.Noise(LinearGradient(0xff000000,0xff646464),4,4,5,0.995f,3,
                   GenTexture::NoiseDirect|GenTexture::NoiseNormalize|GenTexture::NoiseBandlimit);
  baseTex.Paste(baseTex,noiseLayer,0.0f,0.0f,1.0f,0.0f,0.0f,1.0f,GenTexture::CombineAdd,0);
  Colorize(baseTex,recipe.colorStart,recipe.colorEnd);

  if(recipe.style == PRIME_STYLE_METAL)
  {
    out = baseTex;
    return;
  }

  // --- grid + bump (panel, organic, ice, lava, rust) -----------------------
  Matrix44 m1,m2,m3;
  MatTranslate(m1,-0.5f,-0.5f,0.0f);
  MatScale(m2,3.0f * sSQRT2F,3.0f * sSQRT2F,1.0f);
  MatMult(m3,m2,m1);
  MatRotateZ(m1,recipe.gridRotation * sPI2F);
  MatMult(m2,m1,m3);
  MatTranslate(m1,0.5f,0.5f,0.0f);
  MatMult(m3,m1,m2);

  GenTexture rect1,rect1x,rect1n;
  rect1.Init(size,size);
  rect1.LinearCombine(black,1.0f,0,0);
  rect1.GlowRect(rect1,gradWB,0.5f,0.5f,0.41f,0.0f,0.0f,0.25f,0.7805f,0.64f);
  rect1x.Init(size,size);
  rect1x.CoordMatrixTransform(rect1,m3,GenTexture::WrapU|GenTexture::WrapV|GenTexture::FilterBilinear);
  rect1n.Init(size,size);
  rect1n.Derive(rect1x,GenTexture::DeriveNormals,recipe.bumpStrength);

  out.Init(size,size);
  Pixel amb,diff;
  amb.Init(0xff101010);
  diff.Init(0xffffffff);
  out.Bump(baseTex,rect1n,0,0,0.0f,0.0f,0.0f,recipe.lightX,recipe.lightY,recipe.lightZ,amb,diff,sTRUE);

  GenTexture rect2,rect2x;
  rect2.Init(size,size);
  rect2.LinearCombine(white,1.0f,0,0);
  rect2.GlowRect(rect2,gradBW,0.5f,0.5f,0.36f,0.0f,0.0f,0.20f,0.8805f,0.74f);
  rect2x.Init(size,size);
  rect2x.CoordMatrixTransform(rect2,m3,GenTexture::WrapU|GenTexture::WrapV|GenTexture::FilterBilinear);
  out.Paste(out,rect2x,0.0f,0.0f,1.0f,0.0f,0.0f,1.0f,GenTexture::CombineMultiply,0);
}

bool PrimeSaveTGA(const GenTexture &img, const char *filename)
{
  FILE *f = fopen(filename,"wb");
  if(!f) return false;

  sU8 buffer[18];
  sSetMem(buffer,0,sizeof(buffer));
  buffer[ 2] = 2;
  buffer[12] = img.XRes & 0xff;
  buffer[13] = img.XRes >> 8;
  buffer[14] = img.YRes & 0xff;
  buffer[15] = img.YRes >> 8;
  buffer[16] = 32;
  buffer[17] = 0x20;

  if(!fwrite(buffer,sizeof(buffer),1,f)) { fclose(f); return false; }

  sU8 *lineBuf = new sU8[img.XRes*4];
  for(sInt y=0;y<img.YRes;y++)
  {
    const Pixel *in = &img.Data[y*img.XRes];
    for(sInt x=0;x<img.XRes;x++)
    {
      lineBuf[x*4+0] = in->b >> 8;
      lineBuf[x*4+1] = in->g >> 8;
      lineBuf[x*4+2] = in->r >> 8;
      lineBuf[x*4+3] = in->a >> 8;
      in++;
    }
    if(!fwrite(lineBuf,img.XRes*4,1,f)) { delete[] lineBuf; fclose(f); return false; }
  }
  delete[] lineBuf;
  fclose(f);
  return true;
}
