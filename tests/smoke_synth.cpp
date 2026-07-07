// Synth-engine part of the Prime smoke test.

#include "smoke_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "v2mplayer.h"
#include "v2mconv.h"
#include "sounddef.h"

static sU8 *LoadFile(const char *path, sU32 *outLen)
{
  FILE *f = fopen(path,"rb");
  if(!f)
    return 0;

  fseek(f,0,SEEK_END);
  long len = ftell(f);
  fseek(f,0,SEEK_SET);

  sU8 *buf = new sU8[len];
  if(fread(buf,1,len,f) != (size_t)len)
  {
    delete[] buf;
    fclose(f);
    return 0;
  }

  fclose(f);
  *outLen = (sU32)len;
  return buf;
}

void TestSynth(const char *tunePath)
{
  sU32 inLen = 0;
  sU8 *inData = LoadFile(tunePath,&inLen);
  CHECK(inData != 0,"test tune loads from disk");
  if(!inData)
    return;

  sdInit();
  int delta = CheckV2MVersion(inData,inLen);
  CHECK(delta >= 0,"v2m version check succeeds");
  if(delta < 0)
  {
    delete[] inData;
    return;
  }

  const sU8 *tune = inData;
  sU8 *converted = 0;
  if(delta > 0)
  {
    int convLen = 0;
    ConvertV2M(inData,inLen,&converted,&convLen);
    CHECK(converted && convLen > 0,"v2m conversion to current format succeeds");
    tune = converted;
  }

  V2MPlayer player;
  player.Init();
  sBool opened = player.Open(tune,44100);
  CHECK(opened,"V2MPlayer opens the tune");

  if(opened)
  {
    player.Play();
    CHECK(player.IsPlaying(),"player reports playing state");

    const sU32 nSamples = 44100 * 5; // 5 seconds
    sF32 *buf = new sF32[nSamples*2];
    player.Render(buf,nSamples);

    sF32 peak = 0.0f;
    sF64 energy = 0.0;
    bool sane = true;
    for(sU32 i=0;i<nSamples*2;i++)
    {
      sF32 v = buf[i];
      if(v != v || fabsf(v) > 100.0f) // NaN or absurd amplitude
        sane = false;
      if(fabsf(v) > peak)
        peak = fabsf(v);
      energy += (sF64)v*v;
    }

    CHECK(sane,"rendered audio contains no NaNs or absurd samples");
    CHECK(peak > 0.01f,"rendered audio is not silence");
    CHECK(energy / (nSamples*2) > 1e-6,"rendered audio has plausible energy");

    delete[] buf;
    player.Close();
  }

  delete[] converted;
  delete[] inData;
}
