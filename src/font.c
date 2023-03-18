/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains functions related to fonts. */

#include "sdop.h"



/*************************************************
*             Find a bullet value                *
*************************************************/

/* This is used for identified lists, both in this module and also from write.c
when actually outputting.

Argument:   the <identifiedlist> item
Returns:    zero if there is no "mark" parameter
            a positive value for the bullet character
*/

int
font_get_bullet(item *i)
{
uschar *markname;
paramstr *p = misc_param_find(i, US"mark");

if (p == NULL) return 0;
markname = p->value;

if (Ustrcmp(markname, "bullet") == 0) return BULLET_BULLET;
else if (Ustrcmp(markname, "dash") == 0) return BULLET_DASH;
else if (Ustrcmp(markname, "opencircle") == 0) return BULLET_OCIRCLE;
else if (markname[0] == 'U' && markname[1] == '+')
  {
  uschar *endptr;
  unsigned int ucode = Ustrtoul(markname + 1, &endptr, 16);
  if (*endptr != 0) error(66, markname); else return ucode;
  }
return 0;
}



/*************************************************
*     Convert character name to Unicode value    *
*************************************************/

/*
Arguments:
  cname    the character name
  fname    the font name (for warning)
  warn     TRUE if warning wanted for not found
  mcptr    if not NULL, where to put the special encoding value

Returns:   a Unicode code point, or -1 if not found
*/

static int
an2u(uschar *cname, uschar *fname, BOOL warn, int *mcptr)
{
int top = an2ucount;
int bot = 0;
if (mcptr != NULL) *mcptr = -1;
while (top > bot)
  {
  int mid = (top + bot)/2;
  an2uencod *an2uitem = an2ulist + mid;
  int c = Ustrcmp(cname, an2uitem->name);
  if (c == 0)
    {
    if (mcptr != NULL) *mcptr = an2uitem->poffset;
    return an2uitem->code;
    }
  if (c > 0) bot = mid + 1; else top = mid;
  }
if (warn) (void)error(21, cname, fname);   /* Warning */
return -1;
}



/*************************************************
*        Kern table sorting comparison           *
*************************************************/

/* This is the auxiliary function used for comparing kern table entries when
sorting them. The pair values are all unsigned but subtracting them may produce
a negative number, hence the fiddling about with casts below. */

static int
table_cmp(const void *a, const void *b)
{
kerntablestr *ka = (kerntablestr *)a;
kerntablestr *kb = (kerntablestr *)b;
return (int)((int64_t)(ka->pair) - (int64_t)(kb->pair));
}



/*************************************************
*          Number reader from AFM files          *
*************************************************/

static uschar *
read_number(int *value, uschar *p)
{
int n = 0;
int sign = 1;
while (*p != 0 && *p == ' ') p++;
if (*p == '-') { sign = -1; p++; }
while (isdigit(*p)) n = n * 10 + *p++ - '0';
*value = n * sign;
return p;
}



/*************************************************
*      Load width and kern tables for a font     *
*************************************************/

/* This function is called from font_loadalltables() below. It is also called
from font_charwidth() when the first character that is in one of the special
fonts is encountered. The font structure must be initialized before calling
this function.

Arguments:
  af        points to a font structure

Returns:    void
*/

static void
font_loadtables(afontstr *af)
{
FILE *f;
kerntablestr *kerntable;
int kerncount = 0;
int finalcount = 0;
int *widths;
uschar *pp;
uschar filename[256];
uschar line[256];

if (af->widths != NULL) return;    /* We have seen this afont before */

/* First search for "fontmetrics/name" in the shared files directories; if not
found, try just "name". */

sprintf(CS line, "fontmetrics/%s.afm", af->name);
if (!misc_find_share(line, filename, FALSE))
  {
  sprintf(CS line, "%s.afm", af->name);
  (void)misc_find_share(line, filename, TRUE);  /* Hard error on failure */
  }

f = Ufopen(filename, "rb");
if (f == NULL)
  (void)error(0, filename, "font metric file", strerror(errno));   /* Hard */

DEBUG(D_fontload) debug_printf("Loading metrics for %s from %s\n",
  af->name, filename);

widths = af->widths = misc_malloc(LOWCHARLIMIT * sizeof(int));
for (int i = 0; i < LOWCHARLIMIT; i++) widths[i] = WIDTH_UNKNOWN;
af->kerncount = 0;

/* Process the AFM file. First find the start of the metrics; on the way, check
for the standard encoding scheme and for fixed pitch. */

for (;;)
  {
  if (Ufgets(line, sizeof(line), f) == NULL)
    (void)error(27, filename, "no metric data found", "");  /* Hard */
  if (memcmp(line, "EncodingScheme AdobeStandardEncoding", 36) == 0)
    {
    DEBUG(D_fontload) debug_printf("  Standard encoding\n");
    af->stdencoding = TRUE;
    }
  if (memcmp(line, "IsFixedPitch true", 17) == 0)
    {
    DEBUG(D_fontload) debug_printf("  Fixed pitch\n");
    af->fixedpitch = TRUE;
    }
  if (memcmp(line, "StartCharMetrics", 16) == 0) break;
  }

/* Process the metric lines for each character */

for (;;)
  {
  int width, code;
  int poffset = -1;

  if (Ufgets(line, sizeof(line), f) == NULL)
    (void)error(27, filename, "unexpected end of metric data", "");  /* Hard */
  if (memcmp(line, "EndCharMetrics", 14) == 0) break;

  if (memcmp(line, "C ", 2) != 0)
    (void)error(27, filename, "unrecognized metric data line: ", line); /* Hard */

  pp = line + 2;

  while (*pp != 0 && memcmp(pp, "WX", 2) != 0) pp++;
  if (*pp == 0)
    (void)error(27, filename, "unrecognized metric data line: ", line); /* Hard */
  pp = read_number(&width, pp+2);

  /* If this is a StandardEncoding font, scan the list of characters so as to
  get the Unicode value for this character. */

  if (af->stdencoding)
    {
    uschar *cname;
    while (*pp != 0 && memcmp(pp, "N ", 2) != 0) pp++;
    if (*pp == 0)
      (void)error(27, filename, "unrecognized metric data line: ", line); /* Hard */
    cname = (pp += 2);
    while (*pp != ' ') pp++;
    *pp = 0;
    code = an2u(cname, af->name, TRUE, &poffset);
    if (code < 0) continue;  /* Don't try to store anything! */
    }

  /* For other fonts, just use the character number directly. If there are
  unencoded characters, ignore them. */

  else
    {
    (void)read_number(&code, line+1);
    if (code < 0) continue;
    }

  /* Remember that this font has certain characters */

  if (code == CHAR_FI) af->hasfi = TRUE;

  /* Now put the width in an appropriate place. */

  if (code < LOWCHARLIMIT) widths[code] = width; else
    {
    tree_node *tc = misc_malloc(sizeof(tree_node) + 6);
    tc->name[misc_ord2utf8(code, tc->name)] = 0;
    tc->data.val[0] = width;
    tc->data.val[1] = poffset;
    (void)tree_insertnode(&(af->widths_tree), tc);
    }
  }

/* Process kerning data (if any); when this is done, we are finished with the
AFM file. */

for (;;)
  {
  if (Ufgets(line, sizeof(line), f) == NULL)
    {
    (void)fclose(f);
    return;        /* No kerning data */
    }
  if (memcmp(line, "StartKernPairs", 14) == 0) break;
  }

/* Find size of kern table, and get space for it. In the past, some of Adobe's
AFM files had a habit of containing a large number of kern pairs with
zero amount of kern. We leave these out of the table and adjust the count for
searching, but don't bother to free up the unused store (it isn't a vast
amount). */

pp = line + 14;
while (*pp != 0 && *pp == ' ') pp++;
(void)read_number(&kerncount, pp);
af->kerns = kerntable = misc_malloc(kerncount*sizeof(kerntablestr));

finalcount = 0;
while (kerncount--)
  {
  uschar *x;
  int sign = 1;
  int value;
  int a = -1;
  int b = -1;

  if (Ufgets(line, sizeof(line), f) == NULL)
    (void)error(27, filename, "unexpected end of kerning data", "");  /* Hard */
  if (memcmp(line, "EndKernPairs", 12) == 0) break;

  /* Skip blank lines */

  if (Ustrlen(line) <= 1)
    {
    kerncount++;
    continue;
    }

  /* Process each kern */

  pp = line + 4;

  x = pp;
  while (*pp != 0 && *pp != ' ') pp++;
  *pp++ = 0;
  a = an2u(x, af->name, FALSE, NULL);

  while (*pp != 0 && *pp == ' ') pp++;
  x = pp;
  while (*pp != 0 && *pp != ' ') pp++;
  *pp++ = 0;
  b = an2u(x, af->name, FALSE, NULL);

  /* If we haven't found the characters, ignore the kern */

  if (a >= 0 && b >= 0)
    {
    kerntable[finalcount].pair = (a << 16) + b;

    /* Read the kern value */

    while (*pp != 0 && *pp == ' ') pp++;
    if (*pp == '-') { sign = -1; pp++; }
    (void)read_number(&value, pp);
    if (value != 0) kerntable[finalcount++].kwidth = value*sign;
    }
  }

/* Adjust the count and sort into ascending order */

af->kerncount = finalcount;  /* true count */
qsort(kerntable, af->kerncount, sizeof(kerntablestr), table_cmp);

(void)fclose(f);
DEBUG(D_fontload) debug_printf("Loaded\n");


/* Early checking debugging */
#ifdef NEVER
debug_printf("FONT %s\n", af->name);
  {
  for (int i = 0; i < LOWCHARLIMIT; i++)
    {
    debug_printf("%04x %5d\n", i, af->widths[i]);
    }

  for (int i = LOWCHARLIMIT; i < 0xffff; i++)
    {
    tree_node *t;
    uschar key[4];
    key[0] = (i >> 8) & 255;
    key[1] = i & 255;
    key[2] = 0;
    t = tree_search(af->widths_tree, key);
    if (t != NULL) debug_printf("%04x %5d %5d\n", i, t->data.val[0],
      t->data.val[1]);
    }

  debug_printf("KERNS %d\n", af->kerncount);
  for (int i = 0; i < af->kerncount; i++)
    {
    kerntablestr *k = &(af->kerns[i]);
    int a = (k->pair >> 16) & 0xffff;
    int b = (k->pair) & 0xffff;
    debug_printf("%04x %04x %5d\n", a, b, k->kwidth);
    }
  }
#endif
}



/*************************************************
*         Load tables for all used fonts         *
*************************************************/

/* This function is called from the mainline, once virtual fonts have been
assigned to all text strings, and the corresponding list of actual fonts has
been built. It is called again when processing the TOC and indexes.

Arguments:   none
Returns:     TRUE
*/

BOOL
font_loadalltables(void)
{
afontstr *af;
DEBUG(D_any) debug_printf("Loading font tables\n");
for (af = afont_list; af != NULL; af = af->next) font_loadtables(af);
return TRUE;
}



/*************************************************
*        Find the width of a character           *
*************************************************/

/* If the character does not exist in the font, see if we can find it in the
Symbol or Dingbats font. If we can, ensure that appropriate afont and vfont
structures are set up for these fonts.

Arguments:
  c              the character
  vf             pointer to a vfont structure
  chtype         if non-NULL, set to the type of character:
                   CHTYPE_UNKNOWN, CHTYPE_STD, or CHTYPE_AUX

Returns:         the width, zero for unknown characters
*/

int
font_charwidth(int c, vfontstr *vf, int *chtype)
{
uschar utf[8];
int top, bot, which;
tree_node *t;
u2sencod *u2s;
afontstr *af = vf->afont;

if (chtype != NULL) *chtype = CHTYPE_STD;

/* The zero-width space does not appear in any font, and the break-permitting
characters do not appear in any standardly encoded font. */

if (c == ZERO_SPACE ||
     (af->stdencoding && (c == BREAK_PERMIT || c == NO_BREAK_HERE)))
  return 0;

/* The "hard space" character, U+00A0, does not appear in the AFM files for
some standard fonts. It is output as a space, of course. */

if (c == HARD_SPACE && af->stdencoding) c = ' ';

/* The most common, low-numbered characters, have their widths in a table.
Unknown characters have a characteristic (unreasonable) width. We do not use a
negative value, because there may be fonts that contain characters with
negative widths. */

if (c < LOWCHARLIMIT)
  {
  int w = af->widths[c];
  if (w == WIDTH_UNKNOWN)
    {
    if (chtype != NULL) *chtype = CHTYPE_UNKNOWN;
    return 0;
    }
  return MUL(w, vf->size);
  }

/* The remainder have their widths in a tree, keyed on the UTF-8 encoding of
the character. */

utf[misc_ord2utf8(c, utf)] = 0;
t = tree_search(af->widths_tree, utf);
if (t != NULL) return MUL(t->data.val[0], vf->size);

/* We have a character that is not available in the current font. Look in the
table that lists characters in the special fonts. */

bot = 0;
top = u2scount;

while (top > bot)
  {
  int mid = (top + bot)/2;
  u2s = u2slist + mid;
  if (c == u2s->ucode) break;
  if (c > u2s->ucode) bot = mid + 1; else top = mid;
  }

if (top <= bot)   /* Character not available */
  {
  if (chtype != NULL) *chtype = CHTYPE_UNKNOWN;
  return 0;
  }

/* This indicates which special font we are dealing with. */

which = u2s->which;

/* If the current vfont does not already have a pointer to this special font,
we have to set it up. First check for, and set up if necessary, an afont. Then
do the same for a vfont. */

if (vf->sfont[which] == NULL)
  {
  vfontstr *avf = NULL;
  uschar *fontname = sfontname[which];

  for (af = afont_list; af != NULL; af = af->next)
    if (Ustrcmp(af->name, fontname) == 0) break;

  /* Need to set up a new afont and load its tables. */

  if (af == NULL)
    {
    af = misc_malloc(sizeof(afontstr) + Ustrlen(fontname));
    if (afont_last == NULL) afont_list = af; else afont_last->next = af;
    afont_last = af;
    Ustrcpy(af->name, fontname);
    af->next = NULL;
    af->widths = NULL;
    af->widths_tree = NULL;
    af->kerns = NULL;
    af->kerncount = 0;
    af->psnumber = -1;
    af->stdencoding = FALSE;
    af->fixedpitch = FALSE;
    af->hasfi = FALSE;
    font_loadtables(af);
    }

  /* If an afont exists, see if there's an existing vfont of the correct size
  that points to it. */

  else
    {
    for (avf = vfont_list; avf != NULL; avf = avf->next)
      if (avf->size == vf->size && avf->afont == af) break;
    }

  /* If there is no matching auxiliary vfont, create one. */

  if (avf == NULL)
    {
    avf = misc_malloc(sizeof(vfontstr));
    if (vfont_last == NULL) vfont_list = avf; else vfont_last->next = avf;
    vfont_last = avf;
    avf->next = NULL;
    avf->family = FFAM_SPECIAL;
    avf->type = FTYPE_SPECIAL;
    avf->size = vf->size;
    avf->leading = vf->leading;
    avf->pnumber = -1;
    avf->afont = af;
    memset(avf->sfont, 0, sizeof(avf->sfont));
    }

  /* Attach it to the main vfont. */

  vf->sfont[which] = avf;
  }

/* It is assumed that any special font will have only low numbered characters,
so that their widths are always in the table in the afont. */

if (chtype != NULL) *chtype = CHTYPE_AUX;
return MUL(vf->sfont[which]->afont->widths[u2s->scode], vf->size);
}



/*************************************************
*        Find the width of a string              *
*************************************************/

/* Unknown characters are effectively ignored, because font_charwidth() returns
their width as zero.

Arguments:
  s              the string
  vf             pointer to a vfont structure

Returns:         the width
*/

int
font_stringwidth(uschar *s, vfontstr *vf)
{
int width = 0;
while (*s != 0)
  {
  int c;
  GETCHARINC(c, s);
  width += font_charwidth(c, vf, NULL);
  }
return width;
}



/*************************************************
*      Find the kern between two characters      *
*************************************************/

/* At present, the only Unicode characters recognized are those with values
less than 65536 (i.e. two bytes in binary). For fast lookup, we restrict this
to such values, and hope we never need to do anything else!

Arguments:
  lastc         the previous character, -1 if none
  c             the current character
  vf            pointer to a vfont structure

Returns:        a kern value, or 0
*/

int
font_kernwidth(int lastc, int c, vfontstr *vf)
{
uint32_t pair;
int top, bot, mid;
afontstr *af = vf->afont;

if (lastc < 0 || lastc > 0xffff || c > 0xffff) return 0;

pair = ((uint32_t)lastc << 16) | (uint32_t)c;

bot = 0;
top = af->kerncount;

while (top > bot)
  {
  kerntablestr *k;
  mid = (top + bot)/2;
  k = &(af->kerns[mid]);
  if (pair == k->pair) return MUL(k->kwidth, vf->size);
  if (pair > k->pair) bot = mid + 1; else top = mid;
  }

return 0;
}



/*************************************************
*          Take note that a font is used         *
*************************************************/

/* When a virtual font is used for the first time, set up a pointer to an
actual font block for it, and put it on the vfont chain, unless there is an
identical vfont already on the chain. This function is global because it is
called from toc.c to set up the font for the TOC fill string as well as from
font_assign() below.

Arguments:
  vf        pointer to a vfont structure
  name      for "exotic" fonts, the name; otherwise NULL

Returns:    vf if the block was added to the chain;
            else pointer to the identical block that's already on the chain
*/

vfontstr *
font_used(vfontstr *vf, uschar *fontname)
{
afontstr *af;
vfontstr *ovf;
fontsuffixstr *fs;
uschar *famname;
uschar namebuffer[64];

/* Scan for an already-seen vfont, or one that has the same parameters. */

for (ovf = vfont_list; ovf != NULL; ovf = ovf->next)
  {
  if (ovf == vf ||
      (ovf->family  == vf->family &&
       ovf->type    == vf->type &&
       ovf->size    == vf->size &&
       ovf->leading == vf->leading))
    return ovf;
  }

/* New vfont; add to the chain of vfonts that are actually used. */

if (vfont_last == NULL) vfont_list = vf; else vfont_last->next = vf;
vfont_last = vf;
vf->next = NULL;

/* Sort out the font's name */

if (fontname == NULL)
  {
  famname = type_families[vf->family];
  for (fs = fontsuffixes; fs->familyname != NULL; fs++)
    { if (Ustrcmp(famname, fs->familyname) == 0) break; }
  if (fs->familyname == NULL) (void)error(23, famname);  /* Disaster */
  sprintf(CS namebuffer, "%s%s", famname, fs->suffixes[vf->type]);
  fontname = namebuffer;
  }

for (af = afont_list; af != NULL; af = af->next)
  { if (Ustrcmp(fontname, af->name) == 0) break; }

/* Found an existing actual font block */

if (af != NULL)
  {
  vf->afont = af;
  return vf;
  }

/* Set up a new actual font block, and add it to the chain */

vf->afont = af = misc_malloc(sizeof(afontstr) + Ustrlen(fontname));
if (afont_last == NULL) afont_list = af; else afont_last->next = af;
afont_last = af;
Ustrcpy(af->name, fontname);
af->next = NULL;
af->widths = NULL;
af->widths_tree = NULL;
af->kerns = NULL;
af->kerncount = 0;
af->psnumber = -1;
af->stdencoding = FALSE;
af->fixedpitch = FALSE;
af->hasfi = FALSE;

return vf;
}



/*************************************************
*               Handle titles                    *
*************************************************/

/* This function is called when an item that may have a title is encountered.
It looks forward for the title, and assigns the given font. A second font is
provided if a subtitle for the item is supported.

Processing instructions that change the size of subscript and superscript fonts
must be recognized, to avoid unwanted messages, though in fact there is
currently no facility for changing these sizes in titles.

Arguments:
  i            the item
  vf           the font to assign
  vfs          if not NULL, the font to assign to a subtitle

Returns:       the item after the title or subtitle, if one is found
               the original item if not
*/

static item *
handle_title(item *i, vfontstr *vf, vfontstr *vfs)
{
item *ii;

for (ii = i->next; ii != NULL; ii = ii->next)
  {
  if (ISSECT(ii->name) ||
        (Ustrcmp(ii->name, "#PCDATA") == 0 &&
          (ii->p.txtblk->length != 1 || ii->p.txtblk->string[0] != '\n')) ||
      ii->partner == i) return i;
  if (Ustrcmp(ii->name, "title") == 0) break;
  }

if (ii == NULL) return i;

i = ii->partner;
for (; ii != i; ii = ii->next)
  {
  if (Ustrcmp(ii->name, "#PCDATA") == 0)
    ii->p.txtblk->vfont = font_used(vf, NULL);
  else if (Ustrcmp(ii->name, "?sdop") == 0) pin_change_font_assign(ii);
  }

if (vfs == NULL) return i;

for (ii = i->next; ii != NULL; ii = ii->next)
  {
  if (ISSECT(ii->name) ||
        (Ustrcmp(ii->name, "#PCDATA") == 0 &&
         (ii->p.txtblk->length != 1 || ii->p.txtblk->string[0] != '\n')) ||
      ii->partner == i) return i;
  if (Ustrcmp(ii->name, "subtitle") == 0) break;
  }

if (ii == NULL) return i;
i = ii->partner;

for (; ii != i; ii = ii->next)
  {
  if (Ustrcmp(ii->name, "#PCDATA") == 0)
    ii->p.txtblk->vfont = font_used(vfs, NULL);
  else if (Ustrcmp(ii->name, "?sdop") == 0) pin_change_font_assign(ii);
  }

return i;
}



/*************************************************
*         Assign fonts to string blocks          *
*************************************************/

/* This functions scans the given list of items and assigns an appropriate
vfont to each block of text. We stop at the end of the list or at an <index>
item. Afterwards, we do another pass to set up subscripts and superscripts.
This is done now so that the info can be used when sorting index items.

Arguments:
  item_list   the start of the item list to be processed
  fonts       FONTS_MAIN when processing the main text
              FONTS_TOC when processing the TOC
              FONTS_INDEX when processing an index
              FONTS_HEADFOOT when processing head/foot lines

Returns:      TRUE to continue processing
*/

#define FONTSTACKMAX 20

BOOL
font_assign(item *item_list, int fonts)
{
item *stop_at = NULL;
paramstr *pm;
vfontstr *vfontstack[FONTSTACKMAX + 1];
int bullet = -1;
int section_nest_depth = 0;
int ss_pinflags = 0;
int fontstackptr = 0;
int fontstate = 0;
int fontcolour = 0;
int fontstatestack[FONTSTACKMAX + 1];
int fontcolourstack[FONTSTACKMAX + 1];
int fontgroupstack[FONTSTACKMAX + 1];
BOOL initemizedlist = FALSE;
BOOL inorderedlist = FALSE;
BOOL startitem = FALSE;

vfontstr *current_vfont =
  (fonts == FONTS_TOC)?      &toc_maintext_vfont :
  (fonts == FONTS_TITLE)?    &title_maintext_vfont :
  (fonts == FONTS_INDEX)?    &index_maintext_vfont :
  (fonts == FONTS_HEADFOOT)? &headfoot_maintext_vfont :
                             &maintext_vfont;

DEBUG(D_any) debug_printf("Assigning fonts\n");

/* Initialize list parameters that can change during the scan */

pin_default_list_params();

/* Handle an <index> item at the start */

if (Ustrcmp(item_list->name, "index") == 0)
    {
    stop_at = item_list->partner;
    item_list = handle_title(item_list, &chapter_vfont, &chapsubt_vfont);
    }

/* Loop through all the other items */

for (item *i = item_list; i != NULL; i = i->next)
  {
  fontelstr *fe;
  uschar *name = i->name;

  if (Ustrcmp(name, "#FILENAME") == 0)
    {
    read_filename = i->p.string;
    continue;
    }
  read_linenumber = i->linenumber;

  /* Stop when we hit the start of an index (when processing main text) or the
  end of an index (when processing an index). */

  if (i == stop_at || Ustrcmp(name, "index") == 0) break;

  /* The only processing instructions that currently matter here are changes to
  the size of sub/superscript fonts, and changes to itemized and ordered list
  parameters. */

  if (Ustrcmp(name, "?sdop") == 0)
    {
    pin_change_font_assign(i);
    pin_change_list_params(i, TRUE);
    continue;
    }

  /* Check for elements that just do a font change, but can be configured */

  for (fe = fontels; fe->name != NULL; fe++)
    { if (Ustrcmp(name, fe->name) == 0) break; }

  /* If we have one, do the necessary */

  if (fe->name != NULL)
    {
    int fs = *(fe->fs);

    if (fontstackptr > FONTSTACKMAX) (void)error(19);         /* Hard */
    fontgroupstack[fontstackptr] = fonts;
    fontstatestack[fontstackptr] = fontstate;
    fontcolourstack[fontstackptr] = fontcolour;
    vfontstack[fontstackptr++] = current_vfont;
    i->flags |= IF_FONTSET;                        /* Remember for reset */

    if ((fs & FS_ITALIC) != 0)
      {
      current_vfont = italfonts[fonts + fontstate];
      fontstate |= FS_ITALIC;
      }

    if ((fs & FS_BOLD) != 0)
      {
      current_vfont = boldfonts[fonts + fontstate];
      fontstate |= FS_BOLD;
      }

    if ((fs & FS_MONO) != 0)
      {
      current_vfont = monofonts[fonts + fontstate];
      fontstate |= FS_MONO;
      }
    continue;
    }

  /* Keep track of itemized and ordered lists and section nesting, and handle
  relevant font change terminators. Footnotes can occur only in main body text,
  so there's no need to stack the "fonts" variable - we know what it is going
  to be. */

  if (Ustrcmp(name, "/") == 0)
    {
    item *pt = i->partner;
    if (Ustrcmp(pt->name, "itemizedlist") == 0) initemizedlist = FALSE;
    else if (Ustrcmp(pt->name, "orderedlist") == 0) inorderedlist = FALSE;
    else
      {
      if (ISSECT(pt->name)) section_nest_depth--;
      if ((pt->flags & IF_FONTSET) != 0)
        {
        if (fontstackptr <= 0) (void)error(20);       /* Hard */
        current_vfont = vfontstack[--fontstackptr];
        fontcolour = fontcolourstack[fontstackptr];
        fontstate = fontstatestack[fontstackptr];
        fonts = fontgroupstack[fontstackptr];
        }
      }
    }

  /* Handle chapter and similar titles */

  else if (Ustrcmp(name, "chapter")  == 0 ||
           Ustrcmp(name, "preface")  == 0 ||
           Ustrcmp(name, "colophon") == 0 ||
           Ustrcmp(name, "article")  == 0 ||
             (Ustrcmp(name, "appendix") == 0 &&
              document_type != DOC_ARTICLE))
    i = handle_title(i, &chapter_vfont, &chapsubt_vfont);

  /* Handle section titles (includes appendix in an article) */

  else if (ISSECT(name) || Ustrcmp(name, "appendix") == 0)
    i = handle_title(i,
      (fonts == FONTS_INDEX)? &index_section_vfont :
      (fonts == FONTS_TITLE)? &title_section_vfont :
      (section_nest_depth++ == 0)? &section_vfont  :
      &subsection_vfont, NULL);

  /* Handle formal paragraph titles */

  else if (Ustrcmp(name, "formalpara") == 0)
    i = handle_title(i, &formalpara_title_vfont, NULL);

  /* Handle blockquote titles */

  else if (Ustrcmp(name, "blockquote") == 0)
    i = handle_title(i, &blockquote_title_vfont, NULL);

  /* Handle note titles */

  else if (Ustrcmp(name, "note") == 0)
    i = handle_title(i, &note_title_vfont, NULL);

  /* Handle sidebar titles */

  else if (Ustrcmp(name, "sidebar") == 0)
    i = handle_title(i, &sidebar_title_vfont, NULL);

  /* Handle figure titles */

  else if (Ustrcmp(name, "figure") == 0)
    i = handle_title(i, &figure_title_vfont, NULL);

  /* Handle table titles */

  else if (Ustrcmp(name, "table") == 0)
    i = handle_title(i, &table_title_vfont, NULL);

  /* Handle example titles */

  else if (Ustrcmp(name, "example") == 0)
    i = handle_title(i, &example_title_vfont, NULL);

  /* Handle lineannotation by going back into a non-monospaced font */

  else if (Ustrcmp(name, "lineannotation") == 0)
    {
    if (fontstackptr > FONTSTACKMAX) (void)error(19);         /* Hard */
    fontgroupstack[fontstackptr] = fonts;
    fontstatestack[fontstackptr] = fontstate;
    fontcolourstack[fontstackptr] = fontcolour;
    vfontstack[fontstackptr++] = current_vfont;
    current_vfont = &small_italtext_vfont;
    fonts = FONTS_SMALL;
    fontstate = FS_ITALIC;
    i->flags |= IF_FONTSET;                        /* Remember for reset */
    }

  /* Set up emphasized text - <citetitle> and <email> behave in the same way */

  else if (Ustrcmp(name, "emphasis") == 0 ||
           Ustrcmp(name, "email")    == 0 ||
           Ustrcmp(name, "citetitle") == 0)
    {
    pm = misc_param_find(i, US"role");

    if (fontstackptr > FONTSTACKMAX) (void)error(19);         /* Hard */
    fontgroupstack[fontstackptr] = fonts;
    fontstatestack[fontstackptr] = fontstate;
    fontcolourstack[fontstackptr] = fontcolour;
    vfontstack[fontstackptr++] = current_vfont;

    if (pm == NULL || Ustrcmp(pm->value, "italic") == 0)
      {
      current_vfont = italfonts[fonts + fontstate];
      fontstate |= FS_ITALIC;
      }
    else if (Ustrcmp(pm->value, "bold") == 0)
      {
      current_vfont = boldfonts[fonts + fontstate];
      fontstate |= FS_BOLD;
      }
    else if (Ustrcmp(pm->value, "roman") == 0)
      {
      current_vfont = romanfonts[fonts + fontstate];
      fontstate &= ~(FS_BOLD|FS_ITALIC);
      }
    else if (Ustrncmp(pm->value, "booktitle", 9) == 0)
      {
      switch (pm->value[9])
        {
        default:
        case '1': current_vfont = &booktitle1_vfont; break;
        case '2': current_vfont = &booktitle2_vfont; break;
        case '3': current_vfont = &booktitle3_vfont; break;
        case '4': current_vfont = &booktitle4_vfont; break;
        }
      fontstate |= FS_BOLD;
      }
    else if (Ustrcmp(pm->value, "smallfont") == 0)
      {
      current_vfont = &small_maintext_vfont;
      fonts = FONTS_SMALL;
      fontstate = 0;
      }
    else if (Ustrncmp(pm->value, "rgb=", 4) == 0)
      {
      int c[3];
      if (misc_get_colour(pm->value+4, &c[0]))
        fontcolour = (c[0] << 20) | (c[1] << 10) | c[2];
      }

    /* If the role value is not recognized, assume an exotic font setting. */

    else
      {
      vfontstr *vf;
      int pointsize = current_vfont->size;
      int leading = current_vfont->leading;
      uschar *slash = Ustrchr(pm->value, '/');

      if (slash != NULL)
        {
        uschar *t;
        pointsize = misc_get_fp(slash + 1, &t);
        if (pointsize == 0) pointsize = current_vfont->size;
        if (*t == '/') leading = misc_get_fp(t+1, &t);
        if (*t != 0) error(79, pm->value);
        *slash = 0;
        }

      /* Search existing vfonts for one that matches. */

      for (vf = vfont_list; vf != NULL; vf = vf->next)
        {
        if (vf->family == FFAM_EXOTIC &&
            vf->afont != NULL &&
            Ustrcmp(vf->afont->name, pm->value) == 0 &&
            vf->size == pointsize &&
            vf->leading == leading)
          break;
        }

      /* Not found; create a new vfont */

      if (vf == NULL)
        {
        vf = misc_malloc(sizeof(vfontstr));
        vf->family = FFAM_EXOTIC;
        vf->type = 0;
        vf->size = MULDIV(pointsize, scale_typesize_base,
          scale_typesize_base_orig);
        vf->leading = MULDIV(leading, scale_typesize_base,
          scale_typesize_base_orig);
        vf->pnumber = 0;
        vf->afont = NULL;
        vf->sfont[0] = vf->sfont[1] = NULL;
        vf = font_used(vf, pm->value);
        }

      current_vfont = vf;
      fontstate = 0;
      if (slash != NULL) *slash = '/';
      }

    i->flags |= IF_FONTSET;                        /* Remember for reset */
    }

  /* Handle <programlisting>, <computeroutput>, <screen>, <literal>, and
  monospaced <literallayout>, which are font-changing elements that cannot be
  configured. */

  else if (Ustrcmp(name, "literal") == 0 ||
           Ustrcmp(name, "programlisting") == 0 ||
           Ustrcmp(name, "computeroutput") == 0 ||
           Ustrcmp(name, "screen") == 0 ||
            (
            Ustrcmp(name, "literallayout") == 0 &&
            (pm = misc_param_find(i, US"class")) != NULL &&
            Ustrcmp(pm->value, "monospaced") == 0
            ))
    {
    if (fontstackptr > FONTSTACKMAX) (void)error(19);         /* Hard */
    fontgroupstack[fontstackptr] = fonts;
    fontstatestack[fontstackptr] = fontstate;
    fontcolourstack[fontstackptr] = fontcolour;
    vfontstack[fontstackptr++] = current_vfont;
    current_vfont = monofonts[fonts + fontstate];
    fontstate |= FS_MONO;
    i->flags |= IF_FONTSET;                        /* Remember for reset */
    }

  /* Handle footnotes. Also handle subscripts and superscripts by using the
  foonote set of fonts. FIX ME: to do this "properly", i.e. to allow for
  sub/superscripts in footnotes, we will have to set up a whole new block of
  fonts. */

  else if (Ustrcmp(name, "footnote") == 0 ||
           (Ustrcmp(name, "subscript") == 0 && subscript_small) ||
           (Ustrcmp(name, "superscript") == 0 && superscript_small))
    {
    if (fontstackptr > FONTSTACKMAX) (void)error(19);         /* Hard */
    fontgroupstack[fontstackptr] = fonts;
    fontstatestack[fontstackptr] = fontstate;
    fontcolourstack[fontstackptr] = fontcolour;
    vfontstack[fontstackptr++] = current_vfont;
    current_vfont = &footnote_maintext_vfont;
    fonts = FONTS_FOOTNOTE;
    fontstate = 0;
    i->flags |= IF_FONTSET;                 /* Remember for reset */
    }

  /* Keep track of itemized and ordered lists. Note the start of each item in
  such as list so that we can make sure the right font is available. */

  else if (Ustrcmp(name, "itemizedlist") == 0)
    {
    initemizedlist = TRUE;
    bullet = font_get_bullet(i);
    }

  else if (Ustrcmp(name, "orderedlist") == 0) inorderedlist = TRUE;
  else if (Ustrcmp(name, "listitem") == 0) startitem = TRUE;

  /* Assign the current font to text items. Cross reference text may already
  have its colour set. There's a fudge for footnote key references and
  definitions. */

  else if (Ustrcmp(name, "#PCDATA") == 0)
    {
    if ((i->p.txtblk->pin_flags & (PIN_XREF|PIN_IREF)) == 0 ||
         i->p.txtblk->colour == -1)
      i->p.txtblk->colour = fontcolour;

    if ((i->p.txtblk->pin_flags & (PIN_FNKEYREF|PIN_FNKEYDEF|PIN_FNREFREF))
         != 0)
      i->p.txtblk->vfont = font_used(&footnote_key_vfont, NULL);
    else
      i->p.txtblk->vfont = font_used(current_vfont, NULL);

    /* Handle tags at the start of items in itemized and ordered lists. The
    vfont for these lists is a local parameter that may change in the course of
    a file. Therefore, take a copy here and ensure that a font with its
    characteristics is on the used list. In a footnote, adjust the font size
    and leading. If the copy isn't actually needed (because there's already a
    suitable font on the list), free the memory. */

    if (startitem && (initemizedlist || inorderedlist))
      {
      vfontstr *lf = initemizedlist? &ilist_tag_vfont : &olist_tag_vfont;
      vfontstr *v = misc_malloc(sizeof(vfontstr));
      *v = *lf;

      if (fonts == FONTS_FOOTNOTE)
        {
        v->size = MULDIV(v->size, footnote_maintext_vfont.size,
          maintext_vfont.size);
        v->leading = MULDIV(v->leading, footnote_maintext_vfont.leading,
          maintext_vfont.leading);
        }

      if ((lf = font_used(v, NULL)) != v) misc_free(v, sizeof(vfontstr));

      /* Having now found the font, for an itemized list with an overriding
      bullet mark, we must ensure that an appropriate auxiliary font is set up
      if necessary for the bullet character(s). Finding a character's width
      will get all the necessary work done. If bullet == 0, set up the default
      list of bullet characters. */

      if (initemizedlist && bullet >= 0)
        {
        font_loadtables(lf->afont);
        if (bullet > 0)
          (void)font_charwidth(bullet, lf, NULL);
        else
          {
          int *xp = bullets_default;
          while (*xp != 0) (void)font_charwidth(*xp++, lf, NULL);
          }
        bullet = -1;
        }

      startitem = FALSE;
      }
    }
  }

/* Now make another pass to do subscripts and superscripts. It is easier to do
this than to try to incorporate it above, where titles are handled separately
from other text. */

for (item *i = item_list; i != NULL; i = i->next)
  {
  if (Ustrcmp(i->name, "#FILENAME") == 0)
    {
    read_filename = i->p.string;
    continue;
    }
  read_linenumber = i->linenumber;

  /* Stop when we hit the start of an index (when processing main text) or the
  end of an index (when processing an index). */

  if (i == stop_at || Ustrcmp(i->name, "index") == 0) break;

  /* The only processing instructions that matter here are changes to the level
  of sub/superscripts. */

  if (Ustrcmp(i->name, "?sdop") == 0)
    {
    pin_dynamic_subsuper(i);
    continue;
    }

  else if (Ustrcmp(i->name, "subscript") == 0)
    ss_pinflags = (ss_pinflags & ~PIN_SSPERCENT) |
                (subscript_down << SSPERCENT_SHIFT) |
                PIN_SUBSCRIPT;
  else if (Ustrcmp(i->name, "superscript") == 0)
    ss_pinflags = (ss_pinflags & ~PIN_SSPERCENT) |
                (superscript_up << SSPERCENT_SHIFT) |
                PIN_SUPERSCRIPT;

  else if (Ustrcmp(i->name, "/") == 0)
    {
    if (Ustrcmp(i->partner->name, "subscript") == 0)
      ss_pinflags &= ~(PIN_SUBSCRIPT|PIN_SSPERCENT);
    else if (Ustrcmp(i->partner->name, "superscript") == 0)
      ss_pinflags &= ~(PIN_SUPERSCRIPT|PIN_SSPERCENT);
    }

  else if (Ustrcmp(i->name, "#PCDATA") == 0)
    i->p.txtblk->pin_flags |= ss_pinflags;
  }

DEBUG(D_font) debug_print_item_list(item_list, "after assigning fonts");

read_linenumber = 0;
return TRUE;
}

/* End of font.c */
