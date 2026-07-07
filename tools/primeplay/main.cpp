/****************************************************************************/
/***                                                                      ***/
/***   primeplay - V2 module (.v2m) offline renderer                      ***/
/***                                                                      ***/
/***   Renders a .v2m tune with the Prime synth engine (derived from     ***/
/***   the Farbrausch V2 synthesizer system) into a 16-bit stereo WAV.   ***/
/***   This is the sound of kkrieger and debris, resurrected.            ***/
/***                                                                      ***/
/****************************************************************************/

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
  if(len <= 0)
  {
    fclose(f);
    return 0;
  }

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

static void PutU16(sU8 *p, sU16 v) { p[0]=v&0xff; p[1]=v>>8; }
static void PutU32(sU8 *p, sU32 v) { p[0]=v&0xff; p[1]=(v>>8)&0xff; p[2]=(v>>16)&0xff; p[3]=v>>24; }

static bool WriteWav(const char *path, const sS16 *samples, sU32 nStereoSamples, sU32 sampleRate)
{
  FILE *f = fopen(path,"wb");
  if(!f)
    return false;

  const sU32 dataBytes = nStereoSamples * 2 * sizeof(sS16);

  sU8 hdr[44];
  memcpy(hdr+ 0,"RIFF",4);
  PutU32(hdr+ 4,36+dataBytes);
  memcpy(hdr+ 8,"WAVE",4);
  memcpy(hdr+12,"fmt ",4);
  PutU32(hdr+16,16);                    // fmt chunk size
  PutU16(hdr+20,1);                     // PCM
  PutU16(hdr+22,2);                     // stereo
  PutU32(hdr+24,sampleRate);
  PutU32(hdr+28,sampleRate*2*sizeof(sS16));
  PutU16(hdr+32,2*sizeof(sS16));        // block align
  PutU16(hdr+34,16);                    // bits per sample
  memcpy(hdr+36,"data",4);
  PutU32(hdr+40,dataBytes);

  bool ok = fwrite(hdr,sizeof(hdr),1,f) == 1;
  if(ok && dataBytes)
    ok = fwrite(samples,dataBytes,1,f) == 1;

  fclose(f);
  return ok;
}

static sS16 FloatToS16(sF32 v)
{
  sF32 s = v * 32767.0f;
  if(s > 32767.0f) s = 32767.0f;
  if(s < -32768.0f) s = -32768.0f;
  return (sS16)lrintf(s);
}

int main(int argc,char **argv)
{
  const char *inPath = 0;
  const char *outPath = 0;
  sU32 sampleRate = 44100;
  sU32 maxSeconds = 3600;

  for(int i=1;i<argc;i++)
  {
    if(!strcmp(argv[i],"-o") && i+1 < argc)
      outPath = argv[++i];
    else if(!strcmp(argv[i],"-r") && i+1 < argc)
      sampleRate = (sU32)atoi(argv[++i]);
    else if(!strcmp(argv[i],"-t") && i+1 < argc)
      maxSeconds = (sU32)atoi(argv[++i]);
    else if(argv[i][0] != '-' && !inPath)
      inPath = argv[i];
    else
    {
      printf("primeplay - V2 module renderer (Prime engine)\n\n"
             "usage: primeplay <tune.v2m> [-o out.wav] [-r samplerate] [-t maxseconds]\n\n"
             "  -o out.wav     output file (default: <tune>.wav)\n"
             "  -r samplerate  output sample rate, 44100-192000 (default: 44100)\n"
             "  -t maxseconds  safety cap on render length (default: 3600)\n");
      return (strcmp(argv[i],"-h") && strcmp(argv[i],"--help")) ? 1 : 0;
    }
  }

  if(!inPath)
  {
    fprintf(stderr,"primeplay: no input file. try --help\n");
    return 1;
  }

  if(sampleRate < 44100 || sampleRate > 192000)
  {
    fprintf(stderr,"primeplay: sample rate must be between 44100 and 192000\n");
    return 1;
  }

  // load tune
  sU32 inLen = 0;
  sU8 *inData = LoadFile(inPath,&inLen);
  if(!inData)
  {
    fprintf(stderr,"primeplay: couldn't read '%s'\n",inPath);
    return 1;
  }

  // check the v2m version; convert older files to the current format
  sdInit();
  const sU8 *tune = inData;
  sU8 *converted = 0;

  int versionDelta = CheckV2MVersion(inData,inLen);
  if(versionDelta < 0)
  {
    fprintf(stderr,"primeplay: '%s' doesn't look like a valid v2m file (%s)\n",
            inPath,v2mconv_errors[-versionDelta]);
    delete[] inData;
    return 1;
  }
  if(versionDelta > 0)
  {
    printf("primeplay: converting tune from an older v2m format (delta %d)...\n",versionDelta);
    int convLen = 0;
    ConvertV2M(inData,inLen,&converted,&convLen);
    if(!converted || !convLen)
    {
      fprintf(stderr,"primeplay: conversion failed\n");
      delete[] inData;
      return 1;
    }
    tune = converted;
  }

  // fire up the player
  V2MPlayer player;
  player.Init();
  if(!player.Open(tune,sampleRate))
  {
    fprintf(stderr,"primeplay: player rejected '%s'\n",inPath);
    delete[] converted;
    delete[] inData;
    return 1;
  }

  player.Play();

  // render
  const sU32 chunkSamples = 4096;
  const sU32 maxSamples = maxSeconds * sampleRate;

  sF32 *chunk = new sF32[chunkSamples*2];
  sS16 *out = 0;
  sU32 outCap = 0;
  sU32 total = 0;

  printf("primeplay: rendering '%s' at %u Hz...\n",inPath,sampleRate);

  while(player.IsPlaying() && total < maxSamples)
  {
    sU32 todo = sMin(chunkSamples,maxSamples-total);
    player.Render(chunk,todo);

    if(total+todo > outCap)
    {
      sU32 newCap = sMax(outCap*2,(sU32)(60*sampleRate));
      if(newCap < total+todo) newCap = total+todo;
      sS16 *grown = new sS16[(size_t)newCap*2];
      if(out) memcpy(grown,out,(size_t)total*2*sizeof(sS16));
      delete[] out;
      out = grown;
      outCap = newCap;
    }

    for(sU32 i=0;i<todo*2;i++)
      out[total*2+i] = FloatToS16(chunk[i]);

    total += todo;
  }

  player.Close();

  // write result
  char defaultOut[1024];
  if(!outPath)
  {
    snprintf(defaultOut,sizeof(defaultOut),"%s",inPath);
    char *dot = strrchr(defaultOut,'.');
    if(dot) *dot = 0;
    strncat(defaultOut,".wav",sizeof(defaultOut)-strlen(defaultOut)-1);
    outPath = defaultOut;
  }

  bool ok = WriteWav(outPath,out,total,sampleRate);

  delete[] out;
  delete[] chunk;
  delete[] converted;
  delete[] inData;

  if(!ok)
  {
    fprintf(stderr,"primeplay: couldn't write '%s'\n",outPath);
    return 1;
  }

  printf("primeplay: wrote %s (%u samples, %.1f seconds)\n",
         outPath,total,(double)total/sampleRate);
  return 0;
}
