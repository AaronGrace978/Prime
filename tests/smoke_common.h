#ifndef PRIME_SMOKE_COMMON_H
#define PRIME_SMOKE_COMMON_H

#include <stdio.h>

// shared failure counter (defined in smoke.cpp)
extern int g_failures;

#define CHECK(cond,msg)                                        \
  do {                                                         \
    if(cond)                                                   \
      printf("ok   - %s\n",msg);                               \
    else                                                       \
    {                                                          \
      printf("FAIL - %s\n",msg);                               \
      g_failures++;                                            \
    }                                                          \
  } while(0)

// implemented in smoke_texture.cpp / smoke_synth.cpp
void TestTexture();
void TestSynth(const char *tunePath);

#endif
