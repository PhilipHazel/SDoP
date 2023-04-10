/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: April 2023 */

/* This module contains code for handling graphics objects. */

#include "sdop.h"

#if SUPPORT_JPEG || SUPPORT_PNG
static int image_width;
static int image_depth;
#endif

#if SUPPORT_JPEG
static int jpeg_ncomp;
#endif

#if SUPPORT_PNG
static int png_rowbytes;
#endif


/*************************************************
*       Callback from PNG to pass info           *
*************************************************/

/* This function is called from read_PNG_file() to pass back the size of the
image.

Arguments:
  width         in pixels
  depth         in pixels
  rowbytes      bytes per row

Returns:        nothing
*/

#if SUPPORT_PNG
void
give_png_image_data(int width, int depth, int rowbytes)
{
image_width = width;
image_depth = depth;
png_rowbytes = rowbytes;
DEBUG(D_object)
  debug_printf("PNG pixel width=%d depth=%d rowbytes=%d\n", width, depth,
    rowbytes);
}
#endif


/*************************************************
*      Callback from JPEG to pass info           *
*************************************************/

/* This function is called from read_JPEG_file() to pass back the size of the
image.

Arguments:
  width         in pixels
  depth         in pixels
  ncomp         number of components

Returns:        nothing
*/

#if SUPPORT_JPEG
void
give_jpeg_image_data(int width, int depth, int ncomp)
{
image_width = width;
image_depth = depth;
jpeg_ncomp = ncomp;
DEBUG(D_object)
  debug_printf("JPEG pixel width=%d depth=%d ncomp=%d\n", width, depth, ncomp);
}
#endif



/*************************************************
*    Callback from JPEG to output one scan line  *
*************************************************/

/* This function is called from read_JPEG_file() to pass back the scan lines
that comprise the image. We write out the hext data, keeping the lines to a
reasonable length.

Arguments:
  scanline       points to the line
  row_stride     the stride (number of elements in the line)
  token          token that was passed to read_JPEF_file()

Returns:         nothing
*/

#if SUPPORT_JPEG
void
put_scanline_someplace(uschar *scanline, int row_stride, void *token)
{
FILE *outfile = (FILE *)token;
for (int i = 0; i < row_stride; i++)
  {
  if ((i & 31) == 0) (void)fprintf(outfile, "\n");
  (void)fprintf(outfile, "%02x", ((unsigned char *)scanline)[i]);
  }
(void)fprintf(outfile, "\n");
}
#endif



/*************************************************
*           Find depth of a textobject           *
*************************************************/

/*
Argument:  the <textobject> item
Returns:   the depth
*/

static int
textobject_depth(item *i)
{
int depth = 0;
for (item *j = i->next; j != i->partner; j = j->next)
  {
  outputline *ol;
  if (Ustrcmp(j->name, "#PCPARA") != 0) continue;
  depth += j->p.prgrph->layparm->beforemax;
  for (ol = j->p.prgrph->out; ol != NULL; ol = ol->next) depth += ol->depth;
  }
DEBUG(D_object) debug_printf("textobject depth=%d\n", depth);
return depth;
}



/*************************************************
*        Open the file for an image object       *
*************************************************/

/* This is used both when finding the depth of an object, and when eventually
outputting it.

Arguments:
  i          the <imageobject> item
  msg        where to put a problem message
  dptr       where to return the <imagedata> item
  fnptr      where to return the filename pointer
  piform     where to return the image format type

Returns:     an open file or NULL
*/

static FILE *
object_file(item *i, uschar **msg, item **dptr, uschar **fnptr, int *piform)
{
item *j;
paramstr *pfile, *pform;
FILE *f;
uschar *s;
uschar formname[24];
uschar buffer[1024];

for (j = i->next; j != i->partner; j = j->next)
  if (Ustrcmp(j->name, "imagedata") == 0) break;

if ((*dptr = j) == i->partner)
  {
  *msg = US": no <imagedata> found";
  return NULL;
  }

pfile = misc_param_find(j, US"fileref");
if (pfile == NULL)
  {
  *msg = US": fileref missing";
  return NULL;
  }

if ((pfile->value)[0] == '/')
  Ustrcpy(buffer, pfile->value);
else
  {
  uschar *slash = Ustrrchr(read_filename, '/');
  if (slash == NULL)
    (void)sprintf(CS buffer, "./%s", pfile->value);
  else
    {
    int blen = (int)(slash - read_filename);
    memcpy(buffer, read_filename, blen);
    (void)sprintf(CS buffer + blen, "/%s", pfile->value);
    }
  }

pform = misc_param_find(j, US"format");
if (pform == NULL)
  {
  uschar *dot = Ustrrchr(buffer, '.');
  if (dot == NULL)
    {
    *msg = US": format missing and file has no extension";
    return NULL;
    }
  (void)sprintf(CS formname, "%.*s", (int)(sizeof(formname) - 1), dot + 1);
  }
else
  (void)sprintf(CS formname, "%.*s", (int)(sizeof(formname) - 1), pform->value);

for (s = formname; *s != 0; s++) *s = toupper(*s);

if (Ustrcmp(formname, "EPS") == 0 ||
    Ustrcmp(formname, "PS") == 0)
  {
  *piform = IFORM_EPS;
  }
else if (Ustrcmp(formname, "SVG") == 0)
  {
  *piform = IFORM_SVG;
  }     
else if (Ustrcmp(formname, "JPEG") == 0 ||
         Ustrcmp(formname, "JPG") == 0)
  {
  #if SUPPORT_JPEG
  *piform = IFORM_JPG;
  #else
  *msg = US": SDoP was not compiled with JPEG support";
  return NULL;
  #endif
  }
else if (Ustrcmp(formname, "PNG") == 0)
  {
  #if SUPPORT_PNG
  *piform = IFORM_PNG;
  #else
  *msg = US": SDoP was not compiled with PNG support";
  return NULL;
  #endif
  }
else
  {
  *msg = US": file format unrecognized";
  return NULL;
  }

*fnptr = misc_malloc(Ustrlen(buffer) + 1);
Ustrcpy(*fnptr, buffer);

f = Ufopen(buffer, "rb");
if (f == NULL)
  {
  error(74, buffer, "image object", strerror(errno));
  *msg = US": failed to open file";
  return NULL;
  }

return f;
}



/*************************************************
*     Find depth and scale of an imageobject     *
*************************************************/

/* EPS, SVG, JPEG, and PNG files are supported. If no "depth" parameter is
given, get the depth from the bounding box. If no bounding box is found, but
both "width" and "depth" are available, make up a bounding box.

Argument:
  f        the open object file
  iform    the format of the file
  idata    the <imagedata> item
  scptr    where to return the scale (fixed point)
  bb       where to return the bounding box (double)
  writing  TRUE when actually writing; FALSE when just measuring

Returns:   the depth or -1
*/

static int
imageobject_depth(FILE *f, int iform, item *idata, int *scptr, double *bb,
  BOOL writing)
{
int depth = -1;
int width = -1;
int scale = -1;
BOOL bbset = FALSE;
paramstr *p;
uschar buffer[1024];

#if SUPPORT_PNG
uschar *msg;
#endif

p = misc_param_find(idata, US"depth");
if (p != NULL)
  {
  uschar *t;
  depth = misc_get_fp(p->value, &t);
  depth = misc_scale_number(depth, t);
  }

p = misc_param_find(idata, US"width");
if (p != NULL)
  {
  uschar *t;
  width = misc_get_fp(p->value, &t);
  width = misc_scale_number(width, t);
  }

switch(iform)
  {
  case IFORM_EPS:
  while (Ufgets(buffer, sizeof(buffer), f) != NULL)
    {
    if (sscanf(CS buffer, "%%%%BoundingBox: %lf %lf %lf %lf", &bb[0], &bb[1],
          &bb[2], &bb[3]) == 4)
      {
      bbset = TRUE;
      break;
      }
    }
  break;
  
  case IFORM_SVG:
  if (!svg_find_size(f, bb)) error(109, "SVG", "failed to find size"); /* Hard */
  bbset = TRUE; 
  break;  

  #if SUPPORT_JPEG
  case IFORM_JPG:
  read_JPEG_file(f, TRUE, NULL);
  bb[0] = 0.0;
  bb[1] = 0.0;
  bb[2] = (double)image_width;
  bb[3] = (double)image_depth;
  bbset = TRUE;
  break;
  #endif

  #if SUPPORT_PNG
  case IFORM_PNG:
  if (!read_PNG_file(f, &msg, writing)) error(109, "PNG", msg);  /* Hard */
  bb[0] = 0.0;
  bb[1] = 0.0;
  bb[2] = (double)image_width;
  bb[3] = (double)image_depth;
  bbset = TRUE;
  break;
  #endif

  default:
  error(80, "imageobject_depth()");   /* Hard error */
  break;
  }

/* Find the scale */

p = misc_param_find(idata, US"scale");
if (p != NULL)
  {
  scale = misc_get_number(p->value) * 10;
  if (scale < 0) error(75, p->value, "image scale factor");
  }

p = misc_param_find(idata, US"scalefit");
if (p != NULL && Ustrcmp(p->value, "1") == 0)
  {
  if (scale > 0) error(77);
  if (!bbset) error(76);
    else scale = (width > 0)? (int)((double)(width)/(bb[2]-bb[0])) :
                 (depth > 0)? (int)((double)(depth)/(bb[3]-bb[1])) :
                              (int)((double)(page_linewidth)/(bb[2]-bb[0]));
  }

if (scale < 0) scale = 1000;

if (bbset)
  {
  for (int i = 0; i <4; i++) bb[i] = MULDIV(bb[i], scale, 1000);
  if (depth < 0) depth = (int)((bb[3] - bb[1]) * 1000);
  }
else
  {
  if (depth >= 0 && width >= 0)
    {
    bb[0] = bb[1] = 0.0;
    bb[2] = (double)(width/1000);
    bb[3] = (double)(depth/1000);
    }
  else bb[0] = bb[1] = bb[2] = bb[3] = 0.0;
  }

*scptr = scale;
return depth;
}



/*************************************************
*   Find origin adjustment for an image object   *
*************************************************/

/* The "role" parameter of <imageobject> is abused in order to provide a way to
move an image about without breaking the DocBook DTD. If this parameter
contains one or two dimensions, comma separated, they are taken as x and y
adjustments, respectively.

Arguments:
  i            the <imageobject>
  px           pointer to the x coordinate
  py           pointer to the y coordinate

Returns:       nothing
*/

static void
adjust_image_position(item *i, int *px, int *py)
{
int deltas[2];
int n = 2;
paramstr *p = misc_param_find(i, US"role");

if (p == NULL || !isdigit(p->value[0])) return;

if (Ustrchr(p->value, ',') == NULL)
  {
  n = 1;
  deltas[1] = 0;
  }

if (misc_get_dimensions(n, p->value, deltas, FALSE))
  {
  *px += deltas[0];
  *py += deltas[1];
  }
}



/*************************************************
*         Output an image object                 *
*************************************************/

/* This function is called from write.c to output an image object.

Arguments:
  i           the <imageobject>
  ypos        the y-position on the page
  outfile     the output file
  pwidth      where to return the width
  pjustify    where to return the justification

Returns:      the depth of the image, or -1 if there's a problem
*/

int
object_write_image(item *i, int ypos, FILE *outfile, int *pwidth, int *pjustify)
{
FILE *f;
item *idata;
int depth, bbdepth, scale, x, y, iform;
uschar *msg;
uschar *filename;
paramstr *p;
double bb[4];
uschar buffer[1024];

f = object_file(i, &msg, &idata, &filename, &iform);
if (f == NULL) return -1;

/* The values that are returned are all scaled, but we also need the scale
value to embed in the PostScript. */

depth = imageobject_depth(f, iform, idata, &scale, bb, TRUE);

bbdepth = (int)((bb[3] - bb[1]) * 1000);

x = margin_left - (int)(bb[0] * 1000);
y = ypos - depth - (int)(bb[1] * 1000);

if (depth > bbdepth) y += (depth - bbdepth)/2;

*pwidth = (int)((bb[2] - bb[0]) * 1000);
*pjustify = J_LEFT;

p = misc_param_find(idata, US"align");
if (p != NULL)
  {
  if (Ustrcmp(p->value, "centre") == 0 || Ustrcmp(p->value, "center") == 0)
    {
    x += (page_linewidth - *pwidth)/2;
    *pjustify = J_CENTRE;
    }
  else if (Ustrcmp(p->value, "right") == 0)
    {
    x += (page_linewidth - *pwidth);
    *pjustify = J_RIGHT;
    }
  }

/* The "role" parameter of <imageobject> is abused in order to provide a way of
moving an image about, to cope with poor bounding boxes or other problems. */

adjust_image_position(i, &x, &y);

/* Now we can generate the output */

(void)fprintf(outfile, "\n\n");

switch (iform)
  {
  case IFORM_EPS:
  (void)fprintf(outfile,
    "/picsave save def/a4{null pop}def\n"
    "/showpage{initgraphics}def/copypage{null pop}def\n");
  (void)fprintf(outfile, "%s ", misc_formatfixed(x));
  (void)fprintf(outfile, "%s translate\n", misc_formatfixed(y));
  (void)fprintf(outfile, "%s dup scale\n", misc_formatfixed(scale));
  rewind(f);
  while (Ufgets(buffer, sizeof(buffer), f) != NULL)
    { if (buffer[0] != '%') (void)fprintf(outfile, "%s", buffer); }
  (void)fprintf(outfile, "picsave restore\n");
  break;

  case IFORM_SVG:
  (void)fprintf(outfile,
    "/picsave save def/a4{null pop}def\n"
    "/showpage{initgraphics}def/copypage{null pop}def\n");
  (void)fprintf(outfile, "%s ", misc_formatfixed(x));
  (void)fprintf(outfile, "%s translate\n", misc_formatfixed(y));
  (void)fprintf(outfile, "%s dup scale\n", misc_formatfixed(scale));
  rewind(f);
  svg_write(f, outfile); 
  (void)fprintf(outfile, "picsave restore\n");
  break;

  #if SUPPORT_JPEG
  case IFORM_JPG:
  (void)fprintf(outfile, "gsave\n");
  (void)fprintf(outfile, "/picstr %d string def\n", image_width * jpeg_ncomp);
  (void)fprintf(outfile, "%s ", misc_formatfixed(x));
  (void)fprintf(outfile, "%s translate\n", misc_formatfixed(y));
  (void)fprintf(outfile, "%s dup scale\n", misc_formatfixed(scale));
  (void)fprintf(outfile, "%d %d scale\n", image_width, image_depth);
  (void)fprintf(outfile, "%d %d %d [%d 0 0 -%d 0 %d]\n", image_width,
    image_depth, 8, image_width, image_depth, image_depth);
  (void)fprintf(outfile, "{currentfile picstr readhexstring pop}\n");
  (void)fprintf(outfile, "%s\n", (jpeg_ncomp == 1)?
    "image" : "false 3 colorimage");
  rewind(f);
  read_JPEG_file(f, FALSE, (void *)outfile);
  (void)fprintf(outfile, "grestore\n");
  break;
  #endif

  #if SUPPORT_PNG
  case IFORM_PNG:
  (void)fprintf(outfile, "gsave\n");
  (void)fprintf(outfile, "/picstr %d string def\n", png_rowbytes);
  (void)fprintf(outfile, "%s ", misc_formatfixed(x));
  (void)fprintf(outfile, "%s translate\n", misc_formatfixed(y));
  (void)fprintf(outfile, "%s dup scale\n", misc_formatfixed(scale));
  (void)fprintf(outfile, "%d %d scale\n", image_width, image_depth);
  (void)fprintf(outfile, "%d %d %d [%d 0 0 -%d 0 %d]\n", image_width,
    image_depth, 8, image_width, image_depth, image_depth);
  (void)fprintf(outfile, "{currentfile picstr readhexstring pop}\n");
  (void)fprintf(outfile, "%s\n", (png_rowbytes/image_width == 1)?
    "image" : "false 3 colorimage");
  write_png_data(outfile);
  (void)fprintf(outfile, "grestore\n");
  break;
  #endif
  }

fclose(f);
return depth;
}



/*************************************************
*            Find size of <mediaobject>          *
*************************************************/

/* This function searches the objects inside <[inline]mediaobject> for the
first one that we know how to handle. It returns the width and depth, the
latter including the depth of the caption, if present.

Arguments:
  i        the <[inline]mediaobject> item
  pwidth   pointer by which to return the width

Returns:   the depth
*/

int
object_find_size(item *i, int *pwidth)
{
int caption_depth = 0;
item *idata;
uschar *msg = US"";
uschar *filename;

/* If there's a caption, find its depth */

if (Ustrcmp(i->partner->prev->partner->name, "caption") == 0)
  {
  outputline *ol;
  for (ol = i->partner->prev->partner->next->p.prgrph->out;
       ol != NULL;
       ol = ol->next)
    caption_depth += ol->depth;
  }

/* Now search for a known object type */

for (item *j = i->next; j != i->partner; j = j->next)
  {
  if (Ustrcmp(j->name, "textobject") == 0)
    {
    int depth = textobject_depth(j);
    *pwidth = page_linewidth;
    DEBUG(D_object) debug_printf(
      "Width, depth, and caption depth of textobject are %d %d %d\n",
        *pwidth, depth, caption_depth);
    return caption_depth + depth;
    }

  if (Ustrcmp(j->name, "imageobject") == 0)
    {
    int iform;
    FILE *f = object_file(j, &msg, &idata, &filename, &iform);
    if (f != NULL)
      {
      double bb[4];
      int scale;
      int depth = imageobject_depth(f, iform, idata, &scale, bb, FALSE);
      fclose(f);
      if (depth >= 0)
        {
        *pwidth = (int)(bb[2] - bb[0]) * 1000;
        DEBUG(D_object) debug_printf(
          "Width, depth, and caption depth of object %s are %d %d %d\n",
            filename, *pwidth, depth, caption_depth);
        return caption_depth + depth;
        }
      msg = US": failed to find depth of object";
      }
    }

  DEBUG(D_object) debug_printf("Skipped %s%s\n", j->name, msg);
  j = j->partner;
  }

*pwidth = 0;
return caption_depth;
}

/* End of object.c */
