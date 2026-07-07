/****************************************************************************/
/***                                                                      ***/
/***   Prime operator graph — Werkkzeug-style procedural documents        ***/
/***   Lazy-evaluated node graphs with memoization                        ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef PRIME_GRAPH_HPP
#define PRIME_GRAPH_HPP

#include "recipe.hpp"

#define PRIME_MAX_GRAPH_NODES 64
#define PRIME_MAX_NODE_INPUTS 4
#define PRIME_NODE_ID_LEN     48

enum PrimeOpType
{
  PRIME_OP_GRADIENT = 0,
  PRIME_OP_NOISE,
  PRIME_OP_VORONOI,
  PRIME_OP_COMBINE,
  PRIME_OP_BLUR,
  PRIME_OP_COLORIZE,
  PRIME_OP_GLOWRECT,
  PRIME_OP_COORD_ROTATE,
  PRIME_OP_DERIVE,
  PRIME_OP_BUMP,
  PRIME_OP_PASTE,
  PRIME_OP_CONSTANT,
  PRIME_OP_OUTPUT,
  PRIME_OP_COUNT
};

struct PrimeGraphNode
{
  char         id[PRIME_NODE_ID_LEN];
  PrimeOpType  type;
  char         inputs[PRIME_MAX_NODE_INPUTS][PRIME_NODE_ID_LEN];
  sInt         nInputs;

  // Shared parameter slots — meaning depends on op type.
  sU32 colorStart;
  sU32 colorEnd;
  sF32 f0, f1, f2, f3, f4, f5, f6, f7;
  sInt i0, i1, i2, i3;
  sInt combineMode;
};

struct PrimeGraph
{
  sInt           size;
  sU32           seed;
  char           outputId[PRIME_NODE_ID_LEN];
  PrimeGraphNode nodes[PRIME_MAX_GRAPH_NODES];
  sInt           nNodes;
};

void PrimeGraphDefaults(PrimeGraph &g);

// Parse a .pgraph document (key=value lines, [node id] sections).
bool PrimeGraphLoad(const char *path, PrimeGraph &g);

// Save graph back to .pgraph format.
bool PrimeGraphSave(const PrimeGraph &g, const char *path);

// Evaluate the graph into `out`. Caller must call InitTexgen() and srand(seed).
bool PrimeGraphEvaluate(const PrimeGraph &g, GenTexture &out);

// Build an explicit operator graph from a high-level recipe.
void PrimeGraphFromRecipe(const PrimeRecipe &recipe, PrimeGraph &g);

// Human-readable operator name.
const char *PrimeOpTypeName(PrimeOpType t);

// Parse operator type from string. Returns PRIME_OP_OUTPUT on unknown.
PrimeOpType PrimeOpTypeFromString(const char *s);

// Find node index by id, or -1.
sInt PrimeGraphFindNode(const PrimeGraph &g, const char *id);

#endif
