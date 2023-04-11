/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2023; last modified: April 2023 */

/* This module contains code for processing SVG images. */

#include "sdop.h"

#define NANOSVG_ALL_COLOR_KEYWORDS      // Include full list of color keywords.
#define NANOSVG_IMPLEMENTATION          // Expands implementation

#include "nanosvg.h"

#define MAX_FONTS 20

static char *boundfonts[MAX_FONTS];
static unsigned int setcolour;
static int nextfont;
static int setfont;
static float setfontsize;



/*************************************************
*          Load file into memory                 *
*************************************************/

/* Given an open file, load its contents into a block of mamory, terminated by
a binary zero.

Argument:  the open file
Returns:   pointer to malloc'd memory, or NULL on error
*/

static char *
loadfile(FILE *f)
{
char *string;
size_t size;

fseek(f, 0, SEEK_END);
size = ftell(f);
fseek(f, 0, SEEK_SET);
string = (char *)malloc(size + 1);

if (string != NULL)
  {
  if (fread(string, 1, size, f) != size)
    {
    free(string);
    string = NULL;
    }
  else string[size] = 0;
  }

return string;
}


/*************************************************
*        Get bounding box for an SVG image       *
*************************************************/

/* Load the image, then use the width/depth to generate a bounding box.

Arguments:
  f          open file for the image
  bb         where to return the bounding box

Returns:     TRUE on success
*/

BOOL
svg_find_size(FILE *f, double *bb)
{
NSVGimage *image;
char *string = loadfile(f);

if (string == NULL) return FALSE;
image = nsvgParse(string, "px", 96.0);

if (image == NULL) return FALSE;
bb[0] = 0.0;
bb[1] = 0.0;
bb[2] = image->width;
bb[3] = image->height;

nsvgDelete(image);
free(string);
return TRUE;
}


/*************************************************
*     Output colour change if necessary          *
*************************************************/

/* The colour value has red, green, blue in the lower three bytes, starting
from the least significant end. */

static void
check_colour(unsigned int c, FILE *outfile)
{
if (c == setcolour) return;
fprintf(outfile, "%g %g %g setrgbcolor\n", (double)((c >> 0) & 0xff)/255.0,
  (double)((c >> 8) & 0xff)/255.0,
  (double)((c >> 16) & 0xff)/255.0);
setcolour = c;
}


/*************************************************
*           Handle a <text> element              *
*************************************************/

/* This relies on addions to nanosvg that I made. Only fairly simple kinds of
text are supported. The height of the image is needed because SVG vertical
dimensions go downwards, whereas PostScript's go upwards.

Arguments:
  text       date from the <text> element
  H          the height of the SVG image
  outfile    the file to write to 

Returns:     nothing
*/

static void
write_text(NSVGtext *text, float H, FILE *outfile)
{
float size;
unsigned int weightstyle;
const char *font_family;
char font_key[256];

if (text->string == NULL) return;

size = text->font_size;
font_family = (text->font_family == NULL)? "Times" : text->font_family;

/* Combine weight and style settings into a single value. */

switch(text->weight + 8 * text->style)
  {
  default:
  case NSVG_WEIGHT_NORMAL + 8 * NSVG_TEXTSTYLE_NORMAL:
  weightstyle = 0;
  break;

  case NSVG_WEIGHT_NORMAL + 8 * NSVG_TEXTSTYLE_ITALIC:
  weightstyle = 1;
  break;

  case NSVG_WEIGHT_BOLD + 8 * NSVG_TEXTSTYLE_NORMAL:
  weightstyle = 2;
  break;

  case NSVG_WEIGHT_BOLD + 8 * NSVG_TEXTSTYLE_ITALIC:
  weightstyle = 3;
  break;
  }

/* The contents of font_family can be a comma-separated list of fonts or the
names "serif" or "sans-serif". This, with the addition of the weight and style,
is used as the key for remembering which fonts are already bound. When binding
a new font we need to generate a list of potential font names in the order they
are to be tried, ensuring that at least one is certain to exist. */

sprintf(font_key, "%s(%d)", font_family, weightstyle);

/* If the font we want is not the currently selected font, search the list in
case it has already been bound. If not, generate a list of potential fonts that
can be tried within the PostScript code. */

if (setfont < 0 || strcmp(font_family, boundfonts[setfont]) != 0)
  {
  setfontsize = 0.0;  /* Will always need to set size */

  for (setfont = 0; setfont < nextfont; setfont++)
    if (strcmp(font_key, boundfonts[setfont]) == 0) break;

  /*Need to bind a new font */

  if (setfont >= nextfont)
    {
    const char *failsafe = "/Times-Roman";
    const char *s = font_family;

    if (nextfont >= MAX_FONTS)
      {
      fprintf(stderr, "** Too many fonts in SVG image\n");
      exit(1);
      }

    fprintf(outfile, "[");

    for (;;)
      {
      const char *p, *base;
      int len;

      while (isspace(*s) || *s == ',') s++;
      if (*s == 0) break;
      p = s++;
      while (*s != 0 && *s != ',') s++;
      base = p;
      len = s - p;

      if (strncmp(p, "serif", len) == 0)
        {
        base = "Times";
        len = 5;
        }
      else if (strncmp(p, "sans-serif", len) == 0)
        {
        base = "Helvetica";
        len = 9;
        }

      switch (weightstyle)
        {
        case 0:
        fprintf(outfile, "/%.*s-Roman/%.*s-Regular", len, base, len, base);
        break;

        case 1:
        fprintf(outfile, "/%.*s-Italic/%.*s-Oblique", len, base, len, base);
        failsafe = "/Times-Italic";
        break;

        case 2:
        fprintf(outfile, "/%.*s-Bold", len, base);
        failsafe = "/Times-Bold";
        break;

        case 3:
        fprintf(outfile, "/%.*s-BoldItalic/%.*s-BoldOblique", len, base, len, base);
        failsafe = "/Times-BoldItalic";
        break;
        }

      fprintf(outfile, "/%.*s", len, base);
      if (strncmp(base, "Times", len) == 0 ||
          strncmp(base, "Helvetica", len) == 0)
        failsafe = "";  /* No need */
      }

    fprintf(outfile, "%s] multifindfont /sgfont%d exch def\n", failsafe, setfont);

    /* Remember what is bound */

    boundfonts[nextfont] = malloc(strlen(font_key) + 1);
    strcpy(boundfonts[nextfont++], font_key);
    }
  }

/* We should now be able to use the font whose number if in setfont. Scale to
required size and select if necessary. */

if (fabsf(size - setfontsize) > 0.1)
  {
  fprintf(outfile, "sgfont%d %g scalefont setfont\n", setfont, size);
  setfontsize = size;
  }

/* Now we can output the text. */

check_colour(text->fill, outfile);

fprintf(outfile, "%g %g moveto (", text->x, H - text->y);
for (char *s = text->string; *s != 0; s++)
  {
  if (*s < 32 || *s > 126) fprintf(outfile, "\\%03o", *s); else
    {
    if (*s == '(' || *s == ')' || *s == '\\') fprintf(outfile, "\\");
    fprintf(outfile, "%c", *s);
    }
  }
fprintf(outfile, ")");

switch (text->anchor)
  {
  case NSVG_ANCHOR_START: fprintf(outfile, " S\n"); break;
  case NSVG_ANCHOR_MIDDLE: fprintf(outfile, " SM\n"); break;
  case NSVG_ANCHOR_END: fprintf(outfile, " SE\n"); break;
  }
}


/*************************************************
*       Write and SVG image in PostScript        *
*************************************************/

/* We know the file will load because it will already have happened once in
order to get the depth.

Arguments:
  f            the open SVG file (will be closed externally)
  outfile      where to write

Returns:    nothing
*/

void
svg_write(FILE *f, FILE *outfile)
{
float H;
char *string = loadfile(f);
NSVGimage *image = nsvgParse(string, "px", 96.0);
NSVGtext  *text = image->texts;

setcolour = 0;
nextfont = 0;
setfont = -1;
setfontsize = -1.0;

/* SVG y-axis runs downwards. Moving the origin and inverting the y direction
works well for drawings, but it messes up text. Instead we just subtract all y
coordinates from the image height, saved in H. */

H = image->height;

/* Process all the shapes in the image */

for (NSVGshape *shape = image->shapes; shape != NULL; shape = shape->next)
  {
  for (NSVGpath *path = shape->paths; path != NULL; path = path->next)
    {
    float *p = path->pts;

    if (path->npts < 1) continue;
    fprintf(outfile, "%g %g Mt\n", p[0], H - p[1]);
    p += 2;

    for (int i = 2; i < 2*path->npts; i += 6, p+= 6)
      {
      fprintf(outfile, "%g %g %g %g %g %g Ct\n", p[0], H - p[1], p[2], H - p[3], p[4],
        H - p[5]);
      }

    switch (shape->fill.type)
      {
      case NSVG_PAINT_COLOR:
      check_colour(shape->fill.color, outfile);
      if (shape->stroke.type == NSVG_PAINT_COLOR)
        fprintf(outfile, "gsave fill grestore ");
      else fprintf(outfile, "fill");
      break;
      }

    switch(shape->stroke.type)
      {
      case NSVG_PAINT_COLOR:
      check_colour(shape->stroke.color, outfile);
      fprintf(outfile, "%g Slw St", shape->strokeWidth);
      }

    fprintf(outfile, "\n");
    }

  /* While the next text item is associate with this shape, output it. */

  while (text != NULL && text->aftershape == shape)
    {
    write_text(text, H, outfile);
    text = text->next;
    }
  }

/* Catch any remaining text items - they will all be here if there are no
shapes in the image. */

while (text != NULL)
  {
  write_text(text, H, outfile);
  text = text->next;
  }

fprintf(outfile, "showpage\n");

/* Delete image, free other memory */

nsvgDelete(image);
free(string);
for (int i = 0; i < nextfont; i++) free(boundfonts[i]);
}

/* End of svg.c */
