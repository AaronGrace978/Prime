/****************************************************************************/
/***                                                                      ***/
/***   Prime texture recipe — parameterized procedural generation         ***/
/***   Derived from OpenKTG / Farbrausch Werkkzeug texture ops             ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef PRIME_RECIPE_HPP
#define PRIME_RECIPE_HPP

#include "gentexture.hpp"

enum PrimeStyle
{
  PRIME_STYLE_NOISE = 0,
  PRIME_STYLE_VORONOI,
  PRIME_STYLE_METAL,
  PRIME_STYLE_PANEL,
  PRIME_STYLE_ORGANIC,
  PRIME_STYLE_ICE,
  PRIME_STYLE_LAVA,
  PRIME_STYLE_RUST,
  PRIME_STYLE_COUNT
};

struct PrimeRecipe
{
  sInt       size;           // power of two, 16..4096
  sU32       seed;
  PrimeStyle style;

  // colorize gradient (0xaarrggbb)
  sU32       colorStart;
  sU32       colorEnd;

  // voronoi layers (up to 4)
  sInt       voroIntensity[4];
  sInt       voroCount[4];
  sF32       voroDist[4];

  sF32       blurAmount;
  sF32       gridRotation;   // fraction of full turn (0.125 = 45deg)
  sF32       bumpStrength;

  // lighting for bump pass
  sF32       lightX, lightY, lightZ;
};

// Sensible defaults matching the original OpenKTG demo panel.
void PrimeRecipeDefaults(PrimeRecipe &r);

// Parse a simple key=value recipe file (one per line, # comments).
// Returns false on parse error.
bool PrimeRecipeLoad(const char *path, PrimeRecipe &r);

// Generate the final texture into `out` using the given recipe.
// Caller must have called InitTexgen() and srand(recipe.seed).
void PrimeRecipeGenerate(const PrimeRecipe &recipe, GenTexture &out);

// Save 32bpp uncompressed TGA (top-down). Returns false on I/O error.
bool PrimeSaveTGA(const GenTexture &img, const char *filename);

// Human-readable style name.
const char *PrimeStyleName(PrimeStyle s);

// Parse style from string ("panel", "lava", ...). Returns PRIME_STYLE_PANEL on unknown.
PrimeStyle PrimeStyleFromString(const char *s);

#endif
