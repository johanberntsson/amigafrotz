/*
 * amiga.c
 *
 * IO interface for the Amiga and DICE C
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

#include <proto/amigaguide.h>
#include <proto/asl.h>
#include <proto/diskfont.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <devices/timer.h>
#include <exec/exec.h>
#include <graphics/gfxmacros.h>
#include <graphics/videocontrol.h>
#include <intuition/imageclass.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "frotz.h"
#include "blorb.h"
#include "blorblow.h"
#include "amiga.h"

#define INFORMATION "\
\n\
FROTZ V2.41 - interpreter for all Infocom games. Complies with standard\n\
1.0 of Graham Nelson's specification. Written by Stefan Jokisch in 1995-7.\n\
Amiga version by David Kinder.\n\
\n\
Syntax: frotz [options] story-file\n\
\n\
  -a   watch attribute setting     -r # right margin\n\
  -A   watch attribute testing     -R   save/restore in old Frotz format\n\
  -c # context lines               -s # random number seed value\n\
  -i   ignore runtime errors       -S # transcript width\n\
  -l # left margin                 -t   set Tandy bit\n\
  -o   watch object movement       -u # slots for multiple undo\n\
  -O   watch object locating       -x   expand abbreviations g/x/z\n\
  -p   alter piracy opcode         -Z # error checking (see below)\n\
\n\
Error checking is 0 (none), 1 (report first error (default)),\n\
  2 (report all errors), 3 (exit after any error)."

static unsigned char graphic_font[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x30, 0x70, 0xFF, 0x70, 0x30, 0x00, 0x00,
  0x00, 0x0C, 0x0E, 0xFF, 0x0E, 0x0C, 0x00, 0x00,
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
  0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00,
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  0x08, 0x08, 0x08, 0xFF, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xFF, 0x08, 0x08, 0x08,
  0x08, 0x08, 0x08, 0x08, 0x0F, 0x08, 0x08, 0x08,
  0x10, 0x10, 0x10, 0x10, 0xF0, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0x10, 0x1F, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x1F, 0x10, 0x10, 0x10, 0x10,
  0x00, 0x00, 0x00, 0xF8, 0x08, 0x08, 0x08, 0x08,
  0x08, 0x08, 0x08, 0x08, 0xF8, 0x00, 0x00, 0x00,
  0x10, 0x10, 0x10, 0x10, 0x1F, 0x20, 0x40, 0x80,
  0x80, 0x40, 0x20, 0x1F, 0x10, 0x10, 0x10, 0x10,
  0x01, 0x02, 0x04, 0xF8, 0x08, 0x08, 0x08, 0x08,
  0x08, 0x08, 0x08, 0x08, 0xF8, 0x04, 0x02, 0x01,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
  0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
  0x08, 0x08, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x08, 0x08, 0x08,
  0xF8, 0xF8, 0xF8, 0xF8, 0xFF, 0xF8, 0xF8, 0xF8,
  0x1F, 0x1F, 0x1F, 0x1F, 0xFF, 0x1F, 0x1F, 0x1F,
  0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
  0x00, 0x00, 0x00, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
  0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0x00, 0x00, 0x00,
  0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x20, 0x40, 0x80,
  0x80, 0x40, 0x20, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
  0x01, 0x02, 0x04, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
  0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0x04, 0x02, 0x01,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
  0x00, 0xFF, 0x80, 0x80, 0x80, 0x80, 0xFF, 0x00,
  0x00, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0, 0xFF, 0x00,
  0x00, 0xFF, 0xE0, 0xE0, 0xE0, 0xE0, 0xFF, 0x00,
  0x00, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0x00,
  0x00, 0xFF, 0xF8, 0xF8, 0xF8, 0xF8, 0xFF, 0x00,
  0x00, 0xFF, 0xFC, 0xFC, 0xFC, 0xFC, 0xFF, 0x00,
  0x00, 0xFF, 0xFE, 0xFE, 0xFE, 0xFE, 0xFF, 0x00,
  0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
  0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
  0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00,
  0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81,
  0x08, 0x08, 0x08, 0x08, 0xFF, 0x08, 0x08, 0x08,
  0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x00,
  0x00, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x3C, 0x18,
  0x18, 0x3C, 0x7E, 0x18, 0x18, 0x7E, 0x3C, 0x18,
  0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF,
  0x00, 0x3C, 0x66, 0x06, 0x18, 0x00, 0x18, 0x00,
  0x63, 0x76, 0x5C, 0x60, 0x78, 0x66, 0x60, 0x00,
  0x78, 0x6E, 0x63, 0x7C, 0x63, 0x6E, 0x78, 0x00,
  0x00, 0x1E, 0x1B, 0x18, 0x18, 0xD8, 0x78, 0x00,
  0x63, 0x63, 0x77, 0x49, 0x77, 0x63, 0x63, 0x00,
  0x63, 0x77, 0x6B, 0x6B, 0x63, 0x63, 0x63, 0x00,
  0x49, 0x52, 0x64, 0x68, 0x70, 0x60, 0x60, 0x00,
  0x00, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00,
  0x63, 0x53, 0x4F, 0x63, 0x79, 0x65, 0x63, 0x00,
  0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00,
  0x18, 0x18, 0x7E, 0x99, 0x7E, 0x18, 0x18, 0x00,
  0x60, 0x60, 0x78, 0x6C, 0x67, 0x63, 0x63, 0x00,
  0x38, 0x3C, 0x36, 0x30, 0x30, 0x30, 0x30, 0x00,
  0x77, 0x6B, 0x77, 0x63, 0x63, 0x63, 0x63, 0x00,
  0x18, 0xD8, 0x78, 0x3C, 0x1E, 0x1B, 0x18, 0x00,
  0x71, 0x6A, 0x64, 0x71, 0x6A, 0x64, 0x60, 0x00,
  0x60, 0x60, 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x00,
  0x18, 0x18, 0x18, 0x18, 0x3C, 0x5A, 0xDB, 0x00,
  0x7C, 0x63, 0x63, 0x7C, 0x66, 0x63, 0x63, 0x00,
  0x60, 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x03, 0x00,
  0x18, 0x3C, 0x5A, 0x99, 0x18, 0x18, 0x18, 0x00,
  0x70, 0x78, 0x6C, 0x66, 0x63, 0x63, 0x63, 0x00,
  0x7E, 0x03, 0x7E, 0x03, 0x03, 0x03, 0x7C, 0x00,
  0x70, 0x6C, 0x63, 0x6C, 0x78, 0x60, 0x60, 0x00,
  0xDB, 0x5A, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x00,
  0x78, 0x64, 0x52, 0x49, 0x4F, 0x49, 0x49, 0x00,
  0x60, 0x63, 0x66, 0x6C, 0x78, 0x60, 0x60, 0x00,
  0xE7, 0xC3, 0x81, 0xE7, 0xE7, 0xE7, 0xE7, 0xFF,
  0xFF, 0xE7, 0xE7, 0xE7, 0xE7, 0x81, 0xC3, 0xE7,
  0xE7, 0xC3, 0x81, 0xE7, 0xE7, 0x81, 0xC3, 0xE7,
  0xFF, 0xC3, 0x99, 0xF9, 0xE7, 0xFF, 0xE7, 0xFF
};

USHORT ZColours[11] =
{
  0x0000,0x0E00,0x00C0,0x0EE0,0x005A,0x0F0F,0x00EE,0x0FFF,
  0x0AAA,0x0777,0x0444
};
USHORT DosZColours[16] =
{
  0x0000,0x0999,0x0FFF,0x0990,0x0049,0x0909,0x0099,0x0900,
  0x0000,0x0F00,0x00F0,0x0FF0,0x008F,0x0F0F,0x00FF,0x0090
};
int DosColoursIndex[16] =
{ 0,7,15,3,4,5,6,1,8,9,10,11,12,13,14,2 };
LONG ZMachinePens[16] =
{ -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
int UsePenTable;

USHORT CustomColours[16];
USHORT SystemColours[16];

static int current_fg = 1;
static int current_bg = 1;
static int current_style = 0;
static int current_font = 0;
static int saved_style = 0;
static int saved_font = 0;
static int user_tandy_bit = -1;
static int user_random_seed = -1;
static int cursor_state = 0;

int last_text_out = 0;
int current_game = 0;
int justify_pending = 0;

int getopt (int, char *[], const char *);

extern const char *optarg;
extern int optind;

int init_err_report_mode = -1;
int init_option_ignore_errors = -1;
int init_option_save_quetzal = -1;
int init_option_expand_abbreviations = -1;

#define HISTORY_LINES 20
unsigned char *History[HISTORY_LINES];
int HistoryPosition = HISTORY_LINES;

#define TEXTBUFFER_SIZE 256
#define QUALIFIER_SHIFT (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)
#define QUALIFIER_ALT (IEQUALIFIER_LALT|IEQUALIFIER_RALT)

struct NewMenu NewMenus[] =
 {{ NM_TITLE,"Project",0,0,0,0 },
  { NM_ITEM,"Prefs...","F",0,0,0 },
  { NM_ITEM,"Help...","H",0,0,0 },
  { NM_ITEM,"About...","?",0,0,0 },
  { NM_ITEM,NM_BARLABEL,0,0,0,0 },
  { NM_ITEM,"Quit","Q",0,0,0 },
  { NM_TITLE,"Game",0,0,0,0 },
  { NM_ITEM,"Restore...","O",0,0,"restore" },
  { NM_ITEM,"Save...","S",0,0,"save" },
  { NM_ITEM,"Restart","R",0,0,"restart" },
  { NM_ITEM,NM_BARLABEL,0,0,0,0 },
  { NM_ITEM,"Script","P",0,0,"script" },
  { NM_ITEM,"Unscript","U",0,0,"unscript" },
  { NM_ITEM,NM_BARLABEL,0,0,0,0 },
  { NM_ITEM,"Superbrief","<",0,0,"superbrief" },
  { NM_ITEM,"Brief",".",0,0,"brief" },
  { NM_ITEM,"Verbose",">",0,0,"verbose" },
  { NM_ITEM,NM_BARLABEL,0,0,0,0 },
  { NM_ITEM,"Tandy Flag",0,CHECKIT|MENUTOGGLE,0,0 },
  { NM_TITLE,"Commands",0,0,0,0 },
  { NM_ITEM,"Look","L",0,0,"look" },
  { NM_ITEM,"Inventory","I",0,0,"inventory" },
  { NM_ITEM,NM_BARLABEL,0,0,0,0 },
  { NM_ITEM,"Diagnose","D",0,0,"diagnose" },
  { NM_ITEM,"Score","C",0,0,"score" },
  { NM_ITEM,"Time","T",0,0,"time" },
  { NM_ITEM,NM_BARLABEL,0,0,0,0 },
  { NM_ITEM,"Wait","Z",0,0,"wait" },
  { NM_ITEM,"Again","G",0,0,"again" },
  { NM_ITEM,NM_BARLABEL,0,0,0,0 },
  { NM_ITEM,"Yes","Y",0,0,"yes" },
  { NM_ITEM,"No","N",0,0,"no" },
  { NM_END,0,0,0,0,0 }};

WORD ScreenPens[] = { -1 };
char Version[] = "$VER:Frotz 2.41 (25.8.2007)";
char TitleBar[] = "Frotz 2.41 Amiga Release 18";
unsigned char TextBuffer[TEXTBUFFER_SIZE];
unsigned char LengthBuffer[TEXTBUFFER_SIZE];
unsigned long TextBufferPtr;

extern struct Library *IntuitionBase;
struct Library *KeymapBase;
struct Library *AmigaGuideBase;
struct Screen *Screen;
struct Screen *DefaultPubScreen;
struct Window *Window;
struct Window *OldWindowPtr;
struct RastPort *RastPort;
struct Menu *Menus;
struct FileRequester *FileRequester;
struct Process *ThisProcess;
struct WBStartup *WBMessage;
struct DiskObject *SaveIcon;
struct List *SearchDirs;
struct List *ExcludeDirs;
struct List *GamesList;
struct Preferences FrotzPrefs;
struct MorePreferences MoreFrotzPrefs;
APTR Visual;

ULONG ForeColour = 1;
ULONG BackColour = 0;
ULONG RevForeColour = 2;
ULONG RevBackColour = 3;
ULONG InputColour = 2;
int UsingColour = 0;
int DosColours = 0;

struct TextFont *ScreenFont;
struct TextFont *TextFont, *OpenedTextFont;
struct TextFont *FixedFont, *OpenedFixedFont;
int EmphasisStyle;

struct timerequest *TimerRequest;
struct MsgPort *TimerMsgPort;
int TimerOpenCode = -1;
int TimerActive;

int ScreenWidth, ScreenHeight;
int ScaleX, ScaleY;
int TextWidth, TextHeight;
int PreviousRows;

struct GraphicsHeader
{
  unsigned char  Part;
  unsigned char  Flags;
  unsigned short Unknown1;
  unsigned short Images;
  unsigned short Link;
  unsigned char  InfoSize;
  unsigned char  Unused;
  unsigned short Checksum;
  unsigned short Unknown2;
  unsigned short Version;
};

#define PIC_NUMBER 0
#define PIC_WIDTH  2
#define PIC_HEIGHT 4
#define PIC_FLAGS  6
#define PIC_DATA   8
#define PIC_COLOUR 11

#define GFX_IBM_MCGA 1
#define GFX_IBM_CGA  2
#define GFX_AMIGA    3

char *GfxMemory;
struct GraphicsHeader *GfxHeader;
int GfxType;
unsigned short TableSeq[3840];
unsigned char TableVal[3840];

#define CONFIG_SOUND 0x20

extern int SamplePlaying, SampleLoops;
struct MsgPort *SoundMsgPort;

FILE* BlorbFile = NULL;
bb_map_t* BlorbMap = NULL;

int InputMax1,InputMax2;

#ifdef LOGGING
void Log(const char* msg, ...);
#endif

/*
 * os_beep
 *
 * Play a beep sound. Ideally, the sound should be high- (number == 1)
 * or low-pitched (number == 2).
 *
 */

void os_beep (int number)
{
  DisplayBeep(Window->WScreen);
}

/*
 * os_display_char
 *
 * Display a character of the current font using the current colours and
 * text style. The cursor moves to the next position. Printable codes are
 * all ASCII values from 32 to 126, ISO Latin-1 characters from 160 to
 * 255, ZC_GAP (gap between two sentences) and ZC_INDENT (paragraph
 * indentation). The screen should not be scrolled after printing to the
 * bottom right corner.
 *
 */

void os_display_char (zchar c)
{
#ifdef LOGGING
  Log("os_display_char()");
#endif

unsigned long bcol;

  if (h_version != V6) last_text_out = 1;

  if (current_font == GRAPHICS_FONT)
  {
    if (RastPort->DrawMode&INVERSVID)
    {
      bcol = RastPort->BgPen;
      SetBPen(RastPort,RastPort->FgPen);
    }
    SafeRectFill(RastPort,
      RastPort->cp_x,
      RastPort->cp_y,
      RastPort->cp_x+TextWidth-1,
      RastPort->cp_y+TextHeight-1,
      RastPort->BgPen);
    if (RastPort->DrawMode&INVERSVID) SetBPen(RastPort,bcol);

    if ((TextWidth == 8) && (TextHeight == 8))
    {
    int i;
    __chip static unsigned short graphics_char[8];

      for (i = 0; i < 8; i++)
	graphics_char[i] = graphic_font[((c-32)<<3)+i]<<8;
      BltPattern(RastPort,graphics_char,RastPort->cp_x,RastPort->cp_y,
	RastPort->cp_x+7,RastPort->cp_y+7,2);
    }
    else
    {
    int i,j;
    unsigned char unscaled;

      for (i = 0; i < TextHeight; i++)
      {
	unscaled = graphic_font[((c-32)<<3)+((i*7)/(TextHeight-1))];
	for (j = 0; j < TextWidth; j++)
	{
	  if (unscaled & (128>>((j*7)/(TextWidth-1))))
	    WritePixel(RastPort,RastPort->cp_x+j,RastPort->cp_y+i);
	}
      }
    }
    Move(RastPort,RastPort->cp_x+TextWidth,RastPort->cp_y);
  }
  else
  {
    if (c == ZC_INDENT)
    {
      os_display_char(' ');
      os_display_char(' ');
      os_display_char(' ');
      return;
    }
    if (c == ZC_GAP)
    {
      os_display_char(' ');
      os_display_char(' ');
      return;
    }

    if ((c >= ZC_ASCII_MIN && c <= ZC_ASCII_MAX) || (c >= ZC_LATIN1_MIN && c <= ZC_LATIN1_MAX))
    {
      if (TextBufferPtr == TEXTBUFFER_SIZE) FlushText();
      *(TextBuffer+(TextBufferPtr++)) = c;
    }
  }
}

/*
 * os_display_string
 *
 * Pass a string of characters to os_display_char.
 *
 */

void os_display_string (const zchar *s)
{
#ifdef LOGGING
  Log("os_display_string()");
#endif

zchar c;

  while ((c = *s++) != 0)
  {
    if (c == ZC_NEW_FONT || c == ZC_NEW_STYLE)
    {
      int arg = (zchar)*s++;

      if (c == ZC_NEW_FONT) os_set_font(arg);
      if (c == ZC_NEW_STYLE) os_set_text_style(arg);
    }
    else os_display_char(c);
  }
}

/*
 * os_erase_area
 *
 * Fill a rectangular area of the screen with the current background
 * colour. Top left coordinates are (1,1). The cursor does not move.
 *
 */

void os_erase_area (int top, int left, int bottom, int right)
{
#ifdef LOGGING
  Log("os_erase_area()");
#endif

  FlushText();
  if ((top > 0) && (left > 0))
  {
    if ((bottom > 0) && (right > 0))
    {
      SafeRectFill(RastPort,
	Window->BorderLeft+left-1,
	Window->BorderTop+top-1,
	Window->BorderLeft+right-1,
	Window->BorderTop+bottom-1,
	UsingColour != 0 ? RastPort->BgPen : BackColour);
    }
  }
}

/*
 * os_fatal
 *
 * Display error message and stop interpreter.
 *
 */

void os_fatal (const char *s)
{
#ifdef LOGGING
  Log("os_fatal()");
#endif

  os_stop_sample();

  if (WBMessage)
  {
    Requester(0,"Fatal error: %s","Cancel",s);
  }
  else
  {
    fputs("\nFatal error: ",stderr);
    fputs(s,stderr);
    fputs("\n",stderr);
  }

  Quit(1);
}

/*
 * os_font_data
 *
 * Return true if the given font is available. The font can be
 *
 *    TEXT_FONT
 *    PICTURE_FONT
 *    GRAPHICS_FONT
 *    FIXED_WIDTH_FONT
 *
 * The font size should be stored in "height" and "width". If
 * the given font is unavailable then these values must _not_
 * be changed.
 *
 */

int os_font_data (int font, int *height, int *width)
{
#ifdef LOGGING
  Log("os_font_data()");
#endif

  *height = h_font_height;
  *width = h_font_width;

  if (font == TEXT_FONT) return 1;
  if (font == GRAPHICS_FONT) return 1;
  if (font == FIXED_WIDTH_FONT)	return 1;
  return 0;
}

/*
 * os_read_file_name
 *
 * Return the name of a file. Flag can be one of:
 *
 *    FILE_SAVE     - Save game file
 *    FILE_RESTORE  - Restore game file
 *    FILE_SCRIPT   - Transcript file
 *    FILE_RECORD   - Command file for recording
 *    FILE_PLAYBACK - Command file for playback
 *    FILE_SAVE_AUX - Save auxilary ("preferred settings") file
 *    FILE_LOAD_AUX - Load auxilary ("preferred settings") file
 *
 * The length of the file name is limited by MAX_FILE_NAME. Ideally
 * an interpreter should open a file requester to ask for the file
 * name. If it is unable to do that then this function should call
 * print_string and read_string to ask for a file name.
 *
 */

int os_read_file_name (char *file_name, const char *default_name, int flag)
{
#ifdef LOGGING
  Log("os_read_file_name()");
#endif

FILE *fp;
char dir_buffer[MAX_FILE_NAME+1];
char file_buffer[MAX_FILE_NAME+1];
int saved_replay = istream_replay;
int saved_record = ostream_record;
int result = 0;

  istream_replay = 0;
  ostream_record = 0;

  if (flag != FILE_SCRIPT || MoreFrotzPrefs.Flags & PREFS_ALLFREQ)
  {
    if (FileRequester)
    {
    char *title;
    int save = 0;

      switch (flag)
      {
	case FILE_SAVE:
	  title = "Save Game";
	  save = 1;
	  break;
	case FILE_RESTORE:
	  title = "Restore Game";
	  break;
	case FILE_RECORD:
	  title = "Save Record";
	  save = 1;
	  break;
	case FILE_PLAYBACK:
	  title = "Play Record";
	  break;
	case FILE_SAVE_AUX:
	  title = "Save Auxiliary";
	  save = 1;
	  break;
	case FILE_LOAD_AUX:
	  title = "Load Auxiliary";
	  break;
	case FILE_SCRIPT:
	  title = "Script File";
	  save = 1;
	  break;
      }
      strcpy(dir_buffer,default_name);
      *(FilePart(dir_buffer)) = 0;
      strcpy(file_buffer,FilePart(default_name));

    int asl_return;

      SetColourScheme(0);
      asl_return = AslRequestTags(FileRequester,
	ASLFR_TitleText,title,
	ASLFR_InitialDrawer,dir_buffer,
	ASLFR_InitialFile,file_buffer,
	ASLFR_DoSaveMode,save,TAG_DONE);
      SetColourScheme(1);
      if (asl_return)
      {
	strcpy(file_name,FileRequester->fr_Drawer);
	AddPart(file_name,FileRequester->fr_File,MAX_FILE_NAME);

	if ((flag == FILE_SAVE) && (SaveIcon))
	  PutDiskObject(file_name,SaveIcon);
	result = 1;
      }
      goto finished;
    }
  }

  print_string("Enter a file name.\n");
  print_string("Default is \"");
  print_string(default_name);
  print_string("\": ");

  read_string(MAX_FILE_NAME-4,file_name);
  if (file_name[0] == 0) strcpy(file_name,default_name);

  result = 1;
  if (flag == FILE_SAVE || flag == FILE_SAVE_AUX || flag == FILE_RECORD)
  {
    if ((fp = fopen(file_name,"rb")) == NULL) goto finished;
    fclose(fp);
    result = read_yes_or_no("Overwrite existing file");
  }

finished:

  istream_replay = saved_replay;
  ostream_record = saved_record;
  return result;
}

/*
 * os_init_screen
 *
 * Initialise the IO interface. Prepare screen and other devices
 * (mouse, sound card). Set various OS depending story file header
 * entries:
 *
 *     h_config (aka flags 1)
 *     h_flags (aka flags 2)
 *     h_screen_cols (aka screen width in characters)
 *     h_screen_rows (aka screen height in lines)
 *     h_screen_width
 *     h_screen_height
 *     h_font_height (defaults to 1)
 *     h_font_width (defaults to 1)
 *     h_default_foreground
 *     h_default_background
 *     h_interpreter_number
 *     h_interpreter_version
 *     h_user_name (optional; not used by any game)
 *
 * Finally, set reserve_mem to the amount of memory (in bytes) that
 * should not be used for multiple undo and reserved for later use.
 *
 */

void os_init_screen (void)
{
#ifdef LOGGING
  Log("os_init_screen()");
#endif

static struct TextAttr new_font;
unsigned long screen;
int show_title = 1;

  ReadGamesFile();
  current_game = ScanForGame(h_release,h_serial,h_checksum);
  UsingColour = 0;
  DosColours = 0;

  if ((KeymapBase = OpenLibrary("keymap.library",37)) == 0) Quit(1);

  new_font.ta_Name = FrotzPrefs.TextFontName;
  new_font.ta_YSize = FrotzPrefs.TextFontSize;
  OpenedTextFont = OpenCorrectFont(&new_font);
  TextFont = OpenedTextFont;
  new_font.ta_Name = FrotzPrefs.FixedFontName;
  new_font.ta_YSize = FrotzPrefs.FixedFontSize;
  OpenedFixedFont = OpenCorrectFont(&new_font);
  FixedFont = OpenedFixedFont;

  if (h_version >= V5)
  {
    switch (FrotzPrefs.Colour)
    {
      case COLOUR_BZ_ONLY:
	if (story_id == BEYOND_ZORK) UsingColour = 2;
	break;
      case COLOUR_BZ_AND_V6:
	if (story_id == BEYOND_ZORK) UsingColour = 2;
	if (h_version == V6) UsingColour = 3;
	break;
      case COLOUR_ALL_GAMES:
	UsingColour = 1;
	if (story_id == BEYOND_ZORK) UsingColour = 2;
	if (h_version == V6) UsingColour = 3;
	break;
    }
    if (UsingColour == 1)
    {
      if (MoreFrotzPrefs.Flags & PREFS_MSDOS)
	DosColours = 1;
    }
  }

  if ((DefaultPubScreen = LockPubScreen(0)) == 0) Quit(1);
  screen = FrotzPrefs.CustomScreen;
  if (screen)
  {
  static struct TextAttr screen_font;
  static char screen_font_name[MAXFONTNAME];

    strcpy(screen_font_name,MoreFrotzPrefs.ScreenFontName);
    screen_font.ta_Name = screen_font_name;
    screen_font.ta_YSize = MoreFrotzPrefs.ScreenFontSize;
    ScreenFont = OpenDiskFont(&screen_font);

    if (h_version == V6 && MoreFrotzPrefs.Flags & PREFS_NO_V6_TITLE)
      show_title = 0;

    Screen = OpenScreenTags(0,
      SA_Pens,ScreenPens,
      SA_DisplayID,FrotzPrefs.ScreenMode,
      SA_Overscan,OSCAN_TEXT,
      SA_Depth,UsingColour ? 4 : 2,
      SA_Interleaved,1,
      SA_Type,CUSTOMSCREEN|AUTOSCROLL,
      SA_Font,&screen_font,
      SA_Title,TitleBar,
      SA_ShowTitle,show_title,
      SA_Behind,1,TAG_DONE);
    if (Screen == 0) Quit(1);
    GetScreenRatio(Screen);

    if (MoreFrotzPrefs.Colours[0] != 0xFFFF)
    {
      CustomColours[0] = MoreFrotzPrefs.Colours[0];
      CustomColours[1] = MoreFrotzPrefs.Colours[1];
      CustomColours[2] = MoreFrotzPrefs.Colours[2];
      CustomColours[3] = MoreFrotzPrefs.Colours[3];
    }
    else
    {
      CustomColours[0] = GetRGB4(DefaultPubScreen->ViewPort.ColorMap,0);
      CustomColours[1] = GetRGB4(DefaultPubScreen->ViewPort.ColorMap,1);
      CustomColours[2] = GetRGB4(DefaultPubScreen->ViewPort.ColorMap,2);
      CustomColours[3] = GetRGB4(DefaultPubScreen->ViewPort.ColorMap,3);
    }

    if (UsingColour)
    {
      if (DosColours)
      {
	CopyMem(DosZColours+4,CustomColours+4,8*sizeof(USHORT));
	CopyMem(CustomColours,CustomColours+12,4*sizeof(USHORT));
	CopyMem(CustomColours,SystemColours,16*sizeof(USHORT));

	CopyMem(DosZColours,CustomColours,16*sizeof(USHORT));
	LoadRGB4(&Screen->ViewPort,CustomColours,16);
      }
      else
      {
	CopyMem(ZColours,CustomColours+4,8*sizeof(USHORT));
	CopyMem(CustomColours,CustomColours+12,4*sizeof(USHORT));
	CopyMem(CustomColours,SystemColours,16*sizeof(USHORT));
	LoadRGB4(&Screen->ViewPort,CustomColours,16);
      }
    }
    else
    {
      if (MoreFrotzPrefs.Colours[0] != 0xFFFF)
	LoadRGB4(&Screen->ViewPort,CustomColours,4);
    }

  }
  else
  {
  int i;

    GetScreenRatio(DefaultPubScreen);
    if (IntuitionBase->lib_Version >= 39)
    {
      if (UsingColour)
      {
	for (i = 0; i < 16; i++)
	{
	  ZMachinePens[i] = ObtainPen(
	    DefaultPubScreen->ViewPort.ColorMap,-1,0,0,0,PEN_EXCLUSIVE);
	}
	UsePenTable = 1;
	for (i = 0; i < 16; i++)
	{
	  if (ZMachinePens[i] == -1) FreePenTable();
	}
	if (UsePenTable == 0) UsingColour = 0;
      }
    }
    else UsingColour = 0;

    if (UsingColour == 0)
      DosColours = 0;
  }

short left,top,width,height;

  if (MoreFrotzPrefs.Window[2] > 0)
  {
    left = MoreFrotzPrefs.Window[0];
    top = MoreFrotzPrefs.Window[1];
    width = MoreFrotzPrefs.Window[2];
    height = MoreFrotzPrefs.Window[3];
  }
  else
  {
    left = 0;
    top = DefaultPubScreen->BarHeight+1;
    width = ScreenWidth;
    height = ScreenHeight-DefaultPubScreen->BarHeight-1;
  }

int window_left,window_top,window_width,window_height;

  window_left = screen ? 0 : left;
  window_width = screen ? Screen->Width : width;
  if (screen)
  {
    window_top = show_title ? 2 : 0;
    window_height = show_title ? Screen->Height-2 : Screen->Height;
  }
  else
  {
    window_top = top;
    window_height = height;
  }

  if (h_version == V6 && MoreFrotzPrefs.V6Style == V6_640200)
  {
  int min_width = 640;
  int min_height = 200;

    if (screen)
    {
      if (show_title) min_height += Screen->Font->ta_YSize;
    }
    else
    {
      min_width += DefaultPubScreen->WBorLeft+DefaultPubScreen->WBorRight;
      min_height += DefaultPubScreen->WBorTop+1+
	DefaultPubScreen->Font->ta_YSize+SizeGadgetHeight(DefaultPubScreen);
    }
    window_width =
      MIN(min_width,screen ? Screen->Width : DefaultPubScreen->Width);
    window_height =
      MIN(min_height,screen ? Screen->Height : DefaultPubScreen->Height);
  }    

  if (MoreFrotzPrefs.Flags & PREFS_CENTRE_WINDOW)
  {
    if (screen)
    {
      window_left = (Screen->Width-window_width)/2;
    }
    else
    {
      window_left = (DefaultPubScreen->Width-window_width)/2;
      window_top = (DefaultPubScreen->Height-window_height)/2;
    }
  }
    
  if ((Window = OpenWindowTags(0,
    WA_Left,window_left,
    WA_Top,window_top,
    WA_Width,window_width,
    WA_Height,window_height,
    WA_SmartRefresh,1,
    WA_NewLookMenus,1,
    WA_AutoAdjust,1-screen,
    WA_Borderless,screen,
    WA_Backdrop,screen,
    WA_Activate,1-screen,
    WA_CloseGadget,1-screen,
    WA_DragBar,1-screen,
    WA_DepthGadget,1-screen,
    WA_SizeGadget,1-screen,
    WA_SizeBBottom,1-screen,
    show_title ? WA_Title : TAG_IGNORE,TitleBar,
    WA_ScreenTitle,TitleBar,
    WA_IDCMP,IDCMP_RAWKEY|IDCMP_MENUPICK|IDCMP_MOUSEBUTTONS|IDCMP_CLOSEWINDOW|IDCMP_CHANGEWINDOW,
    screen ? WA_CustomScreen : WA_PubScreen, screen ? Screen : DefaultPubScreen,
    TAG_DONE)) == 0) Quit(1);

  SetZColours();
  ThisProcess = (struct Process *)FindTask(0);
  OldWindowPtr = ThisProcess->pr_WindowPtr;
  ThisProcess->pr_WindowPtr = Window;
  ScreenToFront(Window->WScreen);
  ActivateWindow(Window);

  if ((Visual = GetVisualInfo(Window->WScreen,TAG_DONE)) == 0) Quit(1);
  if ((Menus = CreateMenus(NewMenus,GTMN_NewLookMenus,TRUE,TAG_DONE)) == 0) Quit(1);
  LayoutMenus(Menus,Visual,GTMN_NewLookMenus,TRUE,TAG_DONE);
  SetMenuStrip(Window,Menus);

  if (h_version == V3)
  {
    if (user_tandy_bit != -1)
      ItemAddress(Menus,FULLMENUNUM(1,11,0))->Flags |= CHECKED;
  }
  else OffMenu(Window,FULLMENUNUM(1,11,0));

  RastPort = Window->RPort;
  SetDrMd(RastPort,JAM2);
  SetFont(RastPort,FixedFont);
  TextWidth = RastPort->TxWidth;
  TextHeight = RastPort->TxHeight;

  if ((TimerMsgPort = CreateMsgPort()) == 0) Quit(1);
  if ((TimerRequest = CreateIORequest(TimerMsgPort,sizeof(struct timerequest))) == 0) Quit(1);
  if ((TimerOpenCode = OpenDevice("timer.device",UNIT_VBLANK,(struct IORequest *)TimerRequest,0)) != 0) Quit(1);

  if ((SoundMsgPort = CreateMsgPort()) == 0) Quit(1);

  if ((FileRequester = AllocAslRequestTags(ASL_FileRequest,
    ASLFR_Window,Window,
    ASLFR_SleepWindow,TRUE,
    ASLFR_RejectIcons,TRUE,TAG_DONE)) == 0) Quit(1);

  if (RastPort->BitMap->Depth == 1)
  {
    RevForeColour = 0;
    RevBackColour = 1;
    InputColour = 1;
  }

  EmphasisStyle = MoreFrotzPrefs.Flags & PREFS_ITALIC ?
    FSF_ITALIC : FSF_UNDERLINED;

  option_left_margin = MoreFrotzPrefs.LeftMargin;
  option_right_margin = MoreFrotzPrefs.RightMargin;

  if (h_version == V3)
  {
    h_config |= CONFIG_SPLITSCREEN;
    h_config |= CONFIG_PROPORTIONAL;
    if (user_tandy_bit != -1) h_config |= CONFIG_TANDY;
    if (h_flags & OLD_SOUND_FLAG)
    {
      if (InitializeSound() == 0)
      {
	ResetSound();
	h_flags &= ~OLD_SOUND_FLAG;
      }
    }
  }
  if (h_version >= V4)
  {
    h_config |= CONFIG_BOLDFACE;
    h_config |= CONFIG_EMPHASIS;
    h_config |= CONFIG_FIXED;
    h_config |= CONFIG_TIMEDINPUT;
    if (h_flags & SOUND_FLAG)
    {
      if (InitializeSound() == 0)
      {
	ResetSound();
	h_flags &= ~SOUND_FLAG;
      }
    }
  }
  if (h_version >= V5)
  {
    if (UsingColour)
      h_config |= CONFIG_COLOUR;
    if (h_flags & UNDO_FLAG)
    {
      if (option_undo_slots == 0) h_flags &= ~UNDO_FLAG;
    }
    if (h_flags & COLOUR_FLAG)
    {
      if (UsingColour == 0) h_flags &= ~COLOUR_FLAG;
    }
    LoadGraphicsFile();
  }
  if (h_version == V6)
  {
    h_config |= CONFIG_PICTURES|CONFIG_SOUND;
    h_flags &= ~MENU_FLAG;
  }

  if ((h_version == V6) || (DosColours == 1))
  {
    h_default_foreground = WHITE_COLOUR;
    h_default_background = BLACK_COLOUR;
  }
  else
  {
    h_default_foreground = 1;
    h_default_background = 1;
  }
  SetScreenDimensions();

  if (MoreFrotzPrefs.Flags & PREFS_MSDOS)
  {
    h_interpreter_number = INTERP_MSDOS;
    h_interpreter_version = h_version != V6 ? 'F' : 6;
  }
  else
  {
    h_interpreter_number = INTERP_AMIGA;
    h_interpreter_version = h_version != V6 ? 'C' : 3;
  }

  PreviousRows = h_screen_rows;
}

/*
 * os_more_prompt
 *
 * Display a MORE prompt, wait for a keypress and remove the MORE
 * prompt from the screen.
 *
 */

void os_more_prompt (void)
{
#ifdef LOGGING
  Log("os_more_prompt()");
#endif

int saved_font;
int saved_style;
int saved_x,saved_y;
int new_x,new_y;

  FlushText();

  saved_font = current_font;
  saved_style = current_style;

  os_set_font(TEXT_FONT);
  os_set_text_style(0);

  GetCursorPos(&saved_y,&saved_x);
  os_display_string("[MORE]");
  os_read_key(0,1);

  GetCursorPos(&new_y,&new_x);
  os_erase_area(new_y,saved_x,new_y+h_font_height,new_x);
  os_set_cursor(saved_y,saved_x);
  os_set_font(saved_font);
  if (saved_style != 0) os_set_text_style(saved_style);
}

/*
 * os_process_arguments
 *
 * Handle command line switches. Some variables may be set to activate
 * special features of Frotz:
 *
 *     option_attribute_assignment
 *     option_attribute_testing
 *     option_context_lines
 *     option_object_locating
 *     option_object_movement
 *     option_left_margin
 *     option_right_margin
 *     option_ignore_errors
 *     option_piracy
 *     option_undo_slots
 *     option_expand_abbreviations
 *     option_script_cols
 *
 * The global pointer "story_name" is set to the story file name.
 *
 */

void os_process_arguments (int argc, char *argv[])
{
#ifdef LOGGING
  Log("os_process_arguments()");
#endif

  if (IntuitionBase->lib_Version < 37) exit(0);
  LoadPreferences(1);

int num;
int c;
extern char GameFilename[];

  do
  {
    optarg = 0;
    c = getopt(argc,argv,"aAc:il:oOpr:Rs:S:tu:WxZ:");

    if (optarg != 0) num = atoi(optarg);

    switch (c)
    {
      case 'W': GetGameFilename();
		story_name = GameFilename;
		break;
      case 'a': option_attribute_assignment = 1;
		break;
      case 'A': option_attribute_testing = 1;
		break;
      case 'c': option_context_lines = num;
		break;
      case 'i': option_ignore_errors = 1;
		init_option_ignore_errors = 1;
		break;
      case 'l': option_left_margin = num;
		if (option_left_margin > 0)
		  MoreFrotzPrefs.LeftMargin = option_left_margin;
		else
		  option_left_margin = 0;
		break;
      case 'o': option_object_movement = 1;
		break;
      case 'O': option_object_locating = 1;
		break;
      case 'p': option_piracy = 1;
		break;
      case 'r': option_right_margin = num;
		if (option_right_margin > 0)
		  MoreFrotzPrefs.RightMargin = option_right_margin;
		else
		  option_right_margin = 0;
		break;
      case 'R': option_save_quetzal = 0;
		init_option_save_quetzal = 0;
		break;
      case 's': user_random_seed = num;
		break;
      case 'S': option_script_cols = num;
		break;
      case 't': user_tandy_bit = 1;
		break;
      case 'u': option_undo_slots = num;
		break;
      case 'x': option_expand_abbreviations = 1;
		init_option_expand_abbreviations = 1;
		break;
      case 'Z': if (num >= ERR_REPORT_NEVER && num <= ERR_REPORT_FATAL)
		{
		  err_report_mode = num;
		  init_err_report_mode = num;
		}
		break;
    }
  }
  while (c != EOF);

  if (optind != argc-1 && story_name == 0)
  {
    puts(INFORMATION);
    DeleteList(&SearchDirs,1,0);
    DeleteList(&ExcludeDirs,1,0);
    DeleteList(&GamesList,1,0);
    exit(1);
  }

  if (story_name == 0) story_name = argv[optind];
  LoadPreferences(0);

BPTR lock;
extern char save_name[];
extern char script_name[];
extern char command_name[];
extern char auxilary_name[];
char *file_name;
char *extension;

  if (lock = Lock(story_name,ACCESS_READ))
  {
    NameFromLock(lock,save_name,MAX_FILE_NAME);
    UnLock(lock);
  }
  else strcpy(save_name,story_name);
  file_name = FilePart(save_name);
  extension = strrchr(save_name,'.');
  if (extension > file_name) *extension = 0;

  strcpy(script_name,FilePart(save_name));
  strcpy(command_name,save_name);
  strcpy(auxilary_name,save_name);

  strcat(save_name,".Save");
  strcat(script_name,".Script");
  strcat(command_name,".Record");
  strcat(auxilary_name,".Aux");
}

/*
 * os_read_line
 *
 * Read a line of input from the keyboard into a buffer. The buffer
 * may already be primed with some text. In this case, the "initial"
 * text is already displayed on the screen. After the input action
 * is complete, the function returns with the terminating key value.
 * The length of the input should not exceed "max" characters plus
 * an extra 0 terminator.
 *
 * Terminating keys are the return key (13) and all function keys
 * (see the Specification of the Z-machine) which are accepted by
 * the is_terminator function. Mouse clicks behave like function
 * keys except that the mouse position is stored in global variables
 * "mouse_x" and "mouse_y" (top left coordinates are (1,1)).
 *
 * Furthermore, Frotz introduces some special terminating keys:
 *
 *     ZC_HKEY_PLAYBACK (Alt-P)
 *     ZC_HKEY_RECORD (Alt-R)
 *     ZC_HKEY_SEED (Alt-S)
 *     ZC_HKEY_UNDO (Alt-U)
 *     ZC_HKEY_RESTART (Alt-N, "new game")
 *     ZC_HKEY_QUIT (Alt-X, "exit game")
 *     ZC_HKEY_DEBUG (Alt-D)
 *     ZC_HKEY_HELP (Alt-H)
 *
 * If the timeout argument is not zero, the input gets interrupted
 * after timeout/10 seconds (and the return value is 0).
 *
 * The complete input line including the cursor must fit in "width"
 * screen units.
 *
 * The function may be called once again to continue after timeouts,
 * misplaced mouse clicks or hot keys. In this case the "continued"
 * flag will be set. This information can be useful if the interface
 * implements input line history.
 *
 * The screen is not scrolled after the return key was pressed. The
 * cursor is at the end of the input line when the function returns.
 *
 * Since Frotz 2.2 the helper function "completion" can be called
 * to implement word completion (similar to tcsh under Unix).
 *
 */

zchar os_read_line (int max, zchar *buf, int timeout, int width, int continued)
{
#ifdef LOGGING
  Log("os_read_line()");
#endif

int c, pos;
int inputting = 1;
unsigned long saved_pen;
zchar extension[10];
UWORD qualifier;

  cursor_state = 1;
  pos = strlen(buf);
  DrawCursor(*(buf+pos));
  InputMax1 = RastPort->cp_x+width;
  InputMax2 = MIN(InputMax1,Window->Width-Window->BorderRight);
  saved_pen = RastPort->FgPen;
  if (cwin == 0)
  {
    if (UsingColour == 0) SetAPen(RastPort,InputColour);
  }

  StartTimer(timeout);
  while (inputting)
  {
    LimitWindow(1,TextLength(RastPort,buf+pos,strlen(buf+pos)));
    c = GetKey(&qualifier);
    LimitWindow(0,0);

    switch (c)
    {
      case 27:		/* Escape */
	break;

      case 264:		/* Window Sizing */
	RedrawLine(buf,&pos,max);
	break;

      case 261:		/* Tab */
	if (pos == strlen(buf))
	{
	  if (completion(buf,extension) == 0)
	  {
	  zchar *tbuf;

	    if (tbuf = malloc(strlen(buf)+strlen(extension)+1))
	    {
	      strcpy(tbuf,buf);
	      strcat(tbuf,extension);
	      PutIntoBuffer(buf,tbuf,&pos,max);
	      free(tbuf);
	    }
	  }
	  else os_beep(1);
	}
	break;
      case ZC_BACKSPACE:	/* Backspace */
	if (pos > 0)
	{
	int deleted;

	  DrawCursor(*(buf+pos));
	  deleted = *(buf+pos-1);
	  memmove(buf+pos-1,buf+pos,strlen(buf)-pos+1);
	  MoveTextRow(-CharLength(deleted),InputMax2);
	  Move(RastPort,RastPort->cp_x-CharLength(deleted),RastPort->cp_y);
	  pos--;
	  DrawCursor(*(buf+pos));
	}
        break;

      case 260:			/* Delete */
	if (pos < strlen(buf))
	{
	int deleted;

	  DrawCursor(*(buf+pos));
	  deleted = *(buf+pos);
	  memmove(buf+pos,buf+pos+1,strlen(buf)-pos);
	  Move(RastPort,RastPort->cp_x+CharLength(deleted),RastPort->cp_y);
	  MoveTextRow(-CharLength(deleted),InputMax2);
	  Move(RastPort,RastPort->cp_x-CharLength(deleted),RastPort->cp_y);
	  DrawCursor(*(buf+pos));
	}
        break;

      case ZC_ARROW_LEFT:	/* Cursor Left */
	if (pos > 0)
	{
	  if (qualifier & QUALIFIER_SHIFT)
	  {
	    DrawCursor(*(buf+pos));
	    CursorLeftEnd(buf,&pos);
	    DrawCursor(*(buf+pos));
	  }
	  else
	  {
	    DrawCursor(*(buf+pos--));
	    Move(RastPort,RastPort->cp_x-CharLength(*(buf+pos)),RastPort->cp_y);
	    DrawCursor(*(buf+pos));
	  }
	}
	break;

      case ZC_ARROW_RIGHT:	/* Cursor Right */
	if (pos < strlen(buf))
	{
	  if (qualifier & QUALIFIER_SHIFT)
	  {
	    DrawCursor(*(buf+pos));
	    CursorRightEnd(buf,&pos);
	    DrawCursor(*(buf+pos));
	  }
	  else
	  {
	    DrawCursor(*(buf+pos));
	    Move(RastPort,RastPort->cp_x+CharLength(*(buf+pos)),RastPort->cp_y);
	    pos++;
	    DrawCursor(*(buf+pos));
	  }
	}
	break;

      case ZC_ARROW_UP:		/* Cursor Up */
	if ((cwin != 0) || (qualifier & QUALIFIER_ALT))
	{
	  if (is_terminator(c) != 0)
	  {
	    StopTimer();
	    return Terminate(buf,pos,c,saved_pen);
	  }
	}
	else
	{
	  if (qualifier & QUALIFIER_SHIFT)
	  {
	    HistoryPosition = 0;
	    while ((*(History+HistoryPosition) == 0) && (HistoryPosition < HISTORY_LINES))
	      HistoryPosition++;
	    PutIntoBuffer(buf,HistoryPosition == HISTORY_LINES ?
		(unsigned char *)"" : *(History+HistoryPosition),&pos,max);
	  }
	  else
	  {
	    if ((HistoryPosition > 0) && (*(History+HistoryPosition-1) != 0))
	    {
	      HistoryPosition--;
	      PutIntoBuffer(buf,*(History+HistoryPosition),&pos,max);
	    }
	  }
	}
	break;

      case ZC_ARROW_DOWN:	/* Cursor Down */
	if ((cwin != 0) || (qualifier & QUALIFIER_ALT))
	{
	  if (is_terminator(c) != 0)
	  {
	    StopTimer();
	    return Terminate(buf,pos,c,saved_pen);
	  }
	}
	else
	{
	  if (qualifier & QUALIFIER_SHIFT)
	  {
	    HistoryPosition = HISTORY_LINES;
	    PutIntoBuffer(buf,"",&pos,max);
	  }
	  else
	  {
	    if ((HistoryPosition < HISTORY_LINES-1) && (*(History+HistoryPosition+1) != 0))
	    {
	      HistoryPosition++;
	      PutIntoBuffer(buf,
		HistoryPosition == HISTORY_LINES ?
		  (unsigned char *)"" : *(History+HistoryPosition),&pos,max);
	    }
	    else
	    {
	      HistoryPosition = HISTORY_LINES;
	      PutIntoBuffer(buf,"",&pos,max);
	    }
	  }
	}
	break;

      default:
	if (c >= 2000)		/* Menus */
	{
	  PutIntoBuffer(buf,GTMENUITEM_USERDATA(ItemAddress(Menus,c-2000)),&pos,max);
	  c = ZC_RETURN;
	}

	if ((c >= ZC_ASCII_MIN && c <= ZC_ASCII_MAX) || (c >= ZC_LATIN1_MIN && c <= ZC_LATIN1_MAX))
	{
	  if ((strlen(buf) < max) && (FitTextLine(buf+pos,c)))
	  {
	    memmove(buf+pos+1,buf+pos,strlen(buf)-pos+1);
	    *(buf+pos) = c;

	    pos++;
	    DrawCursor(*(buf+pos));
	    MoveTextRow(CharLength(c),InputMax2);
	    os_display_char(c);
	    DrawCursor(*(buf+pos));
	  }
	}
	else
	{
	  if (is_terminator(c) != 0)
	  {
	    StopTimer();
	    return Terminate(buf,pos,c,saved_pen);
	  }
	}
	break;
    }
  }
}

/*
 * os_read_key
 *
 * Read a single character from the keyboard (or a mouse click) and
 * return it. Input aborts after timeout/10 seconds.
 *
 */

zchar os_read_key (int timeout, bool cursor)
{
#ifdef LOGGING
  Log("os_read_key()");
#endif

int key = 0;

  cursor_state = (cursor) ? 1 : 0;
  DrawCursor(0);

  do
  {
    StartTimer(timeout);
    LimitWindow(1,0);
    if (key == 264) DrawCursor(0);		/* Window Sizing */
    key = GetKey(0);
    LimitWindow(0,0);
    StopTimer();
  }
  while (key >= 256);

  DrawCursor(0);
  last_text_out = 0;
  return (key);
}

/*
 * os_reset_screen
 *
 * Reset the screen before the program ends.
 *
 */

void os_reset_screen (void)
{
#ifdef LOGGING
  Log("os_reset_screen()");
#endif

  FlushText();

  if (last_text_out != 0)
  {
    os_set_font(TEXT_FONT);
    os_set_text_style(0);
    screen_new_line();
    os_display_string("[Hit any key to exit.]");
    os_read_key(0,1);
  }

  CloseDisplay();
  DeleteList(&SearchDirs,1,0);
  DeleteList(&ExcludeDirs,1,0);
  DeleteList(&GamesList,1,0);
}

/*
 * os_scroll_area
 *
 * Scroll a rectangular area of the screen up (units > 0) or down
 * (units < 0) and fill the empty space with the current background
 * colour. Top left coordinates are (1,1). The cursor stays put.
 *
 */

void os_scroll_area (int top, int left, int bottom, int right, int units)
{
#ifdef LOGGING
  Log("os_scroll_area()");
#endif

  FlushText();
  if (units > 0)
  {
    if (Window->BorderTop+bottom > Window->Height-Window->BorderBottom)
      bottom = Window->Height-Window->BorderTop-Window->BorderBottom;

    ClipBlit(RastPort,
      Window->BorderLeft+left-1,
      Window->BorderTop+top-1+units,
      RastPort,
      Window->BorderLeft+left-1,
      Window->BorderTop+top-1,
      right-left+1,bottom-top+1-units,0xC0);
    SafeRectFill(RastPort,
      Window->BorderLeft+left-1,
      Window->BorderTop+bottom-units,
      Window->BorderLeft+right-1,
      Window->BorderTop+bottom-1,
      UsingColour != 0 ? RastPort->BgPen : BackColour);
  }
}

/*
 * os_set_colour
 *
 * Set the foreground and background colours which can be:
 *
 *     1
 *     BLACK_COLOUR
 *     RED_COLOUR
 *     GREEN_COLOUR
 *     YELLOW_COLOUR
 *     BLUE_COLOUR
 *     MAGENTA_COLOUR
 *     CYAN_COLOUR
 *     WHITE_COLOUR
 *
 *     Amiga only:
 *
 *     LIGHTGREY_COLOUR
 *     MEDIUMGREY_COLOUR
 *     DARKGREY_COLOUR
 *
 * There may be more colours in the range from 16 to 255; see the
 * remarks about os_peek_colour.
 *
 */

void os_set_colour (int new_foreground, int new_background)
{
#ifdef LOGGING
  Log("os_set_colour()");
#endif

  FlushText();
  current_fg = new_foreground;
  current_bg = new_background;

  switch (UsingColour)
  {
    case 0:
      SetAPen(RastPort,ForeColour);
      SetBPen(RastPort,BackColour);
      if (GfxType == GFX_IBM_CGA)
      {
	if (new_foreground == BLACK_COLOUR) SetAPen(RastPort,1);
	if (new_foreground == WHITE_COLOUR) SetAPen(RastPort,2);
	if (new_background == BLACK_COLOUR) SetBPen(RastPort,1);
	if (new_background == WHITE_COLOUR) SetBPen(RastPort,2);
      }
      break;
    case 1:
      if (DosColours == 1)
	SetDosColours();
      else
      {
	if (new_foreground != 1)
	{
	  if (UsePenTable)
	    SetAPen(RastPort,ZMachinePens[new_foreground-BLACK_COLOUR]);
	  else
	    SetAPen(RastPort,new_foreground-BLACK_COLOUR+4);
        }
        else SetAPen(RastPort,ForeColour);

	if (new_background != 1)
	{
	  if (UsePenTable)
	    SetBPen(RastPort,ZMachinePens[new_background-BLACK_COLOUR]);
	  else
	    SetBPen(RastPort,new_background-BLACK_COLOUR+4);
	}
	else SetBPen(RastPort,BackColour);
      }
      break;
    case 2:
      if (new_foreground == 1)
	new_foreground = WHITE_COLOUR;
      if (new_background == 1)
	new_background = BLACK_COLOUR;

      if (UsePenTable)
      {
	SetAPen(RastPort,ZMachinePens[new_foreground-BLACK_COLOUR]);
	SetBPen(RastPort,ZMachinePens[new_background-BLACK_COLOUR]);
      }
      else
      {
	SetAPen(RastPort,new_foreground-BLACK_COLOUR+4);
	SetBPen(RastPort,new_background-BLACK_COLOUR+4);
      }

      if (cwin == 0)
      {
	CustomColours[0] = CustomColours[RastPort->BgPen];
	SetColourScheme(1);
      }
      break;
    case 3:
      if (new_foreground == 1)
	new_foreground = h_default_foreground;
      if (new_background == 1)
	new_background = h_default_background;
      SetAPen(RastPort,UsePenTable ? ZMachinePens[1] : 1);
      SetBPen(RastPort,UsePenTable ? ZMachinePens[0] : 0);
      if (cwin == 0)
      {
	if (new_foreground < 16)
	  CustomColours[1] = ZColours[new_foreground-BLACK_COLOUR];
	if (new_background < 16)
	  CustomColours[0] = ZColours[new_background-BLACK_COLOUR];
	SetColourScheme(1);
	if (UsePenTable)
	{
	  SetRGB4(&(Window->WScreen->ViewPort),ZMachinePens[0],
	    (CustomColours[0]&0x0F00)>>8,
	    (CustomColours[0]&0x00F0)>>4,
	    (CustomColours[0]&0x000F));
	  SetRGB4(&(Window->WScreen->ViewPort),ZMachinePens[1],
	    (CustomColours[1]&0x0F00)>>8,
	    (CustomColours[1]&0x00F0)>>4,
	    (CustomColours[1]&0x000F));
	}
      }
      else
      {
	if (new_foreground < 16)
	{
	  if (ZColours[new_foreground-BLACK_COLOUR] == CustomColours[0])
	    SetAPen(RastPort,UsePenTable ? ZMachinePens[0] : 0);
	  if (ZColours[new_foreground-BLACK_COLOUR] == CustomColours[1])
	    SetAPen(RastPort,UsePenTable ? ZMachinePens[1] : 1);
	}
	else SetAPen(RastPort,new_foreground-16);
	if (new_background < 16)
	{
	  if (ZColours[new_background-BLACK_COLOUR] == CustomColours[0])
	    SetBPen(RastPort,UsePenTable ? ZMachinePens[0] : 0);
	  if (ZColours[new_background-BLACK_COLOUR] == CustomColours[1])
	    SetBPen(RastPort,UsePenTable ? ZMachinePens[1] : 1);
	}
	else SetBPen(RastPort,new_background-16);
      }
      break;
  }
}

/*
 * os_set_cursor
 *
 * Place the text cursor at the given coordinates. Top left is (1,1).
 *
 */

void os_set_cursor (int row, int col)
{
#ifdef LOGGING
  Log("os_set_cursor()");
#endif

  FlushText();
  if (row > h_screen_height-h_font_height+1)
    row = h_screen_height-h_font_height+1;
  if (col > h_screen_width-h_font_width+1)
    col = h_screen_width-h_font_width+1;
  Move(RastPort,Window->BorderLeft+col-1,Window->BorderTop+row-1);
}

/*
 * os_set_font
 *
 * Set the font for text output. The interpreter takes care not to
 * choose fonts which aren't supported by the interface.
 *
 */

void os_set_font (int new_font)
{
#ifdef LOGGING
  Log("os_set_font()");
#endif

  FlushText();
  current_font = new_font;
  os_set_text_style(current_style);
}

/*
 * os_set_text_style
 *
 * Set the current text style. Following flags can be set:
 *
 *     REVERSE_STYLE
 *     BOLDFACE_STYLE
 *     EMPHASIS_STYLE (aka underline aka italics)
 *     FIXED_WIDTH_STYLE
 *
 */

void os_set_text_style (int new_style)
{
#ifdef LOGGING
  Log("os_set_text_style()");
#endif

  FlushText();
  current_style = new_style;

  if (current_font == FIXED_WIDTH_FONT || new_style & FIXED_WIDTH_STYLE)
    SetFont(RastPort,FixedFont);
  else
    SetFont(RastPort,TextFont);

  if ((story_id != BEYOND_ZORK) && (UsingColour == 0))
  {
    SetAPen(RastPort,ForeColour);
    SetBPen(RastPort,BackColour);
  }
  else SetDrMd(RastPort,JAM2);
  SetSoftStyle(RastPort,FS_NORMAL,AskSoftStyle(RastPort));

  if (new_style & REVERSE_STYLE)
  {
    if ((story_id != BEYOND_ZORK) && (UsingColour == 0))
    {
      SetAPen(RastPort,RevForeColour);
      SetBPen(RastPort,RevBackColour);
    }
    else SetDrMd(RastPort,JAM2|INVERSVID);
  }

  if (DosColours == 1)
    SetDosColours();
  else
  {
    if (new_style & BOLDFACE_STYLE)
    {
      while ((RastPort->AlgoStyle & FSF_BOLD) == 0)
	SetSoftStyle(RastPort,RastPort->AlgoStyle|FSF_BOLD,
	AskSoftStyle(RastPort));
    }
  }

  if (new_style & EMPHASIS_STYLE)
  {
    while ((RastPort->AlgoStyle & EmphasisStyle) == 0)
      SetSoftStyle(RastPort,RastPort->AlgoStyle|EmphasisStyle,
      AskSoftStyle(RastPort));
  }
}

/*
 * os_string_width
 *
 * Calculate the length of a word in screen units. Apart from letters,
 * the word may contain special codes:
 *
 *    ZC_NEW_STYLE - next character is a new text style
 *    ZC_NEW_FONT  - next character is a new font
 *
 */

int os_string_width (const zchar *s)
{
#ifdef LOGGING
  Log("os_string_width()");
#endif

int length,i,c;

  if (((TextFont->tf_Flags & FPF_PROPORTIONAL) == 0) && (TextFont->tf_XSize == FixedFont->tf_XSize))
  {
    length = 0;
    for (i = 0; s[i] != 0; i++)
    {
      c = (unsigned char) s[i];
      if ((c == ZC_NEW_STYLE) || (c == ZC_NEW_FONT)) i++;
      if (c == ZC_INDENT) length += 3;
      if (c == ZC_GAP) length += 2;
      if ((c >= ZC_ASCII_MIN && c <= ZC_ASCII_MAX) || (c >= ZC_LATIN1_MIN && c <= ZC_LATIN1_MAX))
	length++;
    }
    return length*TextWidth;
  }
  else
  {
    length = 0;
    for (i = 0; s[i] != 0; i++)
    {
      c = (unsigned char) s[i];

      if (c == ZC_INDENT)
      {
	*(LengthBuffer+length++) = ' ';
	*(LengthBuffer+length++) = ' ';
	c = ' ';
      }
      if (c == ZC_GAP)
      {
	*(LengthBuffer+length++) = ' ';
	c = ' ';
      }

      if ((c == ZC_NEW_STYLE) || (c == ZC_NEW_FONT))
      {
	i++;
      }
      else
      {
	if (c >= 32)
	{
	  if (length < TEXTBUFFER_SIZE) *(LengthBuffer+length++) = c;
	}
      }
    }
    return TextLength(RastPort,LengthBuffer,length);
  }
}

/*
 * os_char_width
 *
 * Return the length of the character in screen units.
 *
 */

int os_char_width (zchar c)
{
#ifdef LOGGING
  Log("os_char_width()");
#endif

char s[2];

  s[0] = c;
  s[1] = 0;
  return os_string_width(s);
}

/*
 * os_peek_colour
 *
 * Return the colour of the screen unit below the cursor. (If the
 * interface uses a text mode, it may return the background colour
 * of the character at the cursor position instead.) This is used
 * when text is printed on top of pictures. Note that this coulor
 * need not be in the standard set of Z-machine colours. To handle
 * this situation, Frotz entends the colour scheme: Colours above
 * 15 (and below 256) may be used by the interface to refer to non
 * standard colours. Of course, os_set_colour must be able to deal
 * with these colours.
 *
 */

int os_peek_colour (void)
{
#ifdef LOGGING
  Log("os_peek_colour()");
#endif

ULONG pen,colour;
int i;

  pen = ReadPixel(RastPort,RastPort->cp_x,RastPort->cp_y);
  if (UsingColour == 3)
  {
    colour = GetRGB4(Window->WScreen->ViewPort.ColorMap,pen);
    if (pen < 2)
    {
      for (i = 0; i < 11; i++)
	if (colour == ZColours[i]) return i+BLACK_COLOUR;
    }
    return pen+16;
  }
  else
  {
    switch (pen)
    {
      case 0:
	return h_default_background;
      case 1:
	return BLACK_COLOUR;
      case 2:
	return WHITE_COLOUR;
      default:
	return h_default_foreground;
    }
  }
}

/*
 * os_picture_data
 *
 * Return true if the given picture is available. If so, store the
 * picture width and height in the appropriate variables. Picture
 * number 0 is a special case: Write the highest legal picture number
 * and the picture file release number into the height and width
 * variables respectively when this picture number is asked for.
 *
 */

int os_picture_data (int picture, int *height, int *width)
{
#ifdef LOGGING
  Log("os_picture_data()");
#endif

char *info;

  if (GfxMemory)
  {
    if (picture == 0)
    {
      *height = ByteSwap(GfxHeader->Images);
      *width = ByteSwap(GfxHeader->Version);
      return 1;
    }
    if (info = FindPicture(picture))
    {
      *height = ByteSwap(*(short *)(info+PIC_HEIGHT));
      *width = ByteSwap(*(short *)(info+PIC_WIDTH));

      if (GfxType == GFX_IBM_MCGA || GfxType == GFX_AMIGA)
	*width *= 2;

      return 1;
    }
  }
  *height = 0;
  *width = 0;
  return 0;
}

/*
 * os_draw_picture
 *
 * Display a picture at the given coordinates.
 *
 */

void os_draw_picture (int picture, int y, int x)
{
#ifdef LOGGING
  Log("os_draw_picture()");
#endif

char *info;

  if (UsingColour == 0 && GfxType != GFX_IBM_CGA) return;
  if (GfxMemory == 0) return;
  if ((info = FindPicture(picture)) == 0) return;
  x += Window->BorderLeft-1;
  y += Window->BorderTop-1;

unsigned char buf[512];
long data, colour;
unsigned short code, prev_code, val;
short width, height, flags;
short count, bits, bits_per_code, bits_left, pos, iwidth;
int i, j, ypos = -1, alloc = 0, depth;
struct BitMap bitmap;
PLANEPTR mask;

  data = (long)((*(unsigned char *)(info+PIC_DATA))<<16)+
	 (long)((*(unsigned char *)(info+PIC_DATA+1))<<8)+
	 (long)(*(unsigned char *)(info+PIC_DATA+2));
  width = ByteSwap(*(short *)(info+PIC_WIDTH));
  height = ByteSwap(*(short *)(info+PIC_HEIGHT));
  flags = ByteSwap(*(short *)(info+PIC_FLAGS));

  if (GfxHeader->InfoSize >= 14)
  {
    colour = (long)((*(unsigned char *)(info+PIC_COLOUR))<<16)+
	     (long)((*(unsigned char *)(info+PIC_COLOUR+1))<<8)+
	     (long)(*(unsigned char *)(info+PIC_COLOUR+2));
    if (colour != 0)
    {
      count = *((unsigned char *)(GfxMemory+colour++));
      if (Screen)
      {
	for (i = 0; i < 14; i++)
	{
	  CustomColours[2+i] =
	    (((int)*((unsigned char *)(GfxMemory+colour))>>4)<<8)+
	    (((int)*((unsigned char *)(GfxMemory+colour+1))>>4)<<4)+
	    ((int)*((unsigned char *)(GfxMemory+colour+2))>>4);
	  colour+=3;
	}
	LoadRGB4(&Screen->ViewPort,CustomColours,16);
      }
      else
      {
      ULONG r,g,b;

	for (i = 0; i < 14; i++)
	{
	  r = (int)*((unsigned char *)(GfxMemory+colour))>>4;
	  g = (int)*((unsigned char *)(GfxMemory+colour+1))>>4;
	  b = (int)*((unsigned char *)(GfxMemory+colour+2))>>4;
	  SetRGB4(&(Window->WScreen->ViewPort),ZMachinePens[2+i],r,g,b);
	  colour+=3;
	}
      }
    }
  }

  if (GfxType == GFX_AMIGA) return;
  iwidth = width;
  if (GfxType == GFX_IBM_MCGA) iwidth *= 2;

  depth = UsePenTable ? 8 : 4;
  InitBitMap(&bitmap,depth,iwidth,height);
  for (i = 0; i < depth; i++)
  {
    if (bitmap.Planes[i] = AllocRaster(iwidth,height))
    {
      BltClear(bitmap.Planes[i],RASSIZE(iwidth,height),0);
      alloc++;
    }
  }
  if (mask = AllocRaster(iwidth,height))
  {
    BltClear(mask,RASSIZE(iwidth,height),0);
    alloc++;
  }
  WaitBlit();

  if (alloc == depth+1)
  {
    bits_per_code = 9;
    bits_left = 0;
    pos = 9999;

    for (;;)
    {
      code = 0;
      i = bits_per_code;
      do
      {
	if (bits_left <= 0)
	{
	  bits = *((unsigned char *)(GfxMemory+(data++)));
	  bits_left = 8;
	}
	code |= (bits>>(8-bits_left))<<(bits_per_code-i);
	i -= bits_left;
	bits_left = -i;
      }
      while (i > 0);

      code &= 0xFFF>>(12-bits_per_code);
      if (code == 256)
      {
	bits_per_code = 9;
	count = 257;
	continue;
      }
      if (code == 257) break;
      val = code;
      i = 0;
      if (code == count)
      {
 	val = prev_code;
	i = 1;
      }
      while (val > 255)
      {
	buf[i++] = TableVal[val-256];
	val = TableSeq[val-256];
      }
      buf[i] = val;
      if (code == count) buf[0] = val;
      TableVal[count-256] = val;
      TableSeq[count-256] = prev_code;
      if (++count == (1<<bits_per_code) && bits_per_code < 12)
	bits_per_code++;

      do
      {
      int bitpos;

	if (pos >= width)
	{
	  ypos++;
	  bitpos = ypos*bitmap.BytesPerRow;
	  pos = 0;
	}
	val = buf[i--];

	if (GfxType == GFX_IBM_CGA)
	{
	int black,white;

	  black = (UsingColour && Screen) ? 4 : 1;
	  white = (UsingColour && Screen) ? WHITE_COLOUR-BLACK_COLOUR+4 : 2;

	  if (flags & 1)
	  {
	    if (val)
	    {
	    int bitposx;

	      switch (val)
	      {
		case 2:
		  val = white;
		  break;
		case 3:
		  val = black;
		  break;
	      }

	      bitposx = bitpos+(pos>>3);
	      for (j = 0; j < depth; j++)
	      {
		if (val & (1<<j))
		  *(bitmap.Planes[j]+bitposx) |= 128>>(pos&7);
	      }
	    }
	    pos++;
	  }
	  else
	  {
	  int bitposx;

	    bitposx = bitpos+(pos>>3);
	    for (j = 0; j < depth; j++)
	    {
	      if (black&(1<<j)) *(bitmap.Planes[j]+bitposx) |= ~val;
	      if (white&(1<<j)) *(bitmap.Planes[j]+bitposx) |= val;
	    }
	    pos+=8;
	  }

	}
	else
	{
	  if (val)
	  {
	  int bitposx,xbit = pos*2;

	    if (UsePenTable) val = ZMachinePens[val];
	    bitposx = bitpos+(xbit>>3);
	    for (j = 0; j < depth; j++)
	    {
	      if (val & (1<<j))
		*(bitmap.Planes[j]+bitposx) |= (128+64)>>(xbit&7);
	    }
	  }
	  pos++;
	}

      }
      while (i >= 0);
      prev_code = code;
    }
    for (i = 0; i < RASSIZE(iwidth,height); i++)
    {
      for (j = 0; j < depth; j++) *(mask+i) |= *(bitmap.Planes[j]+i);
    }
    BltMaskBitMapRastPort(&bitmap,0,0,RastPort,x,y,iwidth,height,0xE0,mask);
    WaitBlit();
    if ((Window->Flags & WFLG_BORDERLESS) == 0)
    {
      if ((x+iwidth > Window->Width-Window->BorderRight) || (y+height > Window->Height-Window->BorderBottom))
	RefreshWindowFrame(Window);
    }
  }

  for (i = 0; i < depth; i++)
  {
    if (bitmap.Planes[i])
      FreeRaster(bitmap.Planes[i],iwidth,height);
  }
  if (mask) FreeRaster(mask,iwidth,height);
}

/*
 * os_random_seed
 *
 * Return an appropriate random seed value in the range from 0 to
 * 32767, possibly by using the current system time.
 *
 */

int os_random_seed (void)
{
#ifdef LOGGING
  Log("os_random_seed()");
#endif

  if (user_random_seed == -1) return clock()&32767;
  return user_random_seed;
}

/*
 * os_restart_game
 *
 * This routine allows the interface to interfere with the process of
 * restarting a game at various stages:
 *
 *     RESTART_BEGIN - restart has just begun
 *     RESTART_WPROP_SET - window properties have been initialised
 *     RESTART_END - restart is complete
 *
 */

void os_restart_game (int stage)
{
#ifdef LOGGING
  Log("os_restart_game()");
#endif

  if (stage == RESTART_BEGIN)
  {
  USHORT cols[16];
  int x,y;

    CheckReset();

    if (UsingColour && (story_id == BEYOND_ZORK))
    {
      if (os_picture_data(1,&x,&y))
      {
	CopyMem(CustomColours,cols,16*sizeof(USHORT));
	os_set_colour(WHITE_COLOUR,BLACK_COLOUR);
	SafeRectFill(RastPort,
	  Window->BorderLeft,Window->BorderTop,
	  Window->BorderLeft+h_screen_width-1,
	  Window->BorderTop+h_screen_height-1,0);
	os_draw_picture(1,1,1);
	os_read_key(0,0);
	CopyMem(cols,CustomColours,16*sizeof(USHORT));
	SetZColours();
      }
    }
  }
}

/*
 * os_path_open
 *
 * Open a file in the current directory.
 *
 */

FILE *os_path_open (const char *name, const char *mode, long *size)
{
#ifdef LOGGING
  Log("os_path_open()");
#endif

FILE *fp;

  fp = fopen(name, mode);
  if (fp != NULL)
  {
    if (BlorbMap == NULL)
    {
      if (bb_create_map(fp,&BlorbMap) == bb_err_None)
      {
      bb_result_t result;

	if (bb_load_resource(BlorbMap,bb_method_FilePos,&result,bb_ID_Exec,0) == bb_err_None)
	{
	  unsigned int id = BlorbMap->chunks[result.chunknum].type;
	  if (id == bb_make_id('Z','C','O','D'))
	  {
	    fseek(fp,result.data.startpos,SEEK_SET);
	    *size = result.length;
	    return fp;
	  }
	}
      }
      fseek(fp,0,SEEK_SET);
      *size = -1;
    }
  }
  return fp;
}

void SafeRectFill(struct RastPort *rp,long xMin,long yMin,long xMax,long yMax,unsigned long pen)
{
unsigned long saved_pen;

  saved_pen = rp->FgPen;
  SetAPen(rp,pen);
  if (xMax > Window->Width-Window->BorderRight-1) xMax = Window->Width-Window->BorderRight-1;
  if (yMax > Window->Height-Window->BorderBottom-1) yMax = Window->Height-Window->BorderBottom-1;
  RectFill(rp,xMin,yMin,xMax,yMax);
  SetAPen(rp,saved_pen);
}

void FlushText(void)
{
static int semaphore;

  if (TextBufferPtr < 1) return;
  if (semaphore) return;
  semaphore = 1;

  Move(RastPort,RastPort->cp_x,RastPort->cp_y+RastPort->TxBaseline);

  if (justify_pending)
  {
  int i,j,len,spaces;

    for (i = 0, spaces = 0; i < TextBufferPtr; i++)
      if (*(TextBuffer+i) == ' ') spaces++;

    len = h_screen_width-option_right_margin-option_left_margin+
      Window->BorderLeft-RastPort->cp_x-
      TextLength(RastPort,TextBuffer,TextBufferPtr)+
      (spaces*TextLength(RastPort," ",1));
    if (RastPort->AlgoStyle & FSF_BOLD)
      len -= RastPort->Font->tf_BoldSmear;
    if (*(TextBuffer+TextBufferPtr-1) == ' ') spaces--;

    for (i = 0, j = 0; i < TextBufferPtr; i++)
    {
      if (*(TextBuffer+i) == ' ')
      {
	if (i-j > 0) Text(RastPort,TextBuffer+j,i-j);
	j = i+1;
	Move(RastPort,RastPort->cp_x+(len/spaces),RastPort->cp_y);
	len -= len/spaces;
	spaces--;
      }
    }
    if (i-j > 0) Text(RastPort,TextBuffer+j,i-j);

    justify_pending = 0;
  }
  else Text(RastPort,TextBuffer,TextBufferPtr);

  Move(RastPort,RastPort->cp_x,RastPort->cp_y-RastPort->TxBaseline);
  TextBufferPtr = 0;

  if ((Window->Flags & WFLG_BORDERLESS) == 0)
  {
    if (RastPort->cp_x >= Window->Width-Window->BorderRight)
      RefreshWindowFrame(Window);
  }

  semaphore = 0;
}

int GetKey(UWORD *qualifier_addr)
{
struct IntuiMessage *imsg;
ULONG class, port_signals;
UWORD code, qualifier;
int i,old_height;

static unsigned long old_sec = 0,old_mic = 0;
static unsigned long new_sec = 0,new_mic = 0;

  FlushText();
  port_signals = PORTSIG(Window->UserPort)|
		 PORTSIG(TimerMsgPort)|
		 PORTSIG(SoundMsgPort);

  while(1)
  {
    while(imsg = (struct IntuiMessage *)GetMsg(Window->UserPort))
    {
      class = imsg->Class;
      code = imsg->Code;
      qualifier = imsg->Qualifier;
      mouse_x = imsg->MouseX-Window->BorderLeft+1;
      mouse_y = imsg->MouseY-Window->BorderTop+1;
      new_sec = imsg->Seconds;
      new_mic = imsg->Micros;
      if (class == IDCMP_MENUVERIFY) SetColourScheme(0);
      ReplyMsg((struct Message *)imsg);
      if (qualifier_addr) *qualifier_addr = qualifier;
      switch(class)
      {
	case IDCMP_RAWKEY:
	  switch (code)
	  {
	    case 0x4C:					/* Cursor Up */
	      return ZC_ARROW_UP;
	    case 0x4D:					/* Cursor Down */
	      return ZC_ARROW_DOWN;
	    case 0x4F:					/* Cursor Left */
	      return ZC_ARROW_LEFT;
	    case 0x4E:					/* Cursor Right */
	      return ZC_ARROW_RIGHT;
	    case 0x5F:					/* Help */
	      HelpGuide();
	      break;
	    default:
	      if (code >= 0x50 && code <= 0x59)		/* Function Keys */
		return code - 0x50 + ZC_FKEY_MIN;

	    static struct InputEvent ie;
	    unsigned char ascii_buffer[1];

	      if (code & IECODE_UP_PREFIX) break;
	      ie.ie_Class = IECLASS_RAWKEY;
	      ie.ie_Prev2DownCode = ie.ie_Prev1DownCode;
	      ie.ie_Prev2DownQual = ie.ie_Prev1DownQual;
	      ie.ie_Prev1DownCode = ie.ie_Code;
	      ie.ie_Prev1DownQual = ie.ie_Qualifier;
	      ie.ie_Code = code;
	      ie.ie_Qualifier = 0;
	      if (MapRawKey(&ie,ascii_buffer,1,0))
	      {
		switch (*ascii_buffer)
		{
		  case 'd':
		    if (qualifier & QUALIFIER_ALT) return ZC_HKEY_DEBUG;
		    break;
		  case 'h':
		    if (qualifier & QUALIFIER_ALT) return ZC_HKEY_HELP;
		    break;
		  case 'n':
		    if (qualifier & QUALIFIER_ALT) return ZC_HKEY_RESTART;
		    break;
		  case 'p':
		    if (qualifier & QUALIFIER_ALT) return ZC_HKEY_PLAYBACK;
		    break;
		  case 'r':
		    if (qualifier & QUALIFIER_ALT) return ZC_HKEY_RECORD;
		    break;
		  case 's':
		    if (qualifier & QUALIFIER_ALT) return ZC_HKEY_SEED;
		    break;
		  case 'u':
		    if (qualifier & QUALIFIER_ALT) return ZC_HKEY_UNDO;
		    break;
		  case 'x':
		    if (qualifier & QUALIFIER_ALT) return ZC_HKEY_QUIT;
		    break;
		}
	      }

	      ie.ie_Qualifier = qualifier;
	      if (MapRawKey(&ie,ascii_buffer,1,0))
	      {
		code = (int)(*ascii_buffer);
		switch (code)
		{
		  case 8:
		    return ZC_BACKSPACE;
		  case 9:
		    return 261;
		  case 13:
		    return ZC_RETURN;
		  case 27:
		    return ZC_ESCAPE;
		  case 127:
		    return 260;
		  default:
		    if (qualifier & IEQUALIFIER_NUMERICPAD)
		    {					/* Numeric Keypad */
		      if (code >= '0' && code <= '9') return code - '0' + ZC_NUMPAD_MIN;
		    }
		    if ((code >= ZC_ASCII_MIN && code <= ZC_ASCII_MAX))
		      return code;
		    if ((code >= ZC_LATIN1_MIN && code <= ZC_LATIN1_MAX))
		      return code;
		}
	      }
	      break;
	  }
	  break;
	case IDCMP_MENUPICK:
	  SetColourScheme(1);
	  i = ProcessMenus(code);
	  if (i > 0) return i;
	  break;
	case IDCMP_MOUSEBUTTONS:
	  if (code == SELECTDOWN)
	  {
	    BOOL double = DoubleClick(old_sec,old_mic,new_sec,new_mic);
	    old_sec = new_sec;
	    old_mic = new_mic;
	    return double ? ZC_DOUBLE_CLICK : ZC_SINGLE_CLICK;
	  }
	  break;
	case IDCMP_CLOSEWINDOW:
	  Quit(0);
	  break;
	case IDCMP_CHANGEWINDOW:
	  old_height = h_screen_height;
	  SetScreenDimensions();
	  resize_screen();
	  InputMax2 = MIN(InputMax1,Window->Width-Window->BorderRight);

	  if (PreviousRows > h_screen_rows)
	  {
	    SafeRectFill(RastPort,
	      Window->BorderLeft,
	      Window->BorderTop+(h_screen_rows*TextHeight),
	      Window->Width-Window->BorderRight-1,
	      Window->Height-Window->BorderBottom-1,BackColour);

	    if (RastPort->cp_y+TextHeight > Window->Height-Window->BorderBottom)
	    {
	      Move(RastPort,RastPort->cp_x,Window->BorderTop+((h_screen_rows-1)*TextHeight));
	      SafeRectFill(RastPort,
		Window->BorderLeft,
		RastPort->cp_y,
		Window->Width-Window->BorderRight-1,
		RastPort->cp_y+TextHeight-1,BackColour);
	      PreviousRows = h_screen_rows;
	      return 264;				/* Window Sizing */
	    }
	  }
	  PreviousRows = h_screen_rows;
	  break;
      }
    }
    if (GetMsg(TimerMsgPort))
    {
      TimerActive = 0;
      return ZC_TIME_OUT;
    }
    while (GetMsg(SoundMsgPort))
    {
      if (SampleLoops > 0)
	SampleLoops--;
      if (SampleLoops == 0)
      {
	end_of_sound();
	SamplePlaying = 0;
      }
    }

    ModifyIDCMP(Window,Window->IDCMPFlags | IDCMP_MENUVERIFY);
    Wait(port_signals);
    ModifyIDCMP(Window,Window->IDCMPFlags & ~IDCMP_MENUVERIFY);
  }
}

void DrawCursor(int under)
{
int size;
BYTE saved_draw_mode;

  FlushText();
  if (cursor_state == 0) return;
  size = (under == 0) ? RastPort->TxWidth : CharLength(under);

  saved_draw_mode = RastPort->DrawMode;
  SetDrMd(RastPort,COMPLEMENT);
  if (UsingColour == 3) SetWrMsk(RastPort,0x01);

  SafeRectFill(RastPort,
    RastPort->cp_x,
    RastPort->cp_y,
    RastPort->cp_x+size-1,
    RastPort->cp_y+TextHeight-1,0);

  SetDrMd(RastPort,saved_draw_mode);
  if (UsingColour == 3) SetWrMsk(RastPort,0xFF);
}

void ShowAbout(void)
{
char *title, *after_title;
char *authors, *before_authors, *after_authors;
char *level;

  title = GetGameTitle(current_game);
  authors = GetGameAuthors(current_game);
  level = GetGameLevel(current_game);

  after_title = (strcmp(title,"") == 0) ? "" : "\n";
  if (strcmp(authors,"") == 0)
  {
    before_authors = "";
    after_authors = (strcmp(title,"") == 0) ? "" : "\n";
  }
  else
  {
    before_authors = "Written by ";
    after_authors = (strcmp(level,"") == 0) ? "\n\n" : "\n";
  }

  Requester(Window,
    "%s%s%s%s%s%s"
    "Frotz 2.41 Standard 1.0 Infocom Interpreter\n"
    "Copyright 1995-2001 by Stefan Jokisch and Jim Dunleavy\n"
    "Amiga Release 18 by David Kinder",
    "Continue",
    title,after_title,before_authors,authors,after_authors,level);
}

int CharLength(int c)
{
unsigned char buffer;

  if (c < 0) c += 256;
  buffer = (unsigned char)c;
  return TextLength(RastPort,&buffer,1);
}

void MoveTextRow(int offset,int max)
{
int xSource,xDest;

  xSource = RastPort->cp_x;
  xDest = RastPort->cp_x+offset;
  ClipBlit(RastPort,
    xSource,
    RastPort->cp_y,
    RastPort,
    xDest,
    RastPort->cp_y,
    max-MAX(xSource,xDest),
    TextHeight,0xC0);
}

void CursorLeftEnd(unsigned char *buffer,int *pos)
{
  while (*pos > 0)
  {
    (*pos)--;
    Move(RastPort,RastPort->cp_x-CharLength(*(buffer+*pos)),RastPort->cp_y);
  }
}

void CursorRightEnd(unsigned char *buffer,int *pos)
{
  while (*pos < strlen(buffer))
  {
    Move(RastPort,RastPort->cp_x+CharLength(*(buffer+*pos)),RastPort->cp_y);
    (*pos)++;
  }
}

int FitTextLine(unsigned char *pointer,int new)
{
int a,b;

  a = TextLength(RastPort,pointer,strlen(pointer))+CharLength(new)+
    RastPort->cp_x+RastPort->TxWidth;
  b = Window->BorderLeft+h_screen_width-
    option_right_margin-option_left_margin;
  return (a > b) ? 0 : 1;
}

void Quit(int code)
{
  last_text_out = 0;
  os_reset_screen();
  exit(code);
}

void StartTimer(int timeout)
{
  if ((TimerActive == 0) && (timeout > 0))
  {
    TimerRequest->tr_node.io_Command = TR_ADDREQUEST;
    TimerRequest->tr_node.io_Message.mn_ReplyPort = TimerMsgPort;
    TimerRequest->tr_time.tv_secs = timeout/10;
    TimerRequest->tr_time.tv_micro = (timeout-(TimerRequest->tr_time.tv_secs*10))*100000;
    SendIO((struct IORequest *)TimerRequest);
    TimerActive = 1;
  }
}

void StopTimer(void)
{
  if (TimerActive != 0)
  {
    if (GetMsg(TimerMsgPort) == 0)
    {
      AbortIO((struct IORequest *)TimerRequest);
      WaitIO((struct IORequest *)TimerRequest);
    }
    TimerActive = 0;
  }
}

void StoreInHistory(unsigned char *line)
{
int i;

  if ((*line != 0) && (SafeCmp(*(History+HISTORY_LINES-1),line) != 0))
  {
    if (*History) FreeVec(*History);
    for (i = 0; i < HISTORY_LINES-1; i++) *(History+i) = *(History+i+1);
    if (*(History+HISTORY_LINES-1) = AllocVec(strlen(line)+1,MEMF_CLEAR))
      strcpy(*(History+HISTORY_LINES-1),line);
  }
  HistoryPosition = HISTORY_LINES;
}

int SafeCmp(const char *a,const char *b)
{
  if ((a == 0) || (b == 0)) return 1;
  return strcmp(a,b);
}

void PutIntoBuffer(unsigned char *buffer,unsigned char *new,int *pos,int max_size)
{
  if (new == 0) return;

  DrawCursor(*(buffer+*pos));

int saved_x_position;
int saved_colour;
int i;

  CursorLeftEnd(buffer,pos);
  saved_colour = RastPort->FgPen;
  saved_x_position = RastPort->cp_x;
  SetAPen(RastPort,RastPort->BgPen);
  for (i = 0; i < strlen(buffer); i++) os_display_char(*(buffer+i));
  FlushText();
  SetAPen(RastPort,saved_colour);
  Move(RastPort,saved_x_position,RastPort->cp_y);

  *buffer = 0;

  while(1)
  {
    if ((FitTextLine(buffer,*(new+*pos)) == 0) || (*(new+*pos) == 0) || (*pos == max_size))
    {
      for (i = 0; i < strlen(buffer); i++) os_display_char(*(buffer+i));
      FlushText();
      DrawCursor(*(buffer+*pos));
      return;
    }
    else
    {
      *(buffer+*pos) = *(new+*pos);
      (*pos)++;
      *(buffer+*pos) = 0;
    }
  }
}

void RedrawLine(unsigned char *buffer,int *pos,int max_size)
{
char *current_buffer;

  DrawCursor(*(buffer+*pos));
  if ((current_buffer = AllocVec(max_size,MEMF_CLEAR)) == 0) return;
  strcpy(current_buffer,buffer);
  PutIntoBuffer(buffer,current_buffer,pos,max_size);
  FreeVec(current_buffer);
}

int ProcessMenus(UWORD menu_number)
{
struct MenuItem *menu_item;

  while (menu_number != MENUNULL)
  {
    switch (MENUNUM(menu_number))
    {
      case 0:
	switch (ITEMNUM(menu_number))
	{
	  case 0:
	    if (OpenPreferences()) return ZC_HKEY_RESTART;
	    break;
	  case 1:
	    HelpGuide();
	    break;
	  case 2:
	    ShowAbout();
	    break;
	  case 4:
	    Quit(0);
	    break;
	}
	break;
      case 1:
	if (ITEMNUM(menu_number) == 11)
	{
	  if (h_version == V3)
	  {
	    if (ItemAddress(Menus,menu_number)->Flags & CHECKED)
	    {
	      h_config |= CONFIG_TANDY;
	      SET_BYTE(H_CONFIG,h_config)
	      user_tandy_bit = 1;
	    }
	    else
	    {
	      h_config &= ~CONFIG_TANDY;
	      SET_BYTE(H_CONFIG,h_config)
	      user_tandy_bit = -1;
	    }
	  }
	}
	else return (menu_number+2000);
	break;
      case 2:
	return (menu_number+2000);
	break;
    }
    menu_item = ItemAddress(Menus,menu_number);
    menu_number = menu_item->NextSelect;
  }
  return 0;
}

int Terminate(unsigned char *buffer,int pos,int c,int pen)
{
  DrawCursor(*(buffer+pos));
  CursorRightEnd(buffer,&pos);
  SetAPen(RastPort,pen);
  if (c == ZC_RETURN) StoreInHistory(buffer);
  last_text_out = 0;
  return c;
}

void HelpGuide(void)
{
struct NewAmigaGuide frotz_guide =
  { 0,"Frotz.guide",0,0,0,0,0,0,0,0,0,0,0 };

  frotz_guide.nag_Screen = Window->WScreen;
  if (AmigaGuideBase == 0)
    AmigaGuideBase = OpenLibrary("amigaguide.library",34);
  if (AmigaGuideBase != 0)
  {
    SetColourScheme(0);
    CloseAmigaGuide(OpenAmigaGuide(&frotz_guide,TAG_DONE));
    SetColourScheme(1);
  }
}

void WaitForMsg(struct MsgPort *port)
{
  WaitPort(port);
  GetMsg(port);
}

void BusyPointer(struct Window *window,int busy)
{
  if ((IntuitionBase->lib_Version >= 39) && (window))
    SetWindowPointer(window,WA_BusyPointer,busy,TAG_DONE);
}

LONG Requester(struct Window *window,UBYTE *text,UBYTE *gadgets,...)
{
va_list arguments;
LONG return_value;
static struct EasyStruct requester =
  { sizeof(struct EasyStruct),0,"Frotz",0,0 };

  requester.es_TextFormat = text;
  requester.es_GadgetFormat = gadgets;
  va_start(arguments,gadgets);
  SetColourScheme(0);
  BusyPointer(window,1);
  return_value = EasyRequestArgs(window,&requester,0,arguments);
  BusyPointer(window,0);
  SetColourScheme(1);
  va_end(arguments);
  return return_value;
}

void CloseDisplay(void)
{
int i;

  FlushText();
  last_text_out = 0;

  if (h_version == V3)
  {
    if (h_flags & OLD_SOUND_FLAG) ResetSound();
  }
  if (h_version >= V4)
  {
    if ((h_flags & SOUND_FLAG) || (h_config & CONFIG_SOUND)) ResetSound();
  }

  for (i = 0; i < HISTORY_LINES; i++)
  {
    if (*(History+i)) FreeVec(*(History+i));
    *(History+i) = 0;
  }

  StopTimer();
  if (TimerOpenCode == 0) CloseDevice((struct IORequest *)TimerRequest);
  if (TimerRequest) DeleteIORequest(TimerRequest);
  if (TimerMsgPort) DeleteMsgPort(TimerMsgPort);
  if (SoundMsgPort) DeleteMsgPort(SoundMsgPort);

  if (GfxMemory) FreeVec(GfxMemory);
  if (BlorbMap) bb_destroy_map(BlorbMap);
  if (BlorbFile) fclose(BlorbFile);

  CloseBGUI(2);
  if (SaveIcon) FreeDiskObject(SaveIcon);
  if (ScreenFont) CloseFont(ScreenFont);
  if (OpenedTextFont) CloseFont(OpenedTextFont);
  if (OpenedFixedFont) CloseFont(OpenedFixedFont);
  if (Menus) FreeMenus(Menus);
  if (Visual) FreeVisualInfo(Visual);
  if (ThisProcess) ThisProcess->pr_WindowPtr = OldWindowPtr;
  if (Window) CloseWindow(Window);
  if (Screen) CloseScreen(Screen);
  FreePenTable();
  if (DefaultPubScreen) UnlockPubScreen(0,DefaultPubScreen);
  if (FileRequester) FreeAslRequest(FileRequester);
  if (AmigaGuideBase) CloseLibrary(AmigaGuideBase);
  if (KeymapBase) CloseLibrary(KeymapBase);

  TimerRequest = 0;
  TimerMsgPort = 0;
  SoundMsgPort = 0;

  GfxMemory = 0;
  BlorbMap = 0;
  BlorbFile = 0;

  SaveIcon = 0;
  OpenedTextFont = 0;
  OpenedFixedFont = 0;
  Menus = 0;
  Visual = 0;
  ThisProcess = 0;
  Window = 0;
  Screen = 0;
  DefaultPubScreen = 0;
  FileRequester = 0;
  AmigaGuideBase = 0;
  KeymapBase = 0;
}

void GetScreenRatio(struct Screen *screen)
{
struct TagItem vti[] =
  { VTAG_VIEWPORTEXTRA_GET,0,
    VTAG_END_CM,0 };
struct ViewPortExtra *vpe;

  ScaleX = 1000;
  ScaleY = 1000;
  ScreenWidth = screen->Width;
  ScreenHeight = screen->Height;
  if (screen->ViewPort.ColorMap)
  {
    if (VideoControl(screen->ViewPort.ColorMap,vti) == 0)
    {
      vpe = (struct ViewPortExtra *)vti[0].ti_Data;
      ScreenWidth = vpe->DisplayClip.MaxX - vpe->DisplayClip.MinX + 1;
      ScreenHeight = vpe->DisplayClip.MaxY - vpe->DisplayClip.MinY + 1;
      ScaleX = MAX((ScreenWidth*1000)/640,1000);
      ScaleY = MAX((ScreenHeight*1000)/256,1000);
    }
  }
}

void SetScreenDimensions(void)
{
  h_screen_width = Window->Width-Window->BorderLeft-Window->BorderRight;
  h_screen_height = Window->Height-Window->BorderTop-Window->BorderBottom;
  h_font_width = TextWidth;
  h_font_height = TextHeight;
  h_screen_cols = h_screen_width/h_font_width;
  h_screen_rows = h_screen_height/h_font_height;
}

void SetColourScheme(int custom)
{
  if (Screen)
  {
    if ((UsingColour > 1) || (DosColours == 1))
      LoadRGB4(&Screen->ViewPort,custom ? CustomColours : SystemColours,16);
  }
}

void Justifiable(void)
{
  if (h_version == V6) return;
  if ((MoreFrotzPrefs.Flags & PREFS_JUSTIFICATION) == 0) return;
  if ((TextFont->tf_Flags & FPF_PROPORTIONAL) == 0) return;
  if (cwin != 0) return;
  justify_pending = 1;
}

void LimitWindow(int limit,int x)
{
  if (limit)
  {
    WindowLimits(Window,
      RastPort->cp_x+RastPort->TxWidth+Window->BorderRight+x,
      Window->BorderTop+Window->BorderBottom+(TextHeight*3),
      ~0,~0);
  }
  else
  {
    WindowLimits(Window,
      Window->Width,Window->Height,
      Window->Width,Window->Height);
  }
}

void GetCursorPos(int *row, int *col)
{
  *row = RastPort->cp_y-Window->BorderTop+1;
  *col = RastPort->cp_x-Window->BorderLeft+1;
}

void SetFileExt(char *path,char *ext)
{
char *file,*extpos;

  file = FilePart(path);
  if ((extpos = strrchr(file,'.')) == 0) extpos = file+strlen(file);
  strcpy(extpos,ext);
}

int SafeTryOpen(char *path,char *ext,BPTR *handle)
{
  if (*handle != 0) return 0;

  SetFileExt(path,ext);
  *handle = Open(path,MODE_OLDFILE);
  return *handle;
}

void LoadGraphicsFile(void)
{
char gfx_path[MAX_FILE_NAME+1];
char *file_part;
BPTR fh = 0;
struct FileInfoBlock *fib;

  strcpy(gfx_path,story_name);
  if (SafeTryOpen(gfx_path,".mg1",&fh)) GfxType = GFX_IBM_MCGA;
  if (SafeTryOpen(gfx_path,".cg1",&fh)) GfxType = GFX_IBM_CGA;
  if (SafeTryOpen(gfx_path,".gfx",&fh)) GfxType = GFX_AMIGA;
  if (file_part = FilePart(gfx_path))
  {
    *file_part = '\0';
    strcat(gfx_path,"Pic");
    if (SafeTryOpen(gfx_path,".data",&fh)) GfxType = GFX_AMIGA;
  }
  if (fh != 0)
  {
    if (fib = AllocDosObjectTags(DOS_FIB,TAG_DONE))
    {
      ExamineFH(fh,fib);
      if (GfxMemory = AllocVec(fib->fib_Size,0))
      {
	Read(fh,GfxMemory,fib->fib_Size);
	GfxHeader = (struct GraphicsHeader *)GfxMemory;
      }
      FreeDosObject(DOS_FIB,fib);
    }
    Close(fh);
  }
  else if (BlorbMap == NULL)
  {
    strcpy(gfx_path,story_name);
    SetFileExt(gfx_path,".blb");
    if ((BlorbFile = fopen(gfx_path,"rb")) != NULL)
    {
      if (bb_create_map(BlorbFile,&BlorbMap) != bb_err_None)
	BlorbMap = NULL;
    }
  }
}

int ByteSwap(unsigned short n)
{
  if (GfxType != GFX_AMIGA) return ((n>>8)|((n&255)<<8));
  return n;
}

char *FindPicture(int pic)
{
char *info;
int i;

  info = GfxMemory+sizeof(struct GraphicsHeader);
  for (i = 0; i < ByteSwap(GfxHeader->Images); i++)
  {
    if (pic == ByteSwap(*(short *)(info+PIC_NUMBER))) return info;
    info += GfxHeader->InfoSize;
  }
  return 0;
}

ULONG GetColour32(ULONG col)
{
  col |= (col<<4);
  col |= (col<<8);
  col |= (col<<16);
  col |= (col<<32);
  return col;
}

void FreePenTable(void)
{
  if (DefaultPubScreen && UsePenTable)
  {
    if (IntuitionBase->lib_Version >= 39)
    {
    int i;

      for (i = 0; i < 16; i++)
      {
	ReleasePen(DefaultPubScreen->ViewPort.ColorMap,ZMachinePens[i]);
	ZMachinePens[i] = -1;
      }
    }
  }
  UsePenTable = 0;
}

struct TextFont *OpenCorrectFont(struct TextAttr *textAttr)
{
struct TextAttr topaz8 = {"topaz.font",8,FS_NORMAL,0};
struct TextFont *font = NULL;

  if (h_version != V6 || MoreFrotzPrefs.V6Style != V6_640200)
    font = OpenDiskFont(textAttr);
  if (font == NULL)
    font = OpenFont(&topaz8);
  return font;
}

int SizeGadgetHeight(struct Screen *screen)
{
struct DrawInfo *dri;
struct Image *size;
int height = 0;

  if (dri = GetScreenDrawInfo(screen))
  {
    if (size = NewObject(NULL,SYSICLASS,
      SYSIA_DrawInfo,dri,SYSIA_Which,SIZEIMAGE,TAG_DONE))
    {
      height = size->Height;
      DisposeObject(size);
    }
    FreeScreenDrawInfo(screen,dri);
  }
  return height;
}

void SetZColours(void)
{
  if ((Screen == NULL) && UsingColour)
  {
    int i;

    if (DosColours == 1)
    {
      for (i = 0; i < 16; i++)
      {
	SetRGB4(&(Window->WScreen->ViewPort),ZMachinePens[i],
	  (DosZColours[i]&0x0F00)>>8,
	  (DosZColours[i]&0x00F0)>>4,
	  (DosZColours[i]&0x000F));
      }
    }
    else
    {
      for (i = 0; i < 8; i++)
      {
	SetRGB4(&(Window->WScreen->ViewPort),ZMachinePens[i],
	  (ZColours[i]&0x0F00)>>8,
	  (ZColours[i]&0x00F0)>>4,
	  (ZColours[i]&0x000F));
      }
    }
  }
}

void SetDosColours(void)
{
  int fg = current_fg;
  int bg = current_bg;

  if (fg == 1)
    fg = h_default_foreground;
  if (bg == 1)
    bg = h_default_background;

  if (fg > WHITE_COLOUR)
    fg = WHITE_COLOUR;
  if (bg > WHITE_COLOUR)
    bg = WHITE_COLOUR;

  fg -= BLACK_COLOUR;
  bg -= BLACK_COLOUR;
  if (current_style & BOLDFACE_STYLE)
    fg += 8;

  if (UsePenTable)
  {
    SetAPen(RastPort,ZMachinePens[DosColoursIndex[fg]]);
    SetBPen(RastPort,ZMachinePens[DosColoursIndex[bg]]);
  }
  else
  {
    SetAPen(RastPort,DosColoursIndex[fg]);
    SetBPen(RastPort,DosColoursIndex[bg]);
  }
}

int snprintf(char *buffer, size_t count, const char *format, ...)
{
  va_list va;
  va_start(va,format);
  vsprintf(buffer,format,va);
  va_end(va);
}

#ifdef LOGGING
void Log(const char* msg, ...)
{
#ifdef LOGGING_STDIO
  va_list va;
  va_start(va,msg);
  vprintf(msg,va);
  putchar('\n');
  va_end(va);
#endif

#ifdef LOGGING_FILE
  FILE* log = fopen("Frotz.log","a+t");
  va_list va;
  va_start(va,msg);
  vfprintf(log,msg,va);
  fputc('\n',log);
  va_end(va);
  fclose(log);
#endif
}
#endif
