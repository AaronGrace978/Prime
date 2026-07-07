// Trimmed, portable version of the original fr_public v2/sounddef.cpp.
//
// The original file also contained the patch-editor state and bank
// load/save code for the Windows VSTi/standalone tools (file I/O,
// clipboard, MessageBox and friends). All the v2m *converter* needs is
// the parameter tables from sounddef.h plus the version-size tables
// computed here, so this file keeps exactly that and nothing else.
//
// Original code (c) Tammo 'kb' Hinrichs 2000-2008, released under a
// BSD-style license / Artistic License 2.0 (see licenses/V2-LICENSE.txt).

#include "types.h"
#include "sounddef.h"

#include <string.h>
#include <stdio.h>

unsigned char *soundmem = 0;
long          *patchoffsets = 0;
unsigned char *editmem = 0;
char          patchnames[128][32];
char          globals[v2ngparms];
int           v2version = 0;
int           *v2vsizes = 0;
int           *v2gsizes = 0;
int           *v2topics2 = 0;
int           *v2gtopics2 = 0;
int           v2curpatch = 0;

#ifdef RONAN
char          speech[64][256];
char          *speechptrs[64];
#endif

void sdInit()
{
  if (v2vsizes) // already initialized
    return;

  soundmem = new unsigned char[smsize + v2soundsize];
  patchoffsets = (long *)soundmem;
  unsigned char *sptr = soundmem + 128 * sizeof(void *);

  char s[256];

  for (int i = 0; i < 129; i++)
  {
    if (i < 128)
    {
      patchoffsets[i] = (long)(sptr - soundmem);
      sprintf(s, "Init Patch #%03d", i);
      strcpy(patchnames[i], s);
    }
    else
      editmem = sptr;
    memcpy(sptr, v2initsnd, v2soundsize);
    sptr += v2soundsize;
  }

  for (int i = 0; i < v2ngparms; i++)
    globals[i] = v2initglobs[i];

  // init version control
  v2version = 0;
  for (int i = 0; i < v2nparms; i++)
    if (v2parms[i].version > v2version) v2version = v2parms[i].version;
  for (int i = 0; i < v2ngparms; i++)
    if (v2gparms[i].version > v2version) v2version = v2gparms[i].version;

  v2vsizes = new int[v2version + 1];
  v2gsizes = new int[v2version + 1];
  memset(v2vsizes, 0, (v2version + 1) * sizeof(int));
  memset(v2gsizes, 0, (v2version + 1) * sizeof(int));
  for (int i = 0; i < v2nparms; i++)
  {
    const V2PARAM &p = v2parms[i];
    for (int j = v2version; j >= p.version; j--)
      v2vsizes[j]++;
  }
  for (int i = 0; i < v2ngparms; i++)
  {
    const V2PARAM &p = v2gparms[i];
    for (int j = v2version; j >= p.version; j--)
      v2gsizes[j]++;
  }

  for (int i = 0; i <= v2version; i++)
    v2vsizes[i] += 1 + 255 * 3;

  v2topics2 = new int[v2ntopics];
  int p = 0;
  for (int i = 0; i < v2ntopics; i++)
  {
    v2topics2[i] = p;
    p += v2topics[i].no;
  }

  v2gtopics2 = new int[v2ngtopics];
  p = 0;
  for (int i = 0; i < v2ngtopics; i++)
  {
    v2gtopics2[i] = p;
    p += v2gtopics[i].no;
  }

#ifdef RONAN
  memset(speech, 0, sizeof(speech));
  for (int i = 0; i < 64; i++)
    speechptrs[i] = speech[i];

  strcpy(speech[0], "!DHAX_ !kwIH_k !br4AH_UHn !fAA_ks !jAH_mps !OW!vER_ !DHAX_ !lEY!zIY_ !dAA_g ");
#endif
}

void sdClose()
{
  delete[] soundmem;
  delete[] v2vsizes;
  delete[] v2gsizes;
  delete[] v2topics2;
  delete[] v2gtopics2;
  soundmem = 0;
  v2vsizes = 0;
  v2gsizes = 0;
  v2topics2 = 0;
  v2gtopics2 = 0;
}
