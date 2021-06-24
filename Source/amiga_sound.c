/*
 * amiga_sound.c
 *
 * Changes to distributed source are indicated by comments containing
 * the keyword AMIGA.
 *
 * Frotz is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Frotz is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 */

#include <proto/dos.h>
#include <proto/exec.h>
#include <devices/audio.h>
#include <exec/exec.h>
#include <graphics/gfxbase.h>
#include <math.h>
#include <stdio.h>
#include "frotz.h"
#include "blorb.h"
#include "blorblow.h"
#include "amiga.h"

struct SampleHeader
{
  unsigned short Length;
  unsigned char	 Times;
  unsigned char  BaseNote;
  unsigned short Frequency;
  unsigned char	 Blank[2];
  unsigned short SampleLength;
};

struct MacintoshMid
{
  unsigned short Length;
  unsigned char	 Commands[11];
  unsigned char  SoundFileName[19];
};

#define UNSIGNED_OFFSET 68

struct IOAudio *SoundRequest, *SoundLeftRequest, *SoundRightRequest;
int SampleFrequency, SampleOffset, SampleLength;
int SoundOpenCode = -1;
int CurrentSample, SampleUnsigned, SamplePlaying, SampleLoops;
char *SampleMemory;
char SoundDirectory[MAX_FILE_NAME+1];

extern struct GfxBase *GfxBase;
extern struct MsgPort *SoundMsgPort;
extern int current_game;
extern FILE* BlorbFile;
extern bb_map_t* BlorbMap;

double ReadExtended(const unsigned char *bytes);

/*
 * os_finish_with_sample
 *
 * Remove the current sample from memory (if any).
 *
 */

void os_finish_with_sample (void)
{
  os_stop_sample();

  if (CurrentSample != 0)
  {
    if (SampleMemory != 0)
    {
      FreeVec(SampleMemory);
      SampleMemory = 0;
    }
    CurrentSample = 0;
  }
}

/*
 * os_prepare_sample
 *
 * Load the given sample from the disk.
 *
 */

void os_prepare_sample (int number)
{
  if (CurrentSample == number)
    return;
  os_finish_with_sample();

BPTR sound_data_file;

  SampleUnsigned = 0;
  sound_data_file = OpenSoundData(number);
  if (sound_data_file)
  {
  struct FileInfoBlock *sound_data_fib;

    if (sound_data_fib = AllocDosObject(DOS_FIB,0))
    {
    unsigned long sample_length;

      ExamineFH(sound_data_file,sound_data_fib);
      sample_length = sound_data_fib->fib_Size;
      FreeDosObject(DOS_FIB,sound_data_fib);

      if (SampleMemory = AllocVec(sample_length,MEMF_CHIP))
      {
      struct SampleHeader *header;

	Read(sound_data_file,SampleMemory,sample_length);
	Close(sound_data_file);

	header = (struct SampleHeader *)SampleMemory;
	SampleFrequency = header->Frequency;
	SampleOffset = sizeof(struct SampleHeader);
	SampleLength = header->SampleLength;
      }
    }
  }

  if ((SampleMemory == 0) && (BlorbMap != 0))
  {
  bb_result_t res;
  bb_aux_sound_t *info = 0;

    if (bb_load_resource_snd(BlorbMap,bb_method_FilePos,&res,number,&info) == bb_err_None)
    {
      /* Look for a FORM AIFF resource */
      if (BlorbMap->chunks[res.chunknum].type == bb_make_id('F','O','R','M'))
      {
	if (SampleMemory = AllocVec(res.length,MEMF_CHIP))
	{
	  fseek(BlorbFile,res.data.startpos,SEEK_SET);
	  fread(SampleMemory,1,res.length,BlorbFile);
	  OpenAIFF(res.length);
	}
      }
    }
  }

  if (SampleMemory == 0)
    return;
  CurrentSample = number;

  if (SampleUnsigned)
  {
  int i;

    for (i = 0; i < SampleLength; i++)
      *(SampleMemory+SampleOffset+i) ^= 0x80;
  }
}

/*
 * os_start_sample
 *
 * Play the given sample at the given volume (ranging from 1 to 8 and
 * 255 meaning a default volume). The sound is played once or several
 * times in the background (255 meaning forever). The end_of_sound
 * function is called as soon as the sound finishes.
 *
 */

void os_start_sample (int number, int volume, int repeats)
{
unsigned long rate;
int start = 0;

  os_stop_sample();
  os_prepare_sample(number);

  if (CurrentSample == 0) return;

  if (repeats == 0)
    repeats = 1;

  rate = (GfxBase->DisplayFlags&PAL ? 3546895 : 3579545) / SampleFrequency;

  SampleLoops = 0;
  while (start < SampleLength)
  {
    int length = SampleLength-start;
    if (length > 102400)
    {
      length = 102400;
      repeats = 1;
    }
    SampleLoops += 2;

    SoundLeftRequest->ioa_Request.io_Command = CMD_WRITE;
    SoundLeftRequest->ioa_Request.io_Flags = ADIOF_PERVOL;
    SoundLeftRequest->ioa_Period = rate;
    SoundLeftRequest->ioa_Volume = volume * 8;
    SoundLeftRequest->ioa_Cycles = repeats;
    SoundLeftRequest->ioa_Data = SampleMemory+SampleOffset+start;
    SoundLeftRequest->ioa_Length = length;
    if (SampleUnsigned)
    {
      if (start == 0)
	SoundLeftRequest->ioa_Data += UNSIGNED_OFFSET;
      if (start+length == SampleLength)
      {
	if (SoundLeftRequest->ioa_Length > (UNSIGNED_OFFSET*2))
	  SoundLeftRequest->ioa_Length -= (UNSIGNED_OFFSET*2);
      }
    }
    BeginIO(SoundLeftRequest);

    SoundRightRequest->ioa_Request.io_Command = CMD_WRITE;
    SoundRightRequest->ioa_Request.io_Flags = ADIOF_PERVOL;
    SoundRightRequest->ioa_Period = rate;
    SoundRightRequest->ioa_Volume = volume * 8;
    SoundRightRequest->ioa_Cycles = repeats;
    SoundRightRequest->ioa_Data = SampleMemory+SampleOffset+start;
    SoundRightRequest->ioa_Length = length;
    if (SampleUnsigned)
    {
      if (start == 0)
	SoundRightRequest->ioa_Data += UNSIGNED_OFFSET;
      if (start+length == SampleLength)
      {
	if (SoundRightRequest->ioa_Length > (UNSIGNED_OFFSET*2))
	  SoundRightRequest->ioa_Length -= (UNSIGNED_OFFSET*2);
      }
    }
    BeginIO(SoundRightRequest);

    start += length;
  }

  SamplePlaying = 1;
}

/*
 * os_stop_sample
 *
 * Turn off the current sample.
 *
 */

void os_stop_sample (void)
{
  if (SoundOpenCode != 0) return;

  SoundLeftRequest->ioa_Request.io_Command = CMD_FLUSH;
  BeginSoundIO(SoundLeftRequest);

  SoundRightRequest->ioa_Request.io_Command = CMD_FLUSH;
  BeginSoundIO(SoundRightRequest);

  while (GetMsg(SoundMsgPort));

  SamplePlaying = 0;
  SampleLoops = 0;
}

int InitializeSound(void)
{
  SetupSoundDirectory();

  if ((SoundRequest = (struct IOAudio *)
    CreateIORequest(SoundMsgPort,sizeof(struct IOAudio))) == 0) return 0;
  if ((SoundLeftRequest = (struct IOAudio *)
    CreateIORequest(SoundMsgPort,sizeof(struct IOAudio))) == 0) return 0;
  if ((SoundRightRequest = (struct IOAudio *)
    CreateIORequest(SoundMsgPort,sizeof(struct IOAudio))) == 0) return 0;

static unsigned char allocation_map[] = { 3,5,10,12 };

  SoundRequest->ioa_Request.io_Message.mn_Node.ln_Pri = 127;
  SoundRequest->ioa_Request.io_Command = ADCMD_ALLOCATE;
  SoundRequest->ioa_Request.io_Flags = ADIOF_NOWAIT;
  SoundRequest->ioa_Data = allocation_map;
  SoundRequest->ioa_Length = sizeof(allocation_map);

  if ((SoundOpenCode =
    OpenDevice("audio.device",0,(struct IORequest *)SoundRequest,0)) != 0) return 0;

unsigned long channels;

  channels = (unsigned long)SoundRequest->ioa_Request.io_Unit;
  CopyMem(SoundRequest,SoundLeftRequest,sizeof(struct IOAudio));
  SoundLeftRequest->ioa_Request.io_Unit = (void *)(channels & 9);
  CopyMem(SoundRequest,SoundRightRequest,sizeof(struct IOAudio));
  SoundRightRequest->ioa_Request.io_Unit = (void *)(channels & 6);

  return 1;
}

void ResetSound(void)
{
  os_finish_with_sample();
  if (SoundOpenCode == 0) CloseDevice((struct IORequest *)SoundRequest);
  if (SoundRequest) DeleteIORequest(SoundRequest);
  if (SoundLeftRequest) DeleteIORequest(SoundLeftRequest);
  if (SoundRightRequest) DeleteIORequest(SoundRightRequest);

  SoundOpenCode = -1;
  SoundRequest = 0;
  SoundLeftRequest = 0;
  SoundRightRequest = 0;
}

void BeginSoundIO(struct IOAudio *AudioRequest)
{
  AudioRequest->ioa_Request.io_Flags = IOF_QUICK;
  BeginIO(AudioRequest);

  if ((AudioRequest->ioa_Request.io_Flags & IOF_QUICK) == 0)
    WaitForMsg(SoundMsgPort);
}

void SetupSoundDirectory(void)
{
char base_dir[MAX_FILE_NAME+1];
BPTR lock;

  if (lock = Lock(story_name,ACCESS_READ))
  {
    NameFromLock(lock,base_dir,MAX_FILE_NAME);
    UnLock(lock);
  }
  else strcpy(base_dir,story_name);
  *FilePart(base_dir) = 0;

  switch (current_game)
  {
    case 14:				/* The Lurking Horror */
      strcpy(SoundDirectory,base_dir);
      AddPart(SoundDirectory,"LurkingHorrorSound",MAX_FILE_NAME);
      if (lock = Lock(SoundDirectory,ACCESS_READ))
      {
	UnLock(lock);
	return;
      }
      break;
    case 20:				/* Sherlock */
      strcpy(SoundDirectory,base_dir);
      AddPart(SoundDirectory,"SherlockSound",MAX_FILE_NAME);
      if (lock = Lock(SoundDirectory,ACCESS_READ))
      {
	UnLock(lock);
	return;
      }
      break;
  }
  strcpy(SoundDirectory,base_dir);
  AddPart(SoundDirectory,"Sound",MAX_FILE_NAME);
}

BPTR OpenSoundData(int number)
{
BPTR sound_name_file;
BPTR sound_mac_file;
BPTR fh;
char sound_name[MAX_FILE_NAME+1];
char sound_data[MAX_FILE_NAME+1];
char new_sound_name[MAX_FILE_NAME+1];
char *new_sound_ptr;
struct MacintoshMid mac_mid_data;

  sprintf(sound_name,"%s/s%d.nam",SoundDirectory,number);
  sprintf(sound_data,"%s/s%d.dat",SoundDirectory,number);

  if (sound_name_file = Open(sound_name,MODE_OLDFILE))
  {
    Seek(sound_name_file,2,OFFSET_BEGINNING);
    new_sound_ptr = new_sound_name;
    do
    {
      *new_sound_ptr = FGetC(sound_name_file);
    }
    while (*new_sound_ptr++ != 0);
    sprintf(sound_data,"%s/%s",SoundDirectory,new_sound_name);
    Close(sound_name_file);
  }
  if ((fh = Open(sound_data,MODE_OLDFILE)) != 0) return fh;

  sprintf(sound_data,"%s/s%d",SoundDirectory,number);
  if ((fh = Open(sound_data,MODE_OLDFILE)) != 0)
  {
    SampleUnsigned = 1;
    return fh;
  }

  sprintf(sound_data,"%s/m%d",SoundDirectory,number);
  if ((sound_mac_file = Open(sound_data,MODE_OLDFILE)) != 0)
  {
    SampleUnsigned = 1;
    Read(sound_mac_file,&mac_mid_data,sizeof(struct MacintoshMid));
    Close(sound_mac_file);
    sprintf(sound_data,"%s/%s",SoundDirectory,mac_mid_data.SoundFileName);
    return Open(sound_data,MODE_OLDFILE);
  }

  switch (current_game)
  {
    case 14:				/* The Lurking Horror */
      sprintf(sound_data,"%s/lurkin%.2d.snd",SoundDirectory,number);
      break;
    case 20:				/* Sherlock */
      sprintf(sound_data,"%s/sherlo%.2d.snd",SoundDirectory,number);
      break;
    default:
      return 0;
      break;
  }
  if ((fh = Open(sound_data,MODE_OLDFILE)) != 0)
  {
    SampleUnsigned = 1;
    return fh;
  }

  return 0;
}

void OpenAIFF(int length)
{
  /* Check for AIFF header */
  if ((strncmp(SampleMemory,"FORM",4) == 0) && (strncmp(SampleMemory+8,"AIFF",4) == 0))
  {
    char* data = FindAIFFChunk("COMM",length);
    if (data != 0)
    {
      short channels = *((short*)data);
      long samples = *((long*)(data+2));
      short bits = *((short*)(data+6));
      double freq = ReadExtended(data+8);

      /* Currently only 1 channel, 8 bit samples are supported */
      if ((channels == 1) && (bits == 8))
      {
	data = FindAIFFChunk("SSND",length);
	if (data != 0)
	{
	  SampleFrequency = freq;
	  SampleOffset = data - SampleMemory;
	  SampleLength = samples;
	  return;
	}
      }
    }
  }

  FreeVec(SampleMemory);
  SampleMemory = 0;
}

char *FindAIFFChunk(const char *chunk, int length)
{
  char* data = SampleMemory+12;
  while (1)
  {
    if (strncmp(data,chunk,4) == 0)
      return data+8;

    data += (*((long*)(data+4)))+8;
    if ((data - SampleMemory) >= length)
      break;
  }
  return 0;
}

static double ldexp(double x, int exp)
{
  return x*pow(2.0,exp);
}

/* 
 * Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */
#define UnsignedToFloat(u) (((double)((long)(u - 2147483647L - 1))) + 2147483648.0)
static double ReadExtended(const unsigned char *bytes)
{
  double f;
  int expon;
  unsigned long hiMant, loMant;

  expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
  hiMant = *((unsigned long*)(bytes+2));
  loMant = *((unsigned long*)(bytes+6));

  if (expon == 0 && hiMant == 0 && loMant == 0)
    f = 0;
  else
  {
    if (expon == 0x7FFF) /* Infinity or NaN */
      f = -1;
    else
    {
      expon -= 16383;
      f = ldexp(UnsignedToFloat(hiMant),expon -= 31);
      f += ldexp(UnsignedToFloat(loMant),expon -= 32);
    }
  }

  if (bytes[0] & 0x80)
    return -f;
  return f;
}

