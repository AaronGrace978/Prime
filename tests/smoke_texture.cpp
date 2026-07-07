// Texture-engine part of the Prime smoke test.
// Kept in its own translation unit: the texture and synth engines have
// (deliberately unmerged) legacy type headers that clash when included
// together.

#include "smoke_common.h"

#include "gentexture.hpp"

void TestTexture()
{
  InitTexgen();

  GenTexture grad;
  grad.Init(2,1);
  grad.Data[0].Init(0xff000000);
  grad.Data[1].Init(0xffffffff);

  GenTexture noise;
  noise.Init(64,64);
  noise.Noise(grad,2,2,5,0.5f,1234,
              GenTexture::NoiseDirect|GenTexture::NoiseBandlimit|GenTexture::NoiseNormalize);

  // the noise image must not be constant
  bool nonConstant = false;
  for(sInt i=1;i<noise.NPixels && !nonConstant;i++)
    if(noise.Data[i].r != noise.Data[0].r)
      nonConstant = true;
  CHECK(nonConstant,"texture generator produces non-constant noise");

  // a derived normal map must average to roughly "up" (z ~ 1.0)
  GenTexture normals;
  normals.Init(64,64);
  normals.Derive(noise,GenTexture::DeriveNormals,1.0f);

  sU64 sumB = 0;
  for(sInt i=0;i<normals.NPixels;i++)
    sumB += normals.Data[i].b;
  sU32 avgB = (sU32)(sumB / normals.NPixels);
  CHECK(avgB > 0x8000,"normal map derivation points mostly outward");
}
