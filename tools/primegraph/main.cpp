/****************************************************************************/
/***   primegraph — operator-graph texture renderer for Prime             ***/
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.hpp"
#include "recipe.hpp"

static void PrintUsage()
{
  printf("primegraph - render textures from operator graphs\n\n"
         "usage:\n"
         "  primegraph -g graph.pgraph [-o output.tga]\n"
         "  primegraph -r recipe.txt [-o output.tga] [--save-graph graph.pgraph]\n\n"
         "Graph format: key=value lines with [node_id] sections (see engine/texture/graph.hpp)\n");
}

int main(int argc, char **argv)
{
  const char *graphPath = 0;
  const char *recipePath = 0;
  const char *outPath = "output.tga";
  const char *saveGraphPath = 0;

  for(int i=1;i<argc;i++)
  {
    if(!strcmp(argv[i],"-g") && i+1 < argc)
      graphPath = argv[++i];
    else if(!strcmp(argv[i],"-r") && i+1 < argc)
      recipePath = argv[++i];
    else if(!strcmp(argv[i],"-o") && i+1 < argc)
      outPath = argv[++i];
    else if(!strcmp(argv[i],"--save-graph") && i+1 < argc)
      saveGraphPath = argv[++i];
    else if(!strcmp(argv[i],"-h") || !strcmp(argv[i],"--help"))
    {
      PrintUsage();
      return 0;
    }
    else
    {
      fprintf(stderr,"primegraph: unknown option '%s'\n", argv[i]);
      PrintUsage();
      return 1;
    }
  }

  if(!graphPath && !recipePath)
  {
    fprintf(stderr,"primegraph: need -g graph.pgraph or -r recipe.txt\n");
    return 1;
  }

  PrimeGraph graph;
  PrimeRecipe recipe;
  bool haveRecipe = false;

  if(recipePath)
  {
    if(!PrimeRecipeLoad(recipePath, recipe))
    {
      fprintf(stderr,"primegraph: couldn't read recipe '%s'\n", recipePath);
      return 1;
    }
    haveRecipe = true;
    PrimeGraphFromRecipe(recipe, graph);
    if(saveGraphPath && !PrimeGraphSave(graph, saveGraphPath))
    {
      fprintf(stderr,"primegraph: couldn't write graph '%s'\n", saveGraphPath);
      return 1;
    }
  }
  else if(!PrimeGraphLoad(graphPath, graph))
  {
    fprintf(stderr,"primegraph: couldn't read graph '%s'\n", graphPath);
    return 1;
  }

  sInt size = graph.size;
  if(size < 16 || size > 4096 || (size & (size-1)))
  {
    fprintf(stderr,"primegraph: size must be power of two 16..4096\n");
    return 1;
  }

  srand(graph.seed);
  InitTexgen();

  GenTexture tex;
  if(!PrimeGraphEvaluate(graph, tex))
  {
    fprintf(stderr,"primegraph: graph evaluation failed\n");
    return 1;
  }

  if(!PrimeSaveTGA(tex, outPath))
  {
    fprintf(stderr,"primegraph: couldn't write '%s'\n", outPath);
    return 1;
  }

  if(haveRecipe)
    printf("primegraph: %s (%dx%d, style=%s, seed=%u, nodes=%d)\n",
           outPath, tex.XRes, tex.YRes, PrimeStyleName(recipe.style), recipe.seed, graph.nNodes);
  else
    printf("primegraph: %s (%dx%d, seed=%u, nodes=%d)\n",
           outPath, tex.XRes, tex.YRes, graph.seed, graph.nNodes);
  return 0;
}
