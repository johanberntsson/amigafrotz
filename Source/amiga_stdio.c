/*
 * amiga_stdio.c
 *
 * Standard IO interface for the Amiga and DICE C
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

#include <proto/exec.h>
#include <dos/dosextens.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "frotz.h"
#include "blorb.h"
#include "blorblow.h"

#define INFORMATION "\
\n\
FROTZ V2.41 - interpreter for all Infocom games. Complies with standard\n\
1.0 of Graham Nelson's specification. Written by Stefan Jokisch in 1995-7.\n\
Amiga version by David Kinder.\n\
\n\
Syntax: frotz [options] story-file\n\
\n\
  -a   watch attribute setting     -p   alter piracy opcode\n\
  -A   watch attribute testing     -r # right margin\n\
  -c # context lines               -R   save/restore in old Frotz format\n\
  -f # foreground colour           -s # random number seed value\n\
  -h # screen height               -S # transcript width\n\
  -i   ignore runtime errors       -t   set Tandy bit\n\
  -l # left margin                 -u # slots for multiple undo\n\
  -o   watch object movement       -w # screen width\n\
  -O   watch object locating       -x   expand abbreviations g/x/z\n\
                                   -Z # error checking (see below)\n\
\n\
Error checking is 0 (none), 1 (report first error (default)),\n\
  2 (report all errors), 3 (exit after any error)."

#define OS_PATHSEP ';'

static int current_style = 0;
static int saved_style = 0;
static int user_tandy_bit = -1;
static int user_random_seed = -1;

int getopt (int, char *[], const char *);

extern const char *optarg;
extern int optind;

char Version[] = "$VER:FrotzStdIO 2.41 (25.8.2007)";

struct Process *ThisProcess;
struct Window *OldWindowPtr;

static int stdio_width = 77;
static int stdio_height = 21;
static int stdio_colour = 1;
static int stdio_x,stdio_y;
static int stdio_break = 1;
static int stdio_time = 0;
static char stdio_save[MAX_FILE_NAME+1];
static time_t start_time;
static int warning = 0;

bb_map_t* BlorbMap = NULL;

void get_cursor (int *row, int *col);
int brk(void);
void reset_win_ptr(void);

void os_beep (int number)
{
}

void os_display_char (zchar c)
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
    putchar(c);
    stdio_x++;
  }
}

void os_display_string (const zchar *s)
{
int c;

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

void os_erase_area (int top, int left, int bottom, int right)
{
int row, col, i;

  fflush(stdout);
  get_cursor(&row,&col);
  for(i = bottom;i >= top;i--)
  {
    os_set_cursor(i,0);
    printf("\033[K");
  }
  os_set_cursor(row,col);
}

void os_fatal (const char *s)
{
  fflush(stdout);
  fputs("\nFatal error: ",stderr);
  fputs(s,stderr);
  fputs("\n",stderr);

  exit(1);
}

void os_finish_with_sample (void)
{
}

int os_font_data (int font, int *height, int *width)
{
  *height = h_font_height;
  *width = h_font_width;

  if (font == TEXT_FONT) return 1;
  if (font == GRAPHICS_FONT) return 1;
  if (font == FIXED_WIDTH_FONT)	return 1;
  return 0;
}

int os_read_file_name (char *file_name, const char *default_name, int flag)
{
FILE *fp;
int c;
int first = 1;
char *number;
int saved_replay = istream_replay;
int saved_record = ostream_record;
int result = 0;

  fflush(stdout);

  istream_replay = 0;
  ostream_record = 0;

  if (strcmp(stdio_save,"") != 0)
  {
    if ((flag == FILE_SAVE) || (flag == FILE_RESTORE))
    {
      strcpy(file_name,stdio_save);
      if ((number = strchr(file_name,'?')) == 0)
      {
	result = 1;
	goto finished;
      }

      print_string("Current saved games:");
      for ( c = '0'; c <= '9'; c++ )
      {
	*number = c;
	if (access(file_name,0) == 0)
	{
	  first ? first = 0 : print_char(',');
	  print_char(c);
	}
      }
      print_string("\nEnter game number (0-9):");
      flush_buffer();
      do c = os_read_key(0,1); while (c < '0' || c > '9');
      print_string("\n");
      *number = c;
      result = 1;
    }
    goto finished;
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

void os_init_screen (void)
{
  atexit(reset_win_ptr);
  ThisProcess = (struct Process *)FindTask(0);
  OldWindowPtr = ThisProcess->pr_WindowPtr;
  ThisProcess->pr_WindowPtr = (void *)~0;

  onbreak(brk);
  os_set_cursor(1,1);
  if (stdio_colour >= 8)
    printf("\033[J\033[0;1;3%dm",stdio_colour-8);
  else
    printf("\033[J\033[0;3%dm",stdio_colour);
  time(&start_time);

  if (h_version == V3)
  {
    h_config |= CONFIG_SPLITSCREEN;
    if (user_tandy_bit != -1) h_config |= CONFIG_TANDY;
    if (h_flags & OLD_SOUND_FLAG) h_flags &= ~OLD_SOUND_FLAG;
  }
  if (h_version >= V4)
  {
    h_config |= CONFIG_BOLDFACE;
    h_config |= CONFIG_EMPHASIS;
    h_config |= CONFIG_FIXED;
    if (h_flags & SOUND_FLAG) h_flags &= ~SOUND_FLAG;
  }
  if (h_version >= V5)
  {
    if (h_flags & UNDO_FLAG)
    {
      if (option_undo_slots == 0) h_flags &= ~UNDO_FLAG;
    }
    if (h_flags & COLOUR_FLAG) h_flags &= ~COLOUR_FLAG;
  }
  if (h_version == V6)
  {
    h_flags &= ~MENU_FLAG;
  }

  h_default_foreground = BLACK_COLOUR;
  h_default_background = WHITE_COLOUR;
  h_interpreter_number = INTERP_AMIGA;
  h_interpreter_version = h_version != V6 ? 'C' : 3;
  h_screen_cols = stdio_width;
  h_screen_rows = stdio_height;
  h_font_width = 1;
  h_font_height = 1;
  h_screen_width = h_screen_cols;
  h_screen_height = h_screen_rows;
}

void os_more_prompt (void)
{
int saved_style;
int saved_x,saved_y;
int new_x,new_y;

  fflush(stdout);

  saved_style = current_style;
  os_set_text_style(0);

  get_cursor(&saved_y,&saved_x);
  os_display_string("[MORE]");
  os_read_key(0,1);

  get_cursor(&new_y,&new_x);
  os_erase_area(new_y,saved_x,new_y+h_font_height,new_x);
  os_set_cursor(saved_y,saved_x);
  os_set_text_style(saved_style);
}

void os_prepare_sample (int number)
{
}

void os_process_arguments (int argc, char *argv[])
{
int num;
int c;

  do
  {
    optarg = 0;
    c = getopt(argc,argv,"aAc:Cf:h:il:oOpr:Rs:S:tT:u:V:w:xZ:");

    if (optarg != 0) num = atoi(optarg);

    switch (c)
    {
      case 'a': option_attribute_assignment = 1;
		break;
      case 'A': option_attribute_testing = 1;
		break;
      case 'c': option_context_lines = num;
		break;
      case 'C': stdio_break = 0;
		break;
      case 'f': stdio_colour = num;
		break;
      case 'h': stdio_height = num;
		break;
      case 'i': option_ignore_errors = 1;
		break;
      case 'l': option_left_margin = num;
		break;
      case 'o': option_object_movement = 1;
		break;
      case 'O': option_object_locating = 1;
		break;
      case 'p': option_piracy = 1;
		break;
      case 'r': option_right_margin = num;
		break;
      case 'R': option_save_quetzal = 0;
		break;
      case 's': user_random_seed = num;
		break;
      case 'S': option_script_cols = num;
		break;
      case 't': user_tandy_bit = 1;
		break;
      case 'T': stdio_time = num;
		break;
      case 'u': option_undo_slots = num;
		break;
      case 'V': strcpy(stdio_save,optarg);
		break;
      case 'w': stdio_width = num;
		break;
      case 'x': option_expand_abbreviations = 1;
		break;
      case 'Z': if (num >= ERR_REPORT_NEVER && num <= ERR_REPORT_FATAL)
		  err_report_mode = num;
		break;
    }
  }
  while (c != EOF);

  if (optind != argc-1)
  {
    puts(INFORMATION);
    exit(1);
  }

  story_name = argv[optind];
}

zchar os_read_line (int max, zchar *buf, int timeout, int width, int continued)
{
int row, col;
zchar *trail;

  fflush(stdout);
  get_cursor(&row,&col);
  fgets(buf,max,stdin);
  if (trail = strrchr(buf,'\n')) *trail = '\0';
  os_set_cursor(row,col+strlen(buf));

time_t current_time;

  if (stdio_time > 0)
  {
    time(&current_time);
    if ((current_time-start_time)/60 >= stdio_time)
    {
      screen_new_line();
      os_display_string("You are out of time for this session.");
      screen_new_line();
      os_display_string("Exiting...");
      screen_new_line();
      exit(0);
    }
    if (((current_time-start_time)/60 >= stdio_time-5) && (warning == 0))
    {
      screen_new_line();
      os_display_string("You have 5 minutes left.n");
      screen_new_line();
      warning = 1;
    }
    if (((current_time-start_time)/60 >= stdio_time-1) && (warning == 1))
    {
      screen_new_line();
      os_display_string("You have 1 minute left.");
      screen_new_line();
      warning = 2;
    }
  }

  return ZC_RETURN;
}

zchar os_read_key (int timeout, bool cursor)
{
int row, col, c;

  fflush(stdout);
  get_cursor(&row,&col);
  c = getchar();
  os_set_cursor(row,col);
  return c;
}

void os_reset_screen (void)
{
  if (BlorbMap) bb_destroy_map(BlorbMap);
  BlorbMap = 0;

  fflush(stdout);
  printf("\033[0m");
}

void os_scroll_area (int top, int left, int bottom, int right, int units)
{
  fflush(stdout);
  while (units > 0)
  {
    os_set_cursor(top,0);
    printf("\033[M");
    units--;
  }
  os_set_cursor(h_screen_rows,0);
}

void os_set_colour (int new_foreground, int new_background)
{
}

void os_set_cursor (int row, int col)
{
  fflush(stdout);
  printf("\033[%d;%dH",row,col);
  stdio_x = col;
  stdio_y = row;
}

void os_set_font (int new_font)
{
}

void os_set_text_style (int new_style)
{
  fflush(stdout);
  current_style = new_style;

  if (stdio_colour >= 8)
    printf("\033[0;1;3%dm",stdio_colour-8);
  else
    printf("\033[0;3%dm",stdio_colour);

  if (new_style & REVERSE_STYLE)
  {
    printf("\033[7m");
  }

  if (new_style & BOLDFACE_STYLE)
  {
    printf("\033[1m");
  }

  if (new_style & EMPHASIS_STYLE)
  {
    printf("\033[4m");
  }
}

void os_start_sample (int number, int volume, int repeats)
{
}

void os_stop_sample (void)
{
}

int os_string_width (const zchar *s)
{
int length,i,c;

  length = 0;

  for (i = 0; s[i] != 0; i++)
  {
    c = s[i];
    if ((c == ZC_NEW_STYLE) || (c == ZC_NEW_FONT)) i++;
    if (c == ZC_INDENT) length += 3;
    if (c == ZC_GAP) length += 2;
    if ((c >= ZC_ASCII_MIN && c <= ZC_ASCII_MAX) || (c >= ZC_LATIN1_MIN && c <= ZC_LATIN1_MAX))
      length++;
  }
  return length;
}

int os_char_width (zchar c)
{
zchar s[2];

  s[0] = c;
  s[1] = 0;
  return os_string_width(s);
}

int os_peek_colour (void)
{
  return WHITE_COLOUR;
}

int os_picture_data (int picture, int *height, int *width)
{
  *height = 0;
  *width = 0;
  return 0;
}

void os_draw_picture (int picture, int y, int x)
{
}

int os_random_seed (void)
{
  if (user_random_seed == -1) return clock()&32767;
  return user_random_seed;
}

void os_restart_game (int stage)
{
}

FILE *os_path_open (const char *name, const char *mode, long *size)
{
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

void get_cursor (int *row, int *col)
{
  fflush(stdout);
  *row = stdio_y;
  *col = stdio_x;
}

int brk(void)
{
  if (stdio_break)
  {
    fflush(stdout);
    printf("\033[0m\n***Break\n");
    exit(0);
  }
  return 0;
}

int snprintf(char *buffer, size_t count, const char *format, ...)
{
  va_list va;
  va_start(va,format);
  vsprintf(buffer,format,va);
  va_end(va);
}

void reset_win_ptr(void)
{
  if (ThisProcess) ThisProcess->pr_WindowPtr = OldWindowPtr;
}

void Justifiable(void)
{
}
