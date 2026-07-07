// Graph-engine smoke test — recipe and graph paths must agree.

#include "smoke_common.h"
#include "graph.hpp"
#include "recipe.hpp"

void TestGraph()
{
  InitTexgen();

  PrimeRecipe recipe;
  PrimeRecipeDefaults(recipe);
  recipe.size = 64;
  recipe.seed = 4242;
  recipe.style = PRIME_STYLE_PANEL;

  srand(recipe.seed);
  GenTexture fromRecipe;
  PrimeRecipeGenerate(recipe, fromRecipe);

  PrimeGraph graph;
  PrimeGraphFromRecipe(recipe, graph);
  CHECK(graph.nNodes >= 8, "recipe expands to multi-node graph");

  srand(recipe.seed);
  GenTexture fromGraph;
  CHECK(PrimeGraphEvaluate(graph, fromGraph), "graph evaluation succeeds");

  CHECK(fromRecipe.XRes == fromGraph.XRes && fromRecipe.YRes == fromGraph.YRes,
        "graph output matches recipe dimensions");

  // Pixel-identical: same seed, same operator chain.
  bool match = true;
  for(sInt i=0;i<fromRecipe.NPixels && match;i++)
  {
    if(fromRecipe.Data[i].v != fromGraph.Data[i].v)
      match = false;
  }
  CHECK(match, "graph output matches recipe output exactly");

  // Round-trip save/load
  const char *path = "/tmp/prime_graph_test.pgraph";
  CHECK(PrimeGraphSave(graph, path), "graph saves to disk");

  PrimeGraph loaded;
  CHECK(PrimeGraphLoad(path, loaded), "graph loads from disk");
  CHECK(loaded.nNodes == graph.nNodes, "loaded graph has same node count");

  srand(recipe.seed);
  GenTexture fromLoaded;
  CHECK(PrimeGraphEvaluate(loaded, fromLoaded), "loaded graph evaluates");

  match = true;
  for(sInt i=0;i<fromRecipe.NPixels && match;i++)
  {
    if(fromRecipe.Data[i].v != fromLoaded.Data[i].v)
      match = false;
  }
  CHECK(match, "loaded graph matches recipe output");
}
