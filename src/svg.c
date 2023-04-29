/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2023; last modified: April 2023 */

/* This module contains code for processing SVG images. */

#include "sdop.h"

#define NANOSVG_ALL_COLOR_KEYWORDS      // Include full list of color keywords.
#include "nanosvg.h"

#define MAX_FONTS 20
#define MAX_GRADIENTS 20

static char *boundfonts[MAX_FONTS];
static int nextfont;
static int setfont;

static float *setdasharray;
static float setdashoffset;
static float setfontsize;
static float setmiterlimit;
static float setstrokewidth;

static int setdashcount;
static int setlinecap;
static int setlinejoin;

static unsigned int setcolour;
static NSVGgradient *boundgradients[MAX_GRADIENTS];
static int boundgradienttypes[MAX_GRADIENTS];
static int setgradient;
static int nextgradient;
static BOOL gradientset;


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
if (c == setcolour && !gradientset) return;
fprintf(outfile, "%g %g %g setrgbcolor\n", (double)((c >> 0) & 0xff)/255.0,
  (double)((c >> 8) & 0xff)/255.0,
  (double)((c >> 16) & 0xff)/255.0);
setcolour = c;
gradientset = FALSE;
}


/*************************************************
*     Output gradient change if necessary        *
*************************************************/

/* Unfortunately, shapes with identical gradients (by name) in SVG give rise to
different NSVGgradient blocks returned by nanosvg. The contents of the xform
field are different - I think because of a different translation. Therefore we
have to compare the fields that are relevant to us individually (excluding the
translation, for example).

Arguments:
  shape      the shape we are processing
  H          the image height
  outfile    where to write

Returns:     nothing
*/

static void
check_gradient(NSVGshape *shape, float H, FILE *outfile)
{
int ig;
int type = shape->fill.type;
float *bbox = shape->bounds;
NSVGgradient *g = shape->fill.gradient;
size_t len = g->nstops * sizeof(NSVGgradientStop);

for (ig = 0; ig < nextgradient; ig++)
  {
  NSVGgradient *gg = boundgradients[ig];
  if (type == boundgradienttypes[ig] &&
      g->nstops == gg->nstops &&
      memcmp(g, gg, len) == 0
      )
    break;
  }

if (ig == setgradient && gradientset) return;
setgradient = ig;
gradientset = TRUE;

if (setgradient >= nextgradient)  /* Need to set up a new gradient */
  {
  NSVGgradientStop *s = g->stops;
  float domain[2];

  domain[0] = s->offset;
  domain[1] = (s + g->nstops - 1)->offset;

  fprintf(outfile,
    "<< /PatternType 2\n"
    "   /Shading <<\n"
    "     /ColorSpace /DeviceRGB\n");

  if (g->spread == NSVG_SPREAD_PAD)
    fprintf(outfile,
      "     /Extend [true true]\n");

  if (type == NSVG_PAINT_LINEAR_GRADIENT)
    {
    fprintf(outfile,
      "     /ShadingType 2\n"
      "     /Coords [%g 0 %g 0]\n", bbox[0], bbox[2]);
    }
  else  /* Radial gradient */
    {
    fprintf(outfile,
      "     /ShadingType 3\n"
      "     /Coords [%g %g %g %g %g %g]\n",
        g->fx, H - g->fy, g->fr, g->cx, H- g->cy, g->r);
    }

  fprintf(outfile,
    "     /Domain [%g %g]\n", domain[0], domain[1]);

  /* If there are only two stops, only a single function is needed. Otherwise
  we must generate multiple functions and then "stitch" them into a combined
  function. */

  if (g->nstops == 2)
    {
    unsigned int c = s[0].color;

    fprintf(outfile,
      "     /Function <<\n"
      "       /FunctionType 2\n");
    fprintf(outfile,
      "       /Domain [%g %g]\n", domain[0], domain[1]);
    fprintf(outfile,
      "       /C0 [%g %g %g]\n", (double)((c >> 0) & 0xff)/255.0,
                                 (double)((c >> 8) & 0xff)/255.0,
                                 (double)((c >> 16) & 0xff)/255.0);
    c = (s + 1)->color;
    fprintf(outfile,
      "       /C1 [%g %g %g]\n", (double)((c >> 0) & 0xff)/255.0,
                                 (double)((c >> 8) & 0xff)/255.0,
                                 (double)((c >> 16) & 0xff)/255.0);
    fprintf(outfile,
      "       /N 1\n"
      "     >>\n");
    }

  /* Handle the case of more than 2 nstops */

  else
    {
    int k = g->nstops - 1;   /* Number of sections */

    fprintf(outfile,
      "     /Function <<\n"
      "       /FunctionType 3\n");
    fprintf(outfile,
      "       /Domain [%g %g]\n", domain[0], domain[1]);

    fprintf(outfile,
      "       /Bounds [%g", s[1].offset);
    for (int i = 2; i < k; i++) fprintf(outfile, " %g", s[i].offset);
    fprintf(outfile, "]\n");

    fprintf(outfile,
      "       /Encode [%g %g", s[0].offset, s[1].offset);
    for (int i = 1; i < k; i++)
      fprintf(outfile, " %g %g", s[i].offset, s[i+1].offset);
    fprintf(outfile, "]\n");

    fprintf(outfile,
      "       /Functions [\n");

    /* Create a vector of type 2 functions, one for each interval */

    for (int i = 0; i < k; i++)
      {
      unsigned int c = s[i].color;
      fprintf(outfile,
        "         <<\n"
        "         /FunctionType 2\n"
        "         /Domain [0 1]\n");
      fprintf(outfile,
        "         /C0 [%g %g %g]\n", (double)((c >> 0) & 0xff)/255.0,
                                     (double)((c >> 8) & 0xff)/255.0,
                                     (double)((c >> 16) & 0xff)/255.0);
      c = s[i+1].color;
      fprintf(outfile,
        "         /C1 [%g %g %g]\n", (double)((c >> 0) & 0xff)/255.0,
                                     (double)((c >> 8) & 0xff)/255.0,
                                     (double)((c >> 16) & 0xff)/255.0);
      fprintf(outfile,
        "         /N 1\n"
        "         >>\n");
      }

    /* End functions vector and the type 3 function dictionary. */

    fprintf(outfile,
      "       ]\n"
      "     >>\n");
    }

  /* Terminate the shading and pattern dictionaries. */

  fprintf(outfile,
    "   >>\n"
    ">>\n");

  /* Now create the pattern and name it. */

  fprintf(outfile, "matrix ");

//  fprintf(outfile, "[%g %g %g %g %g %g] ",
//    g->xform[0], g->xform[1], g->xform[2],
//    g->xform[3], g->xform[4], g->xform[5]);

  fprintf(outfile, "makepattern /svggrad%d exch def\n", setgradient);

  /* Remember what is set up */

  boundgradienttypes[nextgradient] = type;
  boundgradients[nextgradient++] = g;
  }

fprintf(outfile, "svggrad%d setpattern\n", setgradient);
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
nextgradient = 0;
setgradient = -1;
gradientset = FALSE;
nextfont = 0;
setfont = -1;
setfontsize = -1.0;
setmiterlimit = -1.0;
setstrokewidth = -1.0;
setlinecap = -1;
setlinejoin = -1;

setdashcount = -1;
setdashoffset = 0.0;
setdasharray = NULL;

/* SVG y-axis runs downwards. Moving the origin and inverting the y direction
works well for drawings, but it messes up text. Instead we just subtract all y
coordinates from the image height, saved in H. */

H = image->height;

/* Process all the shapes in the image. For each shape we output all the paths,
which may be disjoint, forming one PostScript path. */

for (NSVGshape *shape = image->shapes; shape != NULL; shape = shape->next)
  {
  BOOL fillit = shape->fill.type != NSVG_PAINT_UNDEF &&
                shape->fill.type != NSVG_PAINT_NONE;
  BOOL strokeit = shape->stroke.type != NSVG_PAINT_UNDEF &&
                  shape->stroke.type != NSVG_PAINT_NONE;

  if ((shape->flags & NSVG_FLAGS_VISIBLE) == 0) fillit = strokeit = FALSE;

  /* Output the path if it is to be filled or stroked. */

  if (fillit || strokeit)
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
      }
    }

  /* Now handle filling and/or stroking of the path. */

  if (fillit)
    {
    const char *eo = (shape->fillRule == NSVG_FILLRULE_NONZERO)? "" : "eo";

    switch (shape->fill.type)
      {
      case NSVG_PAINT_COLOR:
      check_colour(shape->fill.color, outfile);
      break;

      case NSVG_PAINT_LINEAR_GRADIENT:
      case NSVG_PAINT_RADIAL_GRADIENT:
      check_gradient(shape, H, outfile);
      break;
      }

    /* If we are also going to stroke this path, use gsave to ensure it gets
    re-instated after filling. */

    if (strokeit) fprintf(outfile, "gsave %sfill grestore ", eo);
      else fprintf(outfile, "%sfill\n", eo);
    }

  if (strokeit)
    {
    switch(shape->stroke.type)
      {
      BOOL needdash;

      case NSVG_PAINT_COLOR:
      check_colour(shape->stroke.color, outfile);

      if (fabs(shape->strokeWidth - setstrokewidth) > 0.001)
        {
        fprintf(outfile, "%g Slw ", shape->strokeWidth);
        setstrokewidth = shape->strokeWidth;
        }

      if (setlinecap != shape->strokeLineCap)
        {
        fprintf(outfile, "%d Slc ", shape->strokeLineCap);
        setlinecap = shape->strokeLineCap;
        }

      if (setlinejoin != shape->strokeLineJoin)
        {
        fprintf(outfile, "%d Slj ", shape->strokeLineJoin);
        setlinejoin = shape->strokeLineJoin;
        }

      if (fabs(shape->miterLimit - setmiterlimit) > 0.001)
        {
        fprintf(outfile, "%g Slm ", shape->miterLimit);
        setmiterlimit = shape->miterLimit;
        }

      needdash = setdashcount != shape->strokeDashCount ||
                 setdashoffset != shape->strokeDashOffset;

      if (!needdash)
        {
        for (int i = 0; i < setdashcount; i++)
          {
          if (setdasharray[0] != shape->strokeDashArray[0])
          needdash = TRUE;
          break;
          }
        }

      if (needdash)
        {
        fprintf(outfile, "[");
        for (int i = 0; i < shape->strokeDashCount; i++)
          {
          if (i > 0) fprintf(outfile, " ");
          fprintf(outfile, "%g", shape->strokeDashArray[i]);
          }
        fprintf(outfile, "] %g Sd ", shape->strokeDashOffset);
        setdashcount = shape->strokeDashCount;
        setdashoffset = shape->strokeDashOffset;
        setdasharray = shape->strokeDashArray;
        }

      fprintf(outfile, "St\n");
      break;
      }
    }

  /* While the next text item is associate with this shape, output it. */

  while (text != NULL && text->aftershape == shape)
    {
    if ((text->flags & NSVG_FLAGS_VISIBLE) != 0) write_text(text, H, outfile);
    text = text->next;
    }

  /* If enabled, output a list of ignored attributes. */

  if (svg_listignored)
    {
    if (shape->opacity != 1.0)
      fprintf(stderr, "SVG ignored opacity=%g\n",
        shape->opacity);
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
