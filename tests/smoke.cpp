// Prime engine smoke test driver.
//
// 1. Generates a texture in memory and checks that the generator actually
//    produced non-trivial (non-constant) image data.
// 2. Loads a .v2m tune (path passed as argv[1]), renders a few seconds of
//    audio and checks that the synth produced sound (non-silence, no NaNs,
//    sane amplitude).

#include <stdio.h>

#include "smoke_common.h"

int g_failures = 0;

int main(int argc,char **argv)
{
  printf("=== Prime smoke test ===\n");

  TestTexture();
  TestGraph();

  if(argc > 1)
    TestSynth(argv[1]);
  else
    printf("skip - no tune given, synth test skipped\n");

  if(g_failures)
  {
    printf("=== %d check(s) FAILED ===\n",g_failures);
    return 1;
  }

  printf("=== all checks passed ===\n");
  return 0;
}
