/****************************************************************************/
/***   primeforge — recipe-driven single-texture generator for Prime AI   ***/
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "recipe.hpp"

int main(int argc, char **argv)
{
  const char *recipePath = 0;
  const char *outPath = "output.tga";

  for(int i=1;i<argc;i++)
  {
    if(!strcmp(argv[i],"-r") && i+1 < argc)
      recipePath = argv[++i];
    else if(!strcmp(argv[i],"-o") && i+1 < argc)
      outPath = argv[++i];
    else
    {
      printf("primeforge - generate one texture from a recipe file\n\n"
             "usage: primeforge -r recipe.txt [-o output.tga]\n\n"
             "Recipe format: key=value lines (see docs/AI.md)\n");
      return (strcmp(argv[i],"-h") && strcmp(argv[i],"--help")) ? 1 : 0;
    }
  }

  if(!recipePath)
  {
    fprintf(stderr,"primeforge: missing -r recipe file\n");
    return 1;
  }

  PrimeRecipe recipe;
  if(!PrimeRecipeLoad(recipePath, recipe))
  {
    fprintf(stderr,"primeforge: couldn't read recipe '%s'\n",recipePath);
    return 1;
  }

  if(recipe.size < 16 || recipe.size > 4096 || (recipe.size & (recipe.size-1)))
  {
    fprintf(stderr,"primeforge: size must be power of two 16..4096\n");
    return 1;
  }

  srand(recipe.seed);
  InitTexgen();

  GenTexture tex;
  PrimeRecipeGenerate(recipe, tex);

  if(!PrimeSaveTGA(tex, outPath))
  {
    fprintf(stderr,"primeforge: couldn't write '%s'\n",outPath);
    return 1;
  }

  printf("primeforge: %s (%dx%d, style=%s, seed=%u)\n",
         outPath, tex.XRes, tex.YRes, PrimeStyleName(recipe.style), recipe.seed);
  return 0;
}
