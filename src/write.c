/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains code for writing the final PostScript output. */


#include "sdop.h"

#define PLINETHRESH    60



/*************************************************
*              Static variables                  *
*************************************************/

/* Having these saves passing too many things around. */

static BOOL  all_pages;      /* TRUE if there's no page selection */
static int   ccount;         /* Count of PostScript chars in an output line */
static FILE *outfile;        /* The output file */
static int   listcount;      /* For numbering/bulleting lists */
static int   listnumeration; /* Type of numeration for ordered list */
static int   liststackptr = 0;
static int   liststack[2*MAXLISTNEST];
static int   page_count;     /* The physical page count */
static int   setcolour;
static int   setfont;        /* The currently set font */
static int   setlinewidth;   /* The currently set linewidth */
static BOOL  suppress;       /* Used to suppress unwanted pages */

static int   ytoppage;       /* The y value for the top of the page */
static int   ypos;           /* The current y position */


static uschar *footcentre;   /* Foot strings for */
static uschar *footleft;     /* the */
static uschar *footright;    /* current page */

static uschar *headcentre;   /* Head strings for */
static uschar *headleft;     /* the */
static uschar *headright;    /* current page */

static uschar *arabicpage;
static uschar *romanpage;

static uschar *chaptertitle;
static uschar *chapternumber;
static uschar *sectiontitle;
static uschar *sectionnumber;

static lengthstring *chaptitblock;
static lengthstring *secttitblock;


/* Table for head/foot entity substitutions */

typedef struct hfent {
  uschar *name;
  int length;
  uschar **value;
} hfent;

static hfent hfent_list[] = {
  { US"&arabicpage;",    12, &arabicpage },
  { US"&chapternumber;", 15, &chapternumber },
  { US"&chaptertitle;",  14, &chaptertitle },
  { US"&footcentre;",    12, &footcentre },
  { US"&footleft;",      10, &footleft },
  { US"&footright;",     11, &footright },
  { US"&headcentre;",    12, &headcentre },
  { US"&headleft;",      10, &headleft },
  { US"&headright;",     11, &headright },
  { US"&romanpage;",     11, &romanpage },
  { US"&sectionnumber;", 15, &sectionnumber },
  { US"&sectiontitle;",  14, &sectiontitle }
};

static int hfent_list_count = sizeof(hfent_list)/sizeof(hfent);



/*************************************************
*          Checked write function                *
*************************************************/

/* Writes to outfile if output is not suppressed.

Arguments:
  format       the format
  ...          the args

Returns:       number of characters output
*/

static int
cwritef(const char *format, ...)
{
int yield;
va_list ap;
if (suppress) return 0;
va_start(ap, format);
yield = vfprintf(outfile, format, ap);
va_end(ap);
return yield;
}



/*************************************************
*          Check if page is to be output         *
*************************************************/

/* This is what implements the -p and -pf options.

Arguments:
  pagenumber      the current page number
  isfm            TRUE for frontmatter

Returns:          TRUE if page is to be output
*/

static BOOL
okpage(unsigned int pagenumber, BOOL isfm)
{
BOOL odd = (pagenumber & 1) == 1;
if ((odd && !pages_odd) || (!odd && !pages_even)) return FALSE;
if (pages_front == NULL && pages_main == NULL) return TRUE;
for (pagelist *pl = isfm? pages_front : pages_main; pl != NULL; pl = pl->next)
  if (pagenumber >= pl->start && pagenumber <= pl->end) return TRUE;
return FALSE;
}



/*************************************************
*          Check space on output line            *
*************************************************/

/* If the line is getting long, write a newline; if not, write a space.

Arguments:    none
Returns:      nothing
*/

static void
pcheck(void)
{
if (ccount > PLINETHRESH)
  {
  (void)cwritef("\n");
  ccount = 0;
  }
else if (ccount > 0)
  {
  (void)cwritef(" ");
  ccount++;
  }
}



/*************************************************
*         Ensure a colour is current             *
*************************************************/

/*
Argument:  the required colour
Returns:   nothing
*/

static void
check_colour(int c)
{
if (c == setcolour) return;
ccount += fprintf(outfile, " %s ", misc_formatfixed(c >> 20));
ccount += fprintf(outfile, "%s ", misc_formatfixed((c >> 10) & 0x3ff));
ccount += fprintf(outfile, "%s setrgbcolor", misc_formatfixed(c & 0x3ff));
setcolour = c;
}



/*************************************************
*         Ensure a font is current               *
*************************************************/

/* We set the font that is appropriate to the character in the font for the
given textblock. Return the code point in the range 0-256 that is to be used
for this character.

Arguments:
  vf        the vfont
  c         the current character
  pre       a string to print before changing the font
  chfont    set TRUE if the font was changed; else FALSE
  inaux     set TRUE if the character is in an auxiliary font; else FALSE

Returns:    the encoding within the selected font
*/

static int
set_font(vfontstr *vf, int c, uschar *pre, BOOL *chfont, BOOL *inaux)
{
int code, fr, fn;

*chfont = *inaux = FALSE;

/* Code points < 256 are encoded in the first of the two PostScript fonts,
using the Unicode encoding (so no change). */

if (c < 256)
  {
  fr = 0;
  code = c;
  }

/* Code points >= 256 and < LOWCHARLIMIT are encoded in the second of the two
PostScript fonts, using the Unicode encoding less 256. */

else if (c < LOWCHARLIMIT)
  {
  fr = 1;
  code = c - 256;
  }

/* The remaining code points have to be converted either to some of the
remaining characters in the PostScript font, which are non-standardly encoded,
or to a character in one of the special fonts. */

else
  {
  uschar utf[8];
  tree_node *t;

  /* Search for the character in the widths tree for this font. If we find it,
  we should also find the offset for the printing code point. */

  utf[misc_ord2utf8(c, utf)] = 0;
  t = tree_search(vf->afont->widths_tree, utf);

  if (t != NULL)
    {
    fr = 1;
    code = LOWCHARLIMIT + t->data.val[1] - 256;
    }

  /* Otherwise, see if the character is in one of the special fonts. */

  else
    {
    int top = u2scount;
    int bot = 0;
    u2sencod *u2s;

    while (top > bot)
      {
      int mid = (top + bot)/2;
      u2s = u2slist + mid;
      if (c == u2s->ucode) break;
      if (c > u2s->ucode) bot = mid + 1; else top = mid;
      }

    /* The character is not available. Print the substitute character. For
    non-standardly encode fonts, search for the first available. */

    if (top <= bot)
      {
      fr = 0;
      if (vf->afont->stdencoding)
        code = UNKNOWN_CHAR;
      else
        {
        int i, chtype;
        for (i = 0; i < 256; i++)
          {
          (void)font_charwidth(i, vf, &chtype);
          if (chtype != CHTYPE_UNKNOWN) break;
          }
        code = (i == 256)? 255 : i;
        }
      }

    /* The character is available. The auxiliary font should have been
    set up. */

    else
      {
      *inaux = TRUE;
      fr = 0;
      code = u2s->scode;
      vf = vf->sfont[u2s->which];
      if (vf == NULL) (void)error(28);  /* Hard; should not occur */
      }
    }
  }

/* Find the font number and ensure that it is made current */

fn = vf->pnumber + fr;
if (setfont != fn)
  {
  ccount += cwritef("%s", pre);
  pcheck();
  ccount += cwritef("%d Sf", fn);
  pcheck();
  *chfont = TRUE;
  setfont = fn;
  }

return code;
}



/*************************************************
*        Ensure a linewidth is current           *
*************************************************/

/*
Argument:  the required line width
Returns:   nothing
*/

static void
set_linewidth(int linewidth)
{
if (setlinewidth != linewidth)
  {
  pcheck();
  ccount += cwritef("%s Slw", misc_formatfixed(linewidth));
  setlinewidth = linewidth;
  }
}



/*************************************************
*         Output one or more rules               *
*************************************************/

/* An arbitrary number of relative points may be specified.

Arguments:
  thickness   line thickness
  x0          starting x
  y0          starting y
  n           number of following pairs
  ...         relative pairs of x,y

Returns:      nothing
*/

static void
draw_rule(int thickness, int x0, int y0, int n, ...)
{
va_list ap;
va_start(ap, n);

set_linewidth(thickness);
check_colour(0);
pcheck();
ccount += cwritef("%s ", misc_formatfixed(x0));
ccount += cwritef("%s Mt", misc_formatfixed(y0));

while (n-- > 0)
  {
  pcheck();
  ccount += cwritef("%s ", misc_formatfixed(va_arg(ap, int)));
  ccount += cwritef("%s RLt", misc_formatfixed(va_arg(ap, int)));
  }

ccount += cwritef(" St");
}



/*************************************************
*             Output a change bar                *
*************************************************/

/* Called after passing one or more paragraphs with the changed flag set.
Add a couple of points at the bottom to get it just below the final line.

Arguments:
  yfirst     the ypos of the top marked line
  ylast      the ypos of the bottom marked line

Returns:     nothing
*/

static void
draw_bar(int yfirst, int ylast)
{
draw_rule(page_barwidth, margin_left + page_linewidth + page_baroffset,
  yfirst, 1, 0, ylast - yfirst - 3000);
}



/*************************************************
*        Write PostScript at start of page       *
*************************************************/

/* Called from write_page() below, for a real page, and also in the error case
when a page is too big and has to be split. This should never happen in reality
(but was useful during initial testing).

Argument:   the page identifier
Returns:    nothing
*/

static void
init_page(uschar *pageid)
{
if (!suppress) ++page_count;    /* Number of actual pages written */

(void)cwritef("%%%%Page: %s %d\n", pageid, page_count);
(void)cwritef("%%%%BeginPageSetup\n");
(void)cwritef("/pagesave save def\n");
(void)cwritef("%%%%EndPageSetup\n");

if (paper_size != NULL &&
    (background_colour[0] != 0 ||
     background_colour[1] != 0 ||
     background_colour[2] != 0))
  {
  (void)cwritef("0 0 Mt %g 0 RLt 0 %g RLt -%g 0 RLt closepath currentrgbcolor\n",
     paper_size_width, paper_size_height, paper_size_width);
  (void)cwritef("%s ", misc_formatfixed(background_colour[0]));
  (void)cwritef("%s ", misc_formatfixed(background_colour[1]));
  (void)cwritef("%s setrgbcolor fill setrgbcolor\n",
    misc_formatfixed(background_colour[2]));
  }

ccount = 0;         /* Count of PostScript chars on the current line */
setfont = -1;       /* Currently set font */
setlinewidth = -1;  /* Currently set line width */
setcolour = 0;      /* Currently set colour */
}



/*************************************************
*          Write PostScript for a page           *
*************************************************/

/* Loop through items till the start of the next page, outputting paragraphs as
we go. If a page overflows (should not happen for real), start a new page.

Arguments:
  i             the start of the page (the #PDATA item)
  pagenumber    the page number
  do_fn         TRUE to do only footnotes, FALSE otherwise

Returns:        for main text: the start of the next page or NULL
                for footnotes: NULL
*/

static item *
write_page(item *pagestart, int pagenumber, BOOL do_fn)
{
int overflow_page_count = 0;
int colnum = 0;
int colindent = 0;
int ytable = 0;
int yafterrow = 0;
int ytablelow = 0;
int ytopframe = 0;
int tabledent = 0;
int justspaced = -1;
int toc_fill_width = 0;
int ytopcol = ypos;
int snum, sden, stretchspace;
int chfirst = 0, chlast = 0;
int nextfn = 0;
int fnindent = do_fn? footnote_indent : 0;
int object_width = 0;
int object_justify = J_LEFT;
BOOL fnwarned = FALSE;
BOOL infootnote = FALSE;
BOOL inchanged = FALSE;
BOOL marklistitem = FALSE;
BOOL tablerowsep = TRUE;
BOOL tgrouprowsep = TRUE;
BOOL thisrowsep = TRUE;
BOOL chfont, inaux;
tdatastr *td = NULL;
pdatastr *pd = pagestart->p.pdata;
item *mediaobject = NULL;
item *i;

/* Compute the ratio values for vertically stretching, and the amount of
space that is available. Typically there won't be much available space, so we
want to use just a fraction of each stretchable space. */

stretchspace = pd->available - pd->used;
if (pd->stretchable <= stretchspace)
  {
  snum = sden = 1;
  }
else
  {
  snum = stretchspace;
  sden = pd->stretchable;
  }

/* When we are doing a real page body print (page not suppressed, not printing
footnotes), scan the page's items, and insert the correct footnote reference
numbers. To do this, we scan each output line of each paragraph for a textblock
that has the PIN_FNKEYREF flag set. Enough space has been left for 3-digit
numbers, which seems sufficiently excessive. We have to do this now, rather
than as we output the page, so that we can then process <footnoteref>s and get
the correct numbers for them. If there are more than 9 footnotes, the layout
may be poor. There is an attempt to compensate for this when we output the page
below, and that may help for stretched lines. */

if (!suppress && !do_fn)
  {
  for (i = pagestart->next;
       i != NULL && Ustrcmp(i->name, "#PDATA") != 0;
       i = i->next)
    {
    outputline *ol;
    if (Ustrcmp(i->name, "#PCPARA") != 0) continue;
    for (ol = i->p.prgrph->out; ol != NULL; ol = ol->next)
      {
      textblock *tb;
      for (tb = ol->txtblk; tb != NULL; tb = tb->next)
        {
        if ((tb->pin_flags & PIN_FNKEYREF) != 0)
          (void)sprintf(CS(tb->string), "%d", ++nextfn);
        }
      }
    }

  /* Now scan for <footnoteref>s and adjust their numbers according to the
  referenced footnotes. If a reference is not set, an error message will have
  been given earlier, during the general reference scan. */

  for (i = pagestart->next;
       i != NULL && Ustrcmp(i->name, "#PDATA") != 0;
       i = i->next)
    {
    item *ii, *refitem;
    paramstr *p;
    tree_node *tn;
    textblock *reftb = NULL;

    if (Ustrcmp(i->name, "footnoteref") != 0) continue;
    for (p = i->p.param; p != NULL; p = p->next)
      { if (Ustrcmp(p->name, "linkend") == 0) break; }
    if (p == NULL) continue;

    tn = tree_search(id_tree, p->value);
    if (tn == NULL) continue;

    /* We have a <footnote> item; scan for its reference */

    refitem = (item *)tn->data.ptr;
    for (ii = refitem->prev; ii != NULL; ii = ii->prev)
      {
      outputline *ol;
      if (Ustrcmp(ii->name, "#PCPARA") != 0) continue;
      for (ol = ii->p.prgrph->out; ol != NULL && reftb == NULL; ol = ol->next)
        {
        textblock *tb;
        for (tb = ol->txtblk; tb != NULL; tb = tb->next)
          {
          if ((tb->pin_flags & PIN_FNKEYREF) != 0)
            {
            reftb = tb;
            goto FOUNDREF;
            }
          }
        }
      }

    /* If we find a referenced footnote, we know that a dummy reference
    key was inserted. Find it, and replace its number. */

    FOUNDREF:
    if (reftb != NULL) for (ii = i->prev; ii != NULL; ii = ii->prev)
      {
      outputline *ol;
      if (Ustrcmp(ii->name, "#PCPARA") != 0) continue;
      for (ol = ii->p.prgrph->out; ol != NULL; ol = ol->next)
        {
        textblock *tb;
        for (tb = ol->txtblk; tb != NULL; tb = tb->next)
          {
          if ((tb->pin_flags & PIN_FNREFREF) != 0)
            {
            Ustrcpy(tb->string, reftb->string);
            goto NEXT_FOOTNOTEREF;
            }
          }
        }
      }

    NEXT_FOOTNOTEREF: continue;
    }
  }

/* Now process the page's items for output. If the page is being suppressed,
all we need do is keep track of the current file name and line number. We need
to recount the footnotes so as to be able to try to compensate for bad spacing
when there are more than 9. */

nextfn = 0;
for (i = pagestart->next;
     i != NULL && Ustrcmp(i->name, "#PDATA") != 0;
     i = i->next)
  {
  uschar *name = i->name;
  paragraph *pg;
  item *lastxref;
  outputline *ol;
  textblock *tb;

  if (Ustrcmp(name, "#FILENAME") == 0)
    {
    read_filename = i->p.string;
    continue;
    }
  read_linenumber = i->linenumber;

  /* If we hit an ?sdop item, check for a change to the multicolumning or to
  list parameters. */

  if (Ustrcmp(name, "?sdop") == 0)
    {
    int oldcols = page_columns;
    pin_change_columns(i);
    if (oldcols != page_columns) ytopcol = ypos;
    pin_change_list_params(i, FALSE);
    continue;
    }

  /* No need to do more if suppressing */

  if (suppress) continue;

  /* If not processing foonotes, skip footnotes */

  if (!do_fn)
    {
    if (Ustrcmp(name, "footnote") == 0)
      {
      i = i->partner;
      continue;
      }
    }

  /* If we are (only) processing footnotes, and we are not in a footnote,
  skip forward till we hit one. Otherwise, if we are in a footnote, change
  state when we reach </footnote>. */

  else if (!infootnote)
    {
    for ( ; i != NULL && Ustrcmp(i->name, "#PDATA") != 0; i = i->next)
      {
      if (Ustrcmp(i->name, "?sdop") == 0)
        {
        int oldcols = page_columns;
        pin_change_columns(i);
        if (oldcols != page_columns) ytopcol = ypos;
        pin_change_list_params(i, FALSE);
        }
      if (Ustrcmp(i->name, "footnote") == 0)
        {
        infootnote = TRUE;
        break;
        }
      }
    if (!infootnote) return NULL;
    continue;
    }

  else
    {
    if (Ustrcmp(name, "/") == 0 && Ustrcmp(i->partner->name, "footnote") == 0)
      {
      infootnote = FALSE;
      continue;
      }
    }

  /* If we hit a #PCOL item, it's the end of a column. Increase the indent
  appropriately, go back to the top of the previous column, and recompute the
  stretching parameter. */

  if (Ustrcmp(name, "#PCOL") == 0)
    {
    colindent += (page_linewidth + page_colsep)/page_columns;
    ypos = ytopcol;
    pd = i->p.pdata;
    stretchspace = pd->available - pd->used;
    if (pd->stretchable <= stretchspace)
      {
      snum = sden = 1;
      }
    else
      {
      snum = stretchspace;
      sden = pd->stretchable;
      }
    continue;
    }

  /* If we hit a <figure>, move its title to below the figure, and flag its
  text as a figure title. */

  else if (Ustrcmp(name, "figure") == 0)
    {
    item *j, *k;
    object_width = 0;
    object_justify = J_UNSET;
    for (j = i->next; j != i->partner; j = j->next)
      if (Ustrcmp(j->name, "title") == 0) break;
    if (j != i->partner)
      {
      for (k = j->next; k != j->partner; k = k->next)
        { if (Ustrcmp(k->name, "#PCPARA") == 0) k->flags |= IF_FIGTITLE; }
      j->prev->next = j->partner->next;
      j->partner->next->prev = j->prev;
      k = i->partner->prev;
      k->next = j;
      j->prev = k;
      j->partner->next = i->partner;
      i->partner->prev = j->partner;
      }
    continue;
    }

  /* If we hit a <[inline]mediaobject>, scan it to find an enclosed object that
  we can handle. If it's <textobject>, just remember that we are doing it, and
  let the normal para printing stuff below do the business. Otherwise, arrange
  for special output. */

  else if (Ustrcmp(name, "mediaobject") == 0 ||
           Ustrcmp(name, "inlinemediaobject") == 0)
    {
    item *j;
    for (j = i->next; j != i->partner; j = j->next)
      {
      if (Ustrcmp(j->name, "imageobject") == 0)
        {
        int depth = object_write_image(j, ypos, outfile, &object_width,
          &object_justify);
        if (depth >= 0)
          {
          ypos -= depth;
          ccount = 0;
          i = i->partner;
          break;
          }
        }

      if (Ustrcmp(j->name, "textobject") == 0)
        {
        mediaobject = i;
        i = j;
        break;
        }
      DEBUG(D_object) debug_printf("Skipped %s\n", j->name);
      j = j->partner;
      }
    continue;
    }

  /* At the end of a <textobject> skip on to the end of the enclosing
  <[inline]mediaobject>, unless there is a <caption> at the end (which will be
  the last thing in a <mediaobject>). */

  else if (Ustrcmp(name, "/") == 0 &&
           Ustrcmp(i->partner->name, "textobject") == 0)
    {
    i = mediaobject->partner;
    if (Ustrcmp(i->prev->partner->name, "caption") == 0)
      i = i->prev->partner;
    continue;
    }

  /* If we hit a table, find the data block and set the width. If there is a
  title, set text as a table title. Then move the <title> to above the <table>
  and arrange to re-process it. Otherwise (or when we get back here the next
  time, after the title has been processed), if there is a top frame line, draw
  it. Set a flag if this is the TOC table. */

  else if (Ustrcmp(name, "table") == 0 ||
           Ustrcmp(name, "informaltable") == 0)
    {
    item *j, *k;
    paramstr *p = misc_param_find(i, US"rowsep");
    tablerowsep = global_rowsep_default;
    if (p != NULL) tablerowsep = Ustrcmp(p->value, "0") != 0;

    for (j = i->next; j != i->partner; j = j->next)
      {
      if (Ustrcmp(j->name, "#TDATA") == 0)
        {
        td = j->p.tdata;
        break;
        }
      }
    if (td == NULL) (void)error(42);    /* Hard */

    /* These values are used when formatting a title. */

    object_width = td->twidth;
    object_justify = J_LEFT;   /* Tables are always left justified */

    /* Handle a table title */

    for (j = i->next; j != i->partner; j = j->next)
      if (Ustrcmp(j->name, "title") == 0) break;

    if (j != i->partner)
      {
      for (k = j->next; k != j->partner; k = k->next)
        { if (Ustrcmp(k->name, "#PCPARA") == 0) k->flags |= IF_TABTITLE; }

      j->prev->next = j->partner->next;
      j->partner->next->prev = j->prev;

      i->prev->next = j;
      j->prev = i->prev;

      j->partner->next = i;
      i->prev = j->partner;

      i = j;
      continue;
      }

    /* If this is the TOC table, do some initialization */

    if ((td->flags & TDF_TOC) != 0)
      {
      int c;
      uschar *pp = toc_fill_string;
      while (*pp != 0)
        {
        GETCHARINC(c, pp);
        toc_fill_width += font_charwidth(c, toc_fill_vfont_ptr, NULL);
        }
      }

    /* If not at the top of a page, ensure enough space, and do the stretch
    thing if necessary. */

    if (ypos != ytoppage)
      {
      if (justspaced < td->layparm->beforemin)
        {
        ypos -= td->layparm->beforemin - justspaced;
        justspaced = td->layparm->beforemin;
        }
      if (stretchspace > 0 && justspaced < td->layparm->beforemax)
        {
        int x = MULDIV((td->layparm->beforemax - justspaced), snum, sden);
        if (x > stretchspace) x = stretchspace;
        stretchspace -= x;
        ypos -= x;
        }
      }

    ytopframe = ypos + table_top_frame_space;

    if ((td->flags & TDF_TOPFRAME) != 0)
      {
      draw_rule(table_frame_thickness, margin_left + fnindent + td->indent,
        ytopframe, 1, td->twidth, 0);
      ypos -= table_top_frame_space;
      }

    ytablelow = ypos;
    continue;
    }

  /* If we hit the end of a table, draw the bottom and side frame lines if
  needed, draw any internal lines, and unset the widths data to indicate "not
  in table". */

  if (Ustrcmp(name, "/") == 0 &&
      (Ustrcmp(i->partner->name, "table") == 0 ||
       Ustrcmp(i->partner->name, "informaltable") == 0))
    {
    int n;
    int ybotframe = ytablelow - table_bot_frame_space;
    int sidelength = ytopframe - ybotframe + table_frame_thickness/2;

    if ((td->flags & TDF_SIDEFRAME) != 0)
      {
      if ((td->flags & TDF_BOTFRAME) != 0)
        {
        draw_rule(table_frame_thickness,
                  margin_left + fnindent + td->indent,
                  ytopframe + table_frame_thickness/2,
                  3,
                  0, -sidelength,
                  td->twidth, 0,
                  0, sidelength);
        ypos -= table_bot_frame_space;
        }

      else
        {
        draw_rule(table_frame_thickness,
                  margin_left + fnindent + td->indent,
                  ytopframe + table_frame_thickness/2,
                  1,
                  0, -sidelength);

        draw_rule(table_frame_thickness,
                  margin_left + fnindent + td->indent + td->twidth,
                  ytopframe + table_frame_thickness/2,
                  1,
                  0, -sidelength);
        }
      }

    /* Bottom without sides */

    else if ((td->flags & TDF_BOTFRAME) != 0)
      {
      draw_rule(table_frame_thickness, margin_left + fnindent + td->indent,
        ybotframe, 1, td->twidth, 0);
      ypos -= table_bot_frame_space;
      }

    /* Now deal with any verticals within the table. */

    tabledent = ((td->flags & TDF_SIDEFRAME) != 0)? table_left_col_space : 0;
    for (n = 1; n < td->colcount; n++)
      {
      tabledent += td->coldata[n].width + table_right_col_space;
      if (td->coldata[n].sep)
        draw_rule(table_frame_thickness,
          margin_left + fnindent + td->indent + tabledent,
          ytopframe + table_frame_thickness/2, 1, 0, -sidelength);
      tabledent += table_left_col_space;
      }

    /* Handle space below the table. */

    justspaced = td->layparm->aftermin;
    ypos -= justspaced;

    /* Unset table's parameters */

    td = NULL;
    tabledent = 0;
    toc_fill_width = 0;
    continue;
    }

  /* If we hit a tgroup, we must be in a table. Check for rowsep; align and
  colsep were handled when the table was formatted. */

  if (Ustrcmp(name, "tgroup") == 0)
    {
    if (td == NULL) (void)error(43, "<tgroup>"); else
      {
      paramstr *p = misc_param_find(i, US"rowsep");
      tgrouprowsep = tablerowsep;
      if (p != NULL) tgrouprowsep = Ustrcmp(p->value, "0") != 0;
      }
    continue;
    }

  /* If we hit a row, we must be in a table. Check for rowsep. Count the row,
  and reset the column count and table indent and min ypos for the row. */

  if (Ustrcmp(name, "row") == 0)
    {
    if (td == NULL) (void)error(43, "<row>"); else
      {
      paramstr *p = misc_param_find(i, US"rowsep");
      thisrowsep = tgrouprowsep;
      if (p != NULL) thisrowsep = Ustrcmp(p->value, "0") != 0;
      colnum = 0;
      tabledent = ((td->flags & TDF_SIDEFRAME) != 0)? table_left_col_space : 0;
      yafterrow = ypos;
      }
    continue;
    }

  /* If we hit the end of a row, adjust the y coodinate according to the
  deepest paragraph. Draw a separator line if not the last row and rowsep is
  set. If this is the first part of a continued table, we can never hit the
  last row. */

  if (Ustrcmp(name, "/") == 0 &&
      Ustrcmp(i->partner->name, "row") == 0)
    {
    ypos = yafterrow;
    if (thisrowsep)
      {
      BOOL drawsep = FALSE;

      if ((td->flags & TDF_CONTA) != 0) drawsep = TRUE; else
        {
        item *inext = i->next;
        while (inext != NULL && inext->name[0] == '?') inext = inext->next;
        if (Ustrcmp(inext->name, "row") == 0) drawsep = TRUE;
          else if (Ustrcmp(inext->name, "/") == 0)
            drawsep = (Ustrcmp(inext->partner->name, "thead") == 0 &&
                        (td->flags & (TDF_HASBODY|TDF_HASFOOT)) != 0) ||
                      (Ustrcmp(inext->partner->name, "tbody") == 0 &&
                        (td->flags & TDF_HASFOOT) != 0);
        }

      if (drawsep)
        {
        int ybotrow = ytablelow - table_row_sep_space;
        draw_rule(table_frame_thickness, margin_left + fnindent + td->indent,
          ybotrow, 1, td->twidth, 0);
        ypos -= table_row_sep_space;
        }
      }
    continue;
    }

  /* Handle an itemized list. If no mark is specified, search back for the most
  recent in the nest, and use the next one in a circular list. By default, use
  a bullet. */

  if (Ustrcmp(name, "itemizedlist") == 0)
    {
    liststack[liststackptr++] = listcount;
    liststack[liststackptr++] = 0;         /* Not relevant for itemized list */
    listcount = -font_get_bullet(i);
    if (listcount == 0)
      {
      int k;
      for (k = liststackptr - 1; k >= 0; k--)
        {
        if (liststack[k] < 0)
          {
          int kk;
          for (kk = 0;; kk++)
            {
            if (bullets_default[kk] == 0) break;
            if (-liststack[k] == bullets_default[kk])
              {
              kk++;
              break;
              }
            }
          if (bullets_default[kk] != 0) listcount = -bullets_default[kk];
          break;
          }
        }
      if (listcount == 0) listcount = -BULLET_BULLET;
      }
    continue;
    }

  /* Handle an ordered list */

  if (Ustrcmp(name, "orderedlist") == 0)
    {
    paramstr *p = misc_param_find(i, US"numeration");
    liststack[liststackptr++] = listcount;
    liststack[liststackptr++] = listnumeration;
    listnumeration = list_numeration_default;
    if (p != NULL)
      {
      if (Ustrcmp(p->value, "arabic") == 0) listnumeration = LN_ARABIC;
      else if (Ustrcmp(p->value, "loweralpha") == 0) listnumeration = LN_alpha;
      else if (Ustrcmp(p->value, "lowerroman") == 0) listnumeration = LN_roman;
      else if (Ustrcmp(p->value, "upperalpha") == 0) listnumeration = LN_ALPHA;
      else if (Ustrcmp(p->value, "upperroman") == 0) listnumeration = LN_ROMAN;
      else error(67, p->value);
      }
    listcount = 1;  /* Number of first item */
    continue;
    }

  /* Handle a variable list */

  if (Ustrcmp(name, "variablelist") == 0)
    {
    liststack[liststackptr++] = listcount;
    liststack[liststackptr++] = 0;    /* Not relevant for variable list */
    listcount = -BULLET_NONE;
    continue;
    }

  /* Handle the end of an itemized or ordered list */

  if (Ustrcmp(name, "/") == 0 &&
       (Ustrcmp(i->partner->name, "itemizedlist") == 0 ||
        Ustrcmp(i->partner->name, "orderedlist") == 0 ||
        Ustrcmp(i->partner->name, "variablelist") == 0))
    {
    listnumeration = liststack[--liststackptr];
    listcount = liststack[--liststackptr];
    continue;
    }

  /* Flag the start of a list item so that its mark gets inserted, except for
  variable list items, which of course have no mark. */

  if (Ustrcmp(name, "listitem") == 0)
    {
    if (listcount != -BULLET_NONE) marklistitem = TRUE;
    continue;
    }

  /* If we hit an entry, we must be in a table, and we increment the column
  number and set the parameters for printing. */

  if (Ustrcmp(name, "entry") == 0)
    {
    if (td == NULL) (void)error(43, "<entry>"); else
      {
      if (colnum++ == 0)
        {
        ytable = ypos;
        }
      else
        {
        ypos = ytable;
        tabledent += td->coldata[colnum-1].width + table_left_col_space +
          table_right_col_space;
        }
      }
    }

  /* Otherwise, ignore everything except data to print. */

  if (Ustrcmp(name, "#PCPARA") != 0) continue;

  lastxref = i;
  pg = i->p.prgrph;

  /* If this is a figure or table title, adjust the indent according to the
  width of the figure or table, not the page. If object_justify is unset, we
  haven't seen an object for this figure. */

  if ((i->flags & (IF_FIGTITLE|IF_TABTITLE)) != 0)
    {
    int para_justify = (pg->justify != J_UNSET)? pg->justify :
      pg->layparm->justify;

    if (object_justify != J_UNSET &&
        object_width < page_linewidth)
      {
      switch (object_justify)    /* Tables are always left-justified */
        {
        default:
        case J_LEFT:
        if (para_justify == J_CENTRE)
          {
          for (ol = pg->out; ol != NULL; ol = ol->next)
            {
            ol->indent = (object_width - ol->width)/2;
            if (ol->indent < 0) ol->indent = 0;
            }
          }
        else if (para_justify == J_RIGHT)
          {
          for (ol = pg->out; ol != NULL; ol = ol->next)
            {
            ol->indent = object_width - ol->width;
            if (ol->indent < 0) ol->indent = 0;
            }
          }
        break;

        case J_CENTRE:
        if (para_justify == J_CENTRE)
          {
          for (ol = pg->out; ol != NULL; ol = ol->next)
            ol->indent = (page_linewidth - ol->width)/2;
          }
        else
          {
          for (ol = pg->out; ol != NULL; ol = ol->next)
            ol->indent = (page_linewidth - object_width)/2;
          }
        break;

        case J_RIGHT:
        if (para_justify == J_CENTRE)
          {
          for (ol = pg->out; ol != NULL; ol = ol->next)
            ol->indent = page_linewidth - ((ol->width <= object_width)?
              ol->width + (object_width - ol->width)/2 : ol->width);
          }
        else
          {
          for (ol = pg->out; ol != NULL; ol = ol->next)
            ol->indent = page_linewidth - ((ol->width <= object_width)?
              object_width : ol->width);
          }
        break;
        }
      }
    }

  /* If we are in a table, add in any overall indent. */

  if (td != NULL && td->indent != 0)
    {
    for (ol = pg->out; ol != NULL; ol = ol->next)
      ol->indent += td->indent;
    }

  /* If we are not at the top of the page, and not in a table, check to see
  that there is sufficient space above this paragraph. We must also check for a
  continued paragraph here, because it won't always be at the top of a page
  when there is more than one column. Once we have the minimum space set, check
  for stretching. */

  if (ypos != ytoppage &&
      td == NULL &&
      (i->flags & IF_PARACONTB) == 0)
    {
    if (justspaced < pg->layparm->beforemin)
      {
      ypos -= pg->layparm->beforemin - justspaced;
      justspaced = pg->layparm->beforemin;
      }

    if (stretchspace > 0 && justspaced < pg->layparm->beforemax)
      {
      int x = MULDIV((pg->layparm->beforemax - justspaced), snum, sden);
      if (x > stretchspace) x = stretchspace;
      stretchspace -= x;
      ypos -= x;
      }
    }

  /* Loop for each line in the paragraph. Start a new PostScript output line
  for each one, purely to make the comparison for changes easier to do. A
  miscompare will get back in step more quickly. */

  for (ol = pg->out; ol != NULL; ol = ol->next)
    {
    BOOL changed = FALSE;
    int stretch = 0;
    int insubsuper = 0;
    int indent = margin_left + tabledent + ol->indent + colindent + fnindent;
    int xpos = indent;   /* Used only for xref processing */

    if (ccount > 0)
      {
      (void)cwritef("\n");
      ccount = 0;
      }

    /* At the start of each line, we move down by that line's depth. This
    is a combination of the font size and leading. */

    ypos -= ol->depth;

    /* This is a development hack, left in just in case. */

    if (ypos < 0 && (td == NULL || colnum >= td->colcount))
      {
      uschar buff[64];
      (void)error(26, page_count);
      (void)cwritef("\npagesave restore showpage\n"
                    "%%%%PageTrailer\n\n");
      (void)sprintf(CS buff, "XX-%d", ++overflow_page_count);
      init_page(buff);
      ypos = ytoppage;
      ytable = ytablelow = ytopframe = yafterrow = ypos;
      }

    /* Deal with a "line" that is really a rule. It won't contain any
    characters. */

    if ((ol->flags & OLF_RULE) != 0)
      draw_rule(600, margin_left + ol->indent, ypos + ol->depth/3, 1,
        ol->width, 0);

    /* Handle the start of a marked list item */

    if (marklistitem)
      {
      vfontstr *mtvfont, *olvfont;
      int back, num, den, colour;

      if (infootnote)
        {
        den = maintext_vfont.size;
        num = footnote_maintext_vfont.size;
        }
      else num = den = 1;

      if (listcount < 0)    /* Itemized list */
        {
        back = ilistpara_layparm.indent;
        mtvfont = &ilist_tag_vfont;
        colour = (ilist_tag_colour[0] << 20) | (ilist_tag_colour[1] << 10) |
                 (ilist_tag_colour[2]);
        }
      else                  /* Ordered list */
        {
        back = olistpara_layparm.indent;
        mtvfont = &olist_tag_vfont;
        colour = (olist_tag_colour[0] << 20) | (olist_tag_colour[1] << 10) |
                 (olist_tag_colour[2]);
        }

      ccount += cwritef("%s ", misc_formatfixed(indent - back));
      ccount += cwritef("%s Mt ", misc_formatfixed(ypos));

      /* Find the vfont that corresponds to the current parameters of the
      list's vfont (which may change during the input). Note that the size is
      adjusted if we are in a footnote. */

      for (olvfont = vfont_list; olvfont != NULL; olvfont = olvfont->next)
        {
        if (olvfont->family == mtvfont->family &&
            olvfont->type   == mtvfont->type &&
            olvfont->size   == (mtvfont->size*num)/den)
          break;
        }
      if (olvfont == NULL)
        error(116, (listcount < 0)? "itemized" : "ordered");  /* Fatal */

      check_colour(colour);

      /* Itemized list item */

      if (listcount < 0)
        {
        int code = set_font(olvfont, -listcount, US"", &chfont,
          &inaux);
        ccount += cwritef(
          (code == '(' || code == ')' || code == '\\')? "(\\%c) show " :
          (code >= 32  && code <= 126)? "(%c) show " : "(\\%03o) show ",
          code);
        }

      /* Enumerated list item */

      else
        {
        uschar *s;
        uschar buffer[24];
        uschar outbuffer[32];

        switch(listnumeration)
          {
          default:
          case LN_ARABIC:
          (void)sprintf(CS buffer, "%d", listcount++);
          break;

          case LN_alpha:
          (void)misc_alpha(buffer, listcount++);
          break;

          case LN_ALPHA:
          (void)misc_alpha(buffer, listcount++);

          for (s = buffer; *s != 0; s++) *s = toupper(*s);
          break;

          case LN_roman:
          (void)misc_roman(buffer, listcount++);
          break;

          case LN_ROMAN:
          (void)misc_roman(buffer, listcount++);
          for (s = buffer; *s != 0; s++) *s = toupper(*s);
          break;
          }

        ccount += cwritef("(");
        (void)sprintf(CS outbuffer, CS olist_format, buffer);

        /* Now output the numeration. */

        for (s = outbuffer; *s != 0;)
          {
          int c, code;
          GETCHARINC(c, s);
          code = set_font(olvfont, c, US")S", &chfont, &inaux);
          if (chfont)
            ccount += cwritef("(");
          else if (ccount > PLINETHRESH + 1)
            {
            (void)cwritef(")S\n(");
            ccount = 1;
            }
          ccount += cwritef(
            (code == '(' || code == ')' || code == '\\')? "\\%c" :
            (code >= 32  && code <= 126)? "%c" : "\\%03o",
            code);
          }
        ccount += cwritef(")show ");
        }

      marklistitem = FALSE;
      }

    /* If there is any text in the line and the first textblock is a footnote
    key definition, output the next number at the lefthand side and then move
    on. Then position for printing the text, but there's no need to position if
    there's no (remaining) data on the line. This isn't totally accurate, as
    really we should go through all the chain. But it saves positioning in the
    common cases. */

    if ((tb = ol->txtblk) != NULL)
      {
      if ((tb->pin_flags & PIN_FNKEYDEF) != 0)
        {
        check_colour(0);     /* Ensure black */
        (void)set_font(tb->vfont, '0', US"", &chfont, &inaux);
        ccount += cwritef("%s ", misc_formatfixed(margin_left));
        ccount += cwritef("%s Mt (%d)S ",
          misc_formatfixed(ypos + ol->depth/3), ++nextfn);
        tb =  tb->next;
        }
      if (tb != NULL && (tb->string[0] != 0 || tb->next != NULL))
        {
        ccount += cwritef("%s ", misc_formatfixed(indent));
        ccount += cwritef("%s Mt", misc_formatfixed(ypos));
        }
      }

    /* Compute data for stretching; we do not stretch the last line in a
    paragraph unless it is very nearly full, or if it is at the end of a split
    paragraph. Scan each line to see if it contains footnote references with
    numbers greater than 9. If so, see if we can accommodate the extra digits
    within the stretch. */

    if (ol->stretch > 0 && ol->scount > 0 &&
        ((ol->next != NULL && (ol->next->flags & OLF_RULE) == 0) ||
         ol->stretch <= 6000 || (i->flags & IF_PARACONTA) != 0))
      {
      textblock *ttb;
      int extrawidth = 0;
      for (ttb = tb; ttb != NULL; ttb = ttb->next)
        {
        if ((ttb->pin_flags & PIN_FNKEYREF) != 0)
          {
          if (ttb->string[1] != 0)
            extrawidth += font_charwidth('0', ttb->vfont, NULL);
          }
        }
      stretch = (ol->stretch - extrawidth)/ol->scount;
      if (stretch < 0) stretch = 0;
      }

    /* Loop for each textblock in the line */

    for ( ; tb != NULL; tb = tb->next)
      {
      int c;
      int lastc = -1;
      int ypos_adjust = 0;
      BOOL instring = FALSE;
      BOOL kerning = (tb->pin_flags & PIN_KERN) != 0;
      BOOL fiok = tb->vfont->afont->hasfi && !tb->vfont->afont->fixedpitch;
      uschar *p = tb->string;

      check_colour(tb->colour);
      if ((tb->pin_flags & PIN_REVCH) != 0) changed = TRUE;

      /* If there are more than 9 footnotes the layout may be poor, and so we
      output a warning. There is an attempt to compensate above, which may help
      for stretched lines. */

      if ((tb->pin_flags & PIN_FNKEYREF) != 0)
        {
        if (tb->string[1] != 0 && !fnwarned && !suppress)
          {
          error(73, pagenumber);
          fnwarned = TRUE;
          }
        }

      /* "Superscripts" can also be footnote identifiers; for them, we have a
      fixed proportion of 1/3. Otherwise, the movement is adjustable. */

      if ((tb->pin_flags & PIN_SUPERSCRIPT) != 0)
        {
        int depth =
          (tb->pin_flags & (PIN_FNKEYDEF|PIN_FNKEYREF|PIN_FNREFREF))?
            ol->depth/3
            :
            (ol->depth *
              (int)((tb->pin_flags >> SSPERCENT_SHIFT) & SSPERCENT_MASK))/100;
        if (insubsuper <= 0) ypos_adjust = depth - insubsuper;
        insubsuper = depth;
        }
      else if ((tb->pin_flags & PIN_SUBSCRIPT) != 0)
        {
        int depth = (ol->depth *
          ((tb->pin_flags >> SSPERCENT_SHIFT) & SSPERCENT_MASK))/100;
        if (insubsuper >= 0) ypos_adjust = -depth - insubsuper;
        insubsuper = -depth;
        }
      else if (insubsuper != 0)
        {
        ypos_adjust = -insubsuper;
        insubsuper = 0;
        }

      if (ypos_adjust != 0)
        {
        if (instring) ccount += cwritef(")S");
        instring = FALSE;
        ccount += cwritef(" 0 %s RMt", misc_formatfixed(ypos_adjust));
        }

      /* Unless this feature is disabled, if we are about to output a cross
      reference, find the page to which it refers, and output suitable pdfmark
      instructions so that when the PostScript is converted to a PDF, a
      clickable cross reference can be created. We have to terminate the string
      first; this means that a cross reference can never kern with a previous
      character, but that is such an unlikely requirement - since cross
      references are usually numbers and preceded by spaces - that we can
      ignore it. */

      if (all_pages && xref_links && (tb->pin_flags & PIN_XREF) != 0)
        {
        paramstr *pl, *pn, *yp;
        tree_node *tn;
        item *refitem;

        if (instring)
          {
          (void)cwritef(")S");
          instring = FALSE;
          }
        (void)cwritef("\n");
        ccount = 0;

        /* Find the next <xref> in the paragraph. All <xref>s have been checked
        for validity; control does not get here if there were any references to
        non-existent ids because that causes the error ec_failed, which
        suppresses any output. */

        for (lastxref = lastxref->next; lastxref != i->partner;
             lastxref = lastxref->next)
          if (Ustrcmp(lastxref->name, "xref") == 0) break;
        if (lastxref == i->partner) error(110);   /* Hard */

        /* From the <xref> we follow the linkend parameter to get to the
        original item with the matching id. There we should find a page number
        and a vertical offset from the top of the page. */

        pl = misc_param_find(lastxref, US"linkend");
        tn = tree_search(id_tree, pl->value);
        if (tn == NULL) error(111, pl->value);    /* Hard */

        refitem = (item *)tn->data.ptr;
        pn = misc_param_find(refitem, US"#pagenumber");
        if (pn == NULL) error(112, pl->value);    /* Hard */

        yp = misc_param_find(refitem, US"#pageyposition");
        if (yp == NULL) error(113, pl->value);    /* Hard */

        /* Output the appropriate pdfmark stuff for a clickable link, making
        the hot spot just a bit wider and deeper than the character string. */

        (void)cwritef("[/Rect [");
        (void)cwritef("%s ", misc_formatfixed(xpos - 1000));
        (void)cwritef("%s ", misc_formatfixed(ypos - 3000));
        (void)cwritef("%s ", misc_formatfixed(xpos + 1000 +
                               font_stringwidth(p, tb->vfont)));
        (void)cwritef("%s", misc_formatfixed(ypos + tb->vfont->size));
        (void)cwritef("] /Page %s ", pn->value);
        (void)cwritef("/View [/XYZ 0 %s null] /LNK pdfmark\n", yp->value);
        }

      /* We maintain xpos as the xposition so that we can generate hot spots
      for cross references, in the code immediately above for subsequence
      <xref>s, and in the code below for index page numbers. Add the total
      width here; adjust for stretching and kerning below. */

      xpos += font_stringwidth(p, tb->vfont);

      /* Process the characters in the textblock. */

      while (*p != 0)
        {
        int code;
        GETCHARINC(c, p);

        /* Handle special cases, in standardly encoded fonts only */

        if (tb->vfont->afont->stdencoding) switch (c)
          {
          case 'f':
          if (*p == 'i' && fiok)
            {
            c = CHAR_FI;
            p++;
            }
          break;

          /* A soft hyphen prints only if it is at the end of a line */

          case SOFT_HYPHEN:
          if (*p != 0 || tb->next != NULL) continue;
          break;

          /* A hard space is printed as a space, and can be stretched. */

          case HARD_SPACE:
          c = ' ';
          break;

          /* A zero-width space prints nothing, and prevents kerning. */

          case ZERO_SPACE:
          lastc = -1;
          continue;

          /* The break-permitting characters print nothing, but do not
          prevent kerning. */

          case BREAK_PERMIT:
          case NO_BREAK_HERE:
          continue;
          }

        /* If the character is a space, and stretching is required, output
        an explicit move instead. Don't assume a space width; re-measure it. */

        if (c == ' ' && stretch > 0)
          {
          int w = font_charwidth(' ', tb->vfont, NULL);
          if (instring)
            {
            ccount += cwritef(")S");
            instring = FALSE;
            }
          pcheck();
          ccount += cwritef("%s R", misc_formatfixed(w + stretch));
          xpos += stretch;
          lastc = -1;
          continue;
          }

        /* If we are not already in the middle of a string, there can be no
        kerning with the previous character. */

        if (!instring)
          {
          code = set_font(tb->vfont, c, US"", &chfont, &inaux);
          ccount += cwritef("(");
          instring = TRUE;
          }

        /* If we are in the middle of a string, check for kerning if the
        character is not in an auxiliary font and not a space. */

        else
          {
          int k;
          code = set_font(tb->vfont, c, US")S", &chfont, &inaux);

          /* Handle kerning */

          if (kerning && !inaux && c != ' ' &&
              (k = font_kernwidth(lastc, c, tb->vfont)) != 0)
            {
            if (!chfont)
              {
              ccount += cwritef(")S");
              pcheck();
              }
            ccount += cwritef("%s R(", misc_formatfixed(k));
            xpos += k;
            }

          /* No kerning */

          else
            {
            if (chfont)
              ccount += cwritef("(");
            else if (ccount > PLINETHRESH + 1)
              {
              cwritef(")S\n(");
              ccount = 1;
              }
            }
          }

        /* Output the character */

        ccount += cwritef(
          (code == '(' || code == ')' || code == '\\')? "\\%c" :
          (code >= 32  && code <= 126)? "%c" : "\\%03o",
          code);

        /* Cannot kern with the next if this is a space, or printed from an
        auxiliary font. */

        lastc = (c == ' ' || inaux)? -1 : c;
        }  /* For each character in the textblock */

      /* The normal case will be to have an unclosed string at the end of the
      textblock. If this is the last textblock on the line, and the line was
      hyphenated, we must output the hyphen before closing the string. We have
      to call set_font() even though the hyphen is in the same font, in case
      the previous character was in the other half of the font. This happens
      when a hyphen follows a "fi" ligature. */

      if (instring)
        {
        if (tb->next == NULL && (ol->flags & OLF_ADD_HYPHEN) != 0)
          {
          (void)set_font(tb->vfont, '-', US")S", &chfont, &inaux);
          if (chfont) ccount += cwritef("(");
          ccount += cwritef("-");
          }
        ccount += cwritef(")S");
        }

      /* Unless this feature is disabled, if we are about to output a page
      number in an index, write a pdfmark item, similar to <xref> references
      above. Working backwards from the end of the text, we pick off the last
      number. If it is preceded by ", " or by U+2013, move back for another
      number. The value of xpos is now at the end of the string we have just
      output, so we can work back from it to compute the hot spots. */

      if (all_pages && xref_links &&
          (tb->pin_flags & PIN_IREF) != 0 && tb->length > 0)
        {
        uschar *pp;
        int dwidth = font_charwidth('0', tb->vfont, NULL);   /* Digit width */

        p = tb->string;
        pp = p + tb->length;
        if (pp[-1] == ',') pp--;     /* Ends with comma if continued */

        (void)cwritef("\n");
        ccount = 0;

        for (;;)
          {
          char *end;
          int n, ixpos;
          int d = 0;
          int ok;

          while (pp-- > p && isdigit(*pp)) d++;   /* d = number of digits */
          n = Ustrtoul(pp+1, &end, 10);

          /* There may be no preceding character if we are at the start of a
          continuation line, where the indent will be set. Otherwise, the
          preceding character must be a hard space, or a real space and a
          comma, or an endash. */

          if ((pp < p && ol->indent > 0) ||                /* Continued line */
              (pp > p && *pp == 0xa0 && pp[-1] == 0xc2))   /* Hard space */
            {
            ok = 0;   /* No preceding page numbers */
            }
          else if (pp > p && *pp == ' ' && pp[-1] == ',')  /* Comma, space */
            {
            ok = 1;   /* Go back one byte afterwards */
            }
          else if (pp > p + 1 &&                           /* En dash */
                   *pp == 0x93 && pp[-1] == 0x80 && pp[-2] == 0xe2)
            {
            ok = 2;   /* Go back 2 bytes afterwards */
            }
          else break;    /* Should not occur */

          /* Output the appropriate pdfmark stuff for a clickable link, making
          the hot spot just a bit wider and deeper than the character string.
          First find the start of the character string by subtracting what
          follows the current position from the end position. */

          ixpos = xpos - font_stringwidth(pp + 1, tb->vfont);
          (void)cwritef("[/Rect [");
          (void)cwritef("%s ", misc_formatfixed(ixpos - 1000));
          (void)cwritef("%s ", misc_formatfixed(ypos - 3000));
          (void)cwritef("%s ", misc_formatfixed(ixpos + 1000 + d*dwidth));
          (void)cwritef("%s", misc_formatfixed(ypos + tb->vfont->size));
          (void)cwritef("] /Page %d ", n + title_page_count +
            toc_page_count + preface_page_count);
          (void)cwritef("/View [/XYZ 0 %s null] /LNK pdfmark\n",
            misc_formatfixed(page_full_length + margin_bottom + 40000));

          if (ok > 0) pp -= ok; else break;
          }  /* End of loop for generating clickable links */
        }    /* End of clickable link code */
      }   /* End of loop for each text block in the line */

    /* If we are in a TOC table and this is the first column, and there is a
    filling string, find the next entry and output the filling string. If there
    is no second entry, just ignore the line. Single-entry lines are used for
    blanks, and maybe in the future there may be TOCs without page numbers. */

    if (toc_fill_width != 0 && colnum == 1)
      {
      item *ii;
      outputline *oll = NULL;

      for (ii = i->next; ii != NULL; ii = ii->next)
        {
        if (Ustrcmp(ii->name, "/") == 0 &&
            Ustrcmp(ii->partner->name, "row") == 0)
          break;             /* Give up on this line */

        if (Ustrcmp(ii->name, "#PCPARA") == 0)
          {
          oll = ii->p.prgrph->out;    /* Should be page number */
          break;
          }
        }

      /* If we've found the next field, we can do the business */

      if (oll != NULL)
        {
        int avail = td->coldata[colnum].width +   /* This column width */
                    table_left_col_space +        /* Plus left and right */
                    table_right_col_space +       /* padding values */
                    oll->indent -                 /* Less indent and data */
                    ol->width -                   /* from this column, */
                    ol->indent -                  /* indent in next, */
                    toc_fill_leftspace -          /* fill left and */
                    toc_fill_rightspace;          /* right spaces. */

        if (avail >= toc_fill_width)
          {
          int times = 0;
          while (avail >= toc_fill_width)
            {
            times++;
            avail -= toc_fill_width;
            }

          pcheck();
          ccount += cwritef("%s R",
            misc_formatfixed(toc_fill_leftspace + avail));
          (void)set_font(toc_fill_vfont_ptr, toc_fill_string[0], US"", &chfont,
            &inaux);
          ccount += cwritef("(");

          while (times-- > 0)
            {
            uschar *s = toc_fill_string;
            while (*s != 0)
              {
              int c, code;
              GETCHARINC(c, s);
              code = set_font(toc_fill_vfont_ptr, c, US")S", &chfont, &inaux);
              if (chfont)
                ccount += cwritef("(");
              else if (ccount > PLINETHRESH + 1)
                {
                (void)cwritef(")S\n(");
                ccount = 1;
                }
              ccount += cwritef(
                (code == '(' || code == ')' || code == '\\')? "\\%c" :
                (code >= 32  && code <= 126)? "%c" : "\\%03o",
                code);
              }
            }

          ccount += cwritef(")S");
          }
        }
      }      /* End of TOC fill code */

    /* Deal with output lines marked "changed" */

    if (changed)
      {
      if (!inchanged)
        {
        chfirst = ypos + ol->depth;
        inchanged = TRUE;
        }
      chlast = ypos;
      }
    else if (inchanged)
      {
      draw_bar(chfirst, chlast);
      inchanged = FALSE;
      }

    /* Remember the lowest level we have printed at, for use when drawing table
    frames and row separators. */

    if (ypos < ytablelow) ytablelow = ypos;
    }     /* For each output line */

  /* If we've just printed a cell in a table, maintain the lowest depth for
  this row. */

  if (td != NULL)
    {
    if (ypos < yafterrow) yafterrow = ypos;
    }

  /* Otherwise, output the space after the paragraph, unless it is the first
  part of a split paragraph at the bottom of the page. Allow vertical
  stretching to happen. */

  else
    {
    if ((i->flags & IF_PARACONTA) != 0) justspaced = 0; else
      {
      justspaced = pg->layparm->aftermin;
      if (stretchspace > 0)
        {
        int x =
          MULDIV((pg->layparm->aftermax - pg->layparm->aftermin), snum, sden);
        if (x > stretchspace) x = stretchspace;
        justspaced += x;
        stretchspace -= x;
        }
      }
    ypos -= justspaced;
    }
  }       /* For each paragraph on the page */

/* Change bar at the end if required */

if (inchanged) draw_bar(chfirst, chlast);
return i;
}



/*************************************************
*           Find a title for head/foot           *
*************************************************/

/* Look for a titleabbrev, or failing that a title.

Argument: the item whose title we want
Returns:  a string title or NULL
*/

static lengthstring *
find_a_title(item *i)
{
lengthstring *ls = misc_find_rawtitle(i, US"#RAWTITLEABBREV");
return (ls != NULL)? ls : misc_find_rawtitle(i, US"#RAWTITLE");
}



/*************************************************
*        Scan a string for head/foot entities    *
*************************************************/

/* There are special entities that are used for replacements in heads and feet.
This function scans a string and either makes a replacement string, or just
figures out how much space is needed for the replacement. It operates
recursively by re-scanning the results of any special substitutions. If an &
character is not followed by one of the special head/foot entity names, we try
to interpret it as a normal entity. This can happen when, for example, the
title of a chapter contains named (or numerical) entities. As this function is
always called twice, give any errors only in the case of t == NULL.

Arguments:
  s              input string
  t              where to put output, or NULL for just counting

Returns:         size of replacement string
*/

static int
process_headfoot_entities(uschar *s, uschar *t)
{
int count = 0;
int top, bot, mid;

if (s == NULL) return 0;   /* Just in case unset titles, etc. */

while (*s != 0)
  {
  if (*s != '&')
    {
    if (t != NULL) *t++ = *s;
    count++;
    s++;
    continue;
    }

  bot = 0;
  top = hfent_list_count;

  while (top > bot)
    {
    int c;
    mid = (top + bot)/2;
    c = Ustrncmp(s, hfent_list[mid].name, hfent_list[mid].length);
    if (c == 0) break;
    if (c > 0) bot = mid + 1; else top = mid;
    }

  /* Not one of the special entities; try for a normal entity. */

  if (top <= bot)
    {
    uschar *tt;
    s = entity_find(s+1, &tt, (t != NULL) | suppress,
      US" in head or footlines");
    count += Ustrlen(tt);
    if (t != NULL) t += sprintf(CS t, "%s", tt);
    }

  /* Found one of the special entities; reprocess its contents. */

  else
    {
    int n = process_headfoot_entities(*(hfent_list[mid].value), t);
    count += n;
    if (t != NULL) t += n;
    s += hfent_list[mid].length;
    }
  }

if (t != NULL) *t = 0;
return count;
}



/*************************************************
*        Process a head/foot text block          *
*************************************************/

/* This function is called to make a copy of a text block for a head or foot
line, making appropriate entity substitutions in the text. At this stage, the
only entities present are the special head/foot ones. However, the contents of
these entities (for example the title of a chapter) may contain "normal"
entities.

Arguments:  the text block
Returns:    the new text block
*/

static textblock *
process_headfoot_block(textblock *tb)
{
int length = process_headfoot_entities(tb->string, NULL);
textblock *ntb = misc_malloc(sizeof(textblock) + length);
ntb->next = NULL;
ntb->vfont = tb->vfont;
ntb->pin_flags = tb->pin_flags;
ntb->colour = tb->colour;
ntb->length = length;
(void)process_headfoot_entities(tb->string, ntb->string);
return ntb;
}



/*************************************************
*          Write head or foot lines              *
*************************************************/

/* A head or foot item is potentially different each time it is printed. The
paragraphs have been set up, but not formatted. We make a copy of each
paragraph's text blocks, substituting for the special head/foot entities as we
do so. Then format the paragraphs before printing. Afterwards, the copies are
freed and the original text blocks are restored. The current file name must be
preserved and restored also.

Arguments:
  item_list   the list of items
  pagenumber  the page number

Returns:      nothing
*/

static pdatastr hfpdata = { 0, 0, 0 };
static item hfdummy = { NULL, NULL, &hfdummy, 0, 0,
  { '#','P','D','A','T','A', 0 }, { (paramstr *)(&hfpdata) } };

static void
write_headfoot(item *item_list, int pagenumber)
{
int pcount;
uschar *save_filename = read_filename;
textblock *textsave[MAXHEADFOOTPARA];

inheadorfoot = TRUE;               /* Suppresses "any-only" debugging */

/* Copy and modify the text blocks */

pcount = 0;
for (item *i = item_list; i != NULL; i = i->next)
  {
  paragraph *pp;
  textblock *tb;
  textblock **tba;

  if (Ustrcmp(i->name, "#PCPARA") != 0) continue;
  if (pcount >= MAXHEADFOOTPARA) error(50, MAXHEADFOOTPARA);  /* Hard */

  pp = i->p.prgrph;
  textsave[pcount++] = tb = pp->intxtblk;

  tba = &(pp->intxtblk);
  while (tb != NULL)
    {
    textblock *ntb = process_headfoot_block(tb);
    *tba = ntb;
    tba = &(ntb->next);
    tb = tb->next;
    }
  }

/* Format the paragraphs (most likely table entries). Carry on after errors -
is this sensible? */

(void)para_format(item_list);

/* Print the result - we stick on a dummy #PDATA item at the front, because
that's what write_page() expects. */

hfdummy.next = item_list;
write_page(&hfdummy, pagenumber, FALSE);

/* Free copied text blocks and formatted data, and restore the originals */

pcount = 0;
for (item *i = item_list; i != NULL; i = i->next)
  {
  paragraph *pp;
  outputline *ol, *nol;
  textblock *tb, *ntb;

  if (Ustrcmp(i->name, "#PCPARA") != 0) continue;
  pp = i->p.prgrph;

  for (tb = pp->intxtblk; tb != NULL; tb = ntb)
    {
    ntb = tb->next;
    misc_free(tb, sizeof(textblock) + tb->length);
    }

  for (ol = pp->out; ol != NULL; ol = nol)
    {
    nol = ol->next;
    for (tb = ol->txtblk; tb != NULL; tb = ntb)
      {
      ntb = tb->next;
      misc_free(tb, sizeof(textblock) + tb->length);
      }
    misc_free(ol, sizeof(outputline));
    }

  pp->out = NULL;
  pp->intxtblk = textsave[pcount++];
  }

inheadorfoot = FALSE;
read_filename = save_filename;
}



/*************************************************
*     Show running page number when debugging    *
*************************************************/

/* Show the current page number, overwritten, when debugging.

Arguments:
  name          name of special page type (e.g. "TOC ")
  number        page number, as text (might be roman)

Returns:        nothing
*/

static void
running_page(uschar *name, uschar *number)
{
if (debug_need_nl)
  {
  debug_need_nl = FALSE;
  debug_printf("\r                                     \r");
  }
debug_printf("====> %sPage %s", name, number);
debug_need_nl = TRUE;
}



/*************************************************
*        Write a preface or body page            *
*************************************************/

/* Called for each preface page and also for each body page.

Arguments:
  i           the start of the page
  pagenumber  the page number
  pagetext    the page number in roman or arabic
  hf          the head/foot block
  hilist      the head item list
  filist      the foot item list

Returns:      the next element after the page
*/

static item *
write_pbody_page(item *i, int pagenumber, uschar *pagetext, hfstr *hf,
  item *hilist, item *filist)
{
BOOL foundsection = FALSE;
int footnote_depth = 0;
vfontstr ilist_top_vfont, olist_top_vfont;
int ilist_top_colour[3], olist_top_colour[3];
item *nexti;

DEBUG(D_any) running_page(US"", pagetext);

/* Save list parameters at the start of the page in case they have to be reset
for a footnote scan. */

ilist_top_vfont = ilist_tag_vfont;
olist_top_vfont = olist_tag_vfont;
memcpy(ilist_top_colour, ilist_tag_colour, 3*sizeof(int));
memcpy(olist_top_colour, olist_tag_colour, 3*sizeof(int));

/* Scan the page for

(1) processing instructions that change the heading/footing data.

(2) chapter, section, and index titles, setting the variables for use in head
    and foot lines. There will only ever be one chapter title on a page, but
    there may be more than one section. We use the first of them for setting
    the variables - and only the first level sections.

(3) the presence and depth of footnotes.
*/

for (item *ii = i->next;
     ii != NULL && Ustrcmp(ii->name, "#PDATA") != 0;
     ii = ii->next)
  {
  if (Ustrcmp(ii->name, "?sdop") == 0) pin_headfoot(ii);

  else if (Ustrcmp(ii->name, "footnote") == 0)
    {
    item *fni;
    tdatastr *td = NULL;
    for (fni = ii->next; fni != ii->partner; fni = fni->next)
      {
      outputline *ol;
      if (Ustrcmp(fni->name, "#TDATA") == 0)
        {
        td = fni->p.tdata;
        if (footnote_depth != 0)
          {
          footnote_depth += td->layparm->beforemax;
          if ((td->flags & TDF_TOPFRAME) != 0)
            footnote_depth += table_top_frame_space;
          }
        if ((td->flags & TDF_BOTFRAME) != 0)
          footnote_depth += table_bot_frame_space;
        }
      else if (Ustrcmp(fni->name, "row") == 0)
        {
        footnote_depth += table_row_depth(td, fni);
        fni = fni->partner;
        }
      else if (Ustrcmp(fni->name, "#PCPARA") == 0)
        {
        if (footnote_depth != 0)
          footnote_depth += fni->p.prgrph->layparm->beforemax;
        for (ol = fni->p.prgrph->out; ol != NULL; ol = ol->next)
          footnote_depth += ol->depth;
        }
      }
    }

  else if (Ustrcmp(ii->name, "index") == 0)
    {
    chaptitblock = find_a_title(ii);
    chaptertitle = (chaptitblock == NULL)? NULL : chaptitblock->value;
    DEBUG(D_write) debug_printf("%s title=\"%s\"\n", ii->name, chaptertitle);
    }

  else if (Ustrcmp(ii->name, "chapter")  == 0 ||
           Ustrcmp(ii->name, "preface")  == 0 ||
           Ustrcmp(ii->name, "article")  == 0 ||
             (Ustrcmp(ii->name, "appendix") == 0 &&
              document_type != DOC_ARTICLE))
    {
    paramstr *p = misc_param_find(ii, US"#number");
    chapternumber = (p == NULL)? US "" : p->value;
    chaptitblock = find_a_title(ii);
    chaptertitle = (chaptitblock == NULL)? NULL : chaptitblock->value;
    if (chapter_skip_head) i->flags |= IF_NOHEADFOOT;
    DEBUG(D_write) debug_printf("%s number=\"%s\" title=\"%s\"\n", ii->name,
      chapternumber, chaptertitle);
    }

  else if (!foundsection &&
            (ISSECT(ii->name) || Ustrcmp(ii->name, "appendix") == 0))
    {
    paramstr *p = misc_param_find(ii, US"#number");
    uschar *number = (p == NULL)? US "" : p->value;
    /* Top-level section has only one '.' in its number */
    if (Ustrchr(number, '.') == Ustrrchr(number, '.'))
      {
      sectionnumber = number;
      secttitblock = find_a_title(ii);
      sectiontitle = (secttitblock == NULL)? NULL : secttitblock->value;
      DEBUG(D_write) debug_printf("sectionnumber=\"%s\" title=\"%s\"\n",
        sectionnumber, sectiontitle);
      foundsection = TRUE;
      }
    }
  }

/* Set up the head/foot */

if ((pagenumber & 1) == 0)             /* Even page number */
  {
  footleft = hf->foot_left_verso;
  footcentre = hf->foot_centre_verso;
  footright = hf->foot_right_verso;
  headleft = hf->head_left_verso;
  headcentre = hf->head_centre_verso;
  headright = hf->head_right_verso;
  }
else                                   /* Odd page number */
  {
  footleft = hf->foot_left_recto;
  footcentre = hf->foot_centre_recto;
  footright = hf->foot_right_recto;
  headleft = hf->head_left_recto;
  headcentre = hf->head_centre_recto;
  headright = hf->head_right_recto;
  }

/* OK, now we can write the data for the page */

init_page(pagetext);
if (page_head_length > 0 && (i->flags & IF_NOHEADFOOT) == 0)
  {
  ypos = ytoppage = page_full_length + margin_bottom;
  DEBUG(D_write) debug_printf("Processing header\n");
  write_headfoot(hilist, pagenumber);
  }

DEBUG(D_write) debug_printf("Processing page data\n");
ypos = ytoppage = page_full_length + margin_bottom - page_head_length;
nexti = write_page(i, pagenumber, FALSE);

/* Reset ilist and olist parameters for a footnote scan. */

if (footnote_depth > 0)
  {
  DEBUG(D_write) debug_printf("Footnote depth=%d\n", footnote_depth);
  ilist_tag_vfont = ilist_top_vfont;
  olist_tag_vfont = olist_top_vfont;
  memcpy(ilist_tag_colour, ilist_top_colour, 3*sizeof(int));
  memcpy(olist_tag_colour, olist_top_colour, 3*sizeof(int));
  ypos = ytoppage = margin_bottom + page_foot_length + footnote_depth;
  draw_rule(footnote_line_thickness, margin_left, ypos + 1000, 1,
    footnote_line_length, 0);
  (void)write_page(i, pagenumber, TRUE);
  }

if (page_foot_length > 0 && (i->flags & IF_NOHEADFOOT) == 0)
  {
  ypos = ytoppage = margin_bottom + page_foot_length;
  DEBUG(D_write) debug_printf("Processing footer\n");
  write_headfoot(filist, pagenumber);
  }

(void)cwritef("\npagesave restore showpage\n%%%%PageTrailer\n\n");

/* Finally, we must now scan to find the *last* section name in this page, in
case there is no such name on the next page. */

for (item *ii = i->next;
     ii != NULL && Ustrcmp(ii->name, "#PDATA") != 0;
     ii = ii->next)
  {
  if (ISSECT(ii->name))
    {
    paramstr *p = misc_param_find(ii, US"#number");
    uschar *number = (p == NULL)? US"" : p->value;
    /* Top-level section has only one '.' in its number */
    if (Ustrchr(number, '.') == Ustrrchr(number, '.'))
      {
      sectionnumber = number;
      secttitblock = find_a_title(ii);
      sectiontitle = (secttitblock == NULL)? NULL : secttitblock->value;
      DEBUG(D_write) debug_printf("sectionnumber=\"%s\" title=\"%s\"\n",
        sectionnumber, sectiontitle);
      }
    }
  }

return nexti;
}



/*************************************************
*       Set defaults before writing pages        *
*************************************************/

/* Called twice; once before the dummy run to sort out characters in heads
and feet, and again before the real run.

Argument:     none
Returns:      nothing
*/

static void
set_pagedata_defaults(void)
{
page_count = 0;
page_columns = page_columns_init;
page_colsep = page_colsep_init;
main_headfoot = main_headfoot_default;
pin_default_list_params();
}




/*************************************************
*             Write the output file              *
*************************************************/

/*
Argument:     the name of the output file
Returns:      TRUE/FALSE
*/

BOOL
write_file(uschar *filename)
{
int afontnumber, vfontcount;
unsigned int pagenumber;
item *i;
FILE *ph;
time_t timer;
afontstr *af;
vfontstr *vf;
pdfmarkstr *pdf;
uschar buffer[1024];
uschar arabic[12];
uschar roman[12];

arabicpage = arabic;
romanpage = roman;

/* Set all_pages TRUE if we are outputting all the pages (i.e. there has been
no selection of specific pages only. */

all_pages = pages_even && pages_odd &&
            pages_main == NULL && pages_front == NULL;

/* Set up the output file. */

if (filename == NULL || Ustrcmp(filename, "-") == 0)
  {
  DEBUG(D_any) debug_printf("==> Writing to stdout\n");
  outfile = stdout;
  }
else
  {
  DEBUG(D_any) debug_printf("==> Writing to %s\n", filename);
  outfile = Ufopen(filename, "wb");
  if (outfile == NULL)
    (void)error(0, filename, "output file", strerror(errno));  /* Hard error */
  }


/* Before we do the real output, we do a dummy run through the preface and body
pages, with suppress forced. This has the effect of processing the heads and
feet, which may contain text from chapter or section titles. This may include
characters that require additional vfonts to be bound (e.g. special
characters); the initial scan of the head/foot lines won't have picked these
up. Processing the head/foot as a table causes font_charwidth() to be called
for all the characters, and this has the effect of setting up any missing
vfonts. */

set_pagedata_defaults();
pagenumber = 0;
suppress = TRUE;

/* ---------- DUMMY: The preface pages ---------- a*/

if (preface_item_list != NULL)
  {
  chaptertitle = chapternumber = NULL;
  sectiontitle = sectionnumber = NULL;
  chaptitblock = secttitblock = NULL;

  i = preface_item_list->next;       /* The first #PDATA (page data) item */

  DEBUG(D_any) if (i != NULL) debug_printf("Dummy scan of preface\n");

  while (i != NULL && i->next != NULL)
    {
    (void)sprintf(CS arabicpage, "%d", ++pagenumber);
    (void)misc_roman(romanpage, pagenumber);
    margin_left = ((pagenumber & 1) == 0)? margin_left_recto:margin_left_verso;
    i = write_pbody_page(i, pagenumber, romanpage, &preface_headfoot,
      preface_head_item_list, preface_foot_item_list);
    }
  }

/* ------ DUMMY: The main body pages, appendices, indexes, colophons ------ */

chaptertitle = chapternumber = NULL;
sectiontitle = sectionnumber = NULL;
chaptitblock = secttitblock = NULL;

i = main_item_list->next;       /* The first #PDATA (page data) item */

DEBUG(D_any) if (i != NULL) debug_printf("Dummy scan of main body etc.\n");

while (i != NULL && i->next != NULL)
  {
  (void)sprintf(CS arabicpage, "%d", ++pagenumber);
  (void)misc_roman(romanpage, pagenumber);
  margin_left = ((pagenumber & 1) == 0)? margin_left_recto : margin_left_verso;
  i = write_pbody_page(i, pagenumber, arabicpage, &main_headfoot,
    main_head_item_list, main_foot_item_list);
  }


/* ------ REAL ------ */

/* Re-initialize things and output the beginning of the PostScript. */

set_pagedata_defaults();
time(&timer);
suppress = FALSE;

(void)cwritef("%%!PS-Adobe-3.0\n");
(void)cwritef("%%%%Creator: SDoP %s\n", SDOP_VERSION);
(void)cwritef("%%%%CreationDate: %s", ctime(&timer));
(void)cwritef("%%%%Pages: (atend)\n");

(void)cwritef("%%%%DocumentNeededResources:\n");
for (af = afont_list; af != NULL; af = af->next)
  {
  afontstr *bf;
  for (bf = afont_list; bf != af; bf = bf->next)
    { if (Ustrcmp(af->name, bf->name) == 0) break; }
  if (af == bf) (void)cwritef("%%%%+ font %s\n", af->name);
  }

(void)cwritef("%%%%Requirements: numcopies(1)\n");
(void)cwritef("%%%%EndComments\n\n");

/* Copy the PostScript header file, omitting any single-% comments and
any blank lines. */

(void)misc_find_share(US"PSheader", buffer, TRUE);
ph = Ufopen(buffer, "rb");
if (ph == NULL)  /* Hard */
  (void)error(0, buffer, "PostScript header file", strerror(errno));

while (Ufgets(buffer, sizeof(buffer), ph) != NULL)
  {
  if (buffer[0] == '\n' || (buffer[0] == '%' && buffer[1] != '%')) continue;
  (void)cwritef("%s", CS buffer);
  }
(void)fclose(ph);

/* Do the font binding in the general setup section. */

(void)cwritef("\n%%%%BeginSetup\n");

/* Use this to prevent compression if you want to look at a PDF generated from
this PostScript. */

#if 0
(void)cwritef("systemdict /setdistillerparams known {\n"
  "<< /CompressPages false >> setdistillerparams\n} if\n");
#endif

/* Handle paper size, if set */

if (paper_size != NULL)
  (void)cwritef("<< /PageSize [ %g %g ] >> setpagedevice\n", paper_size_width,
    paper_size_height);

/* Let a PDF know how the pages are labelled: the front matter, Contents and
Preface in lower case Roman, then the rest in Arabic. Suppress this if the
output has only certain pages selected, because it doesn't then work. */

if (all_pages)
  {
  (void)cwritef("[ {Catalog} << /PageLabels << /Nums [\n"
    "0 << /S /r >> "
    "%d << /S /D >> "
    "] >> >> /PUT pdfmark\n",
    title_page_count + toc_page_count + preface_page_count);

  /* Set up the default viewing state, show the bookmarks (outlines). */

  (void)cwritef(
    "[/View [/XYZ null null 1] /Page 1 /PageMode /UseOutlines "
    "/DOCVIEW pdfmark\n");

  /* Bookmark data for the main text is set up during TOC processing. Remember
  that page numbers start from 1. */

  if (title_page_count != 0)
    (void)cwritef(
      "[/Title (Title page) /Page 1 /View [/XYZ null null 1] /OUT pdfmark\n");

  if (toc_item_list->next != NULL)
    (void)cwritef(
      "[/Title (Contents) /Page %d /View [/XYZ null null 1] /OUT pdfmark\n",
      title_page_count + 1);

  /* At each level we need to know the count of subordinate bookmarks at the
  next level (*not* including their subordinates). If there are none, output
  nothing. Otherwise output /Count -n to suppress their showing initially. */

  for (pdf = toc_pdfmarks; pdf != NULL; pdf = pdf->next)
    {
    pdfmarkstr *pt;
    uschar *t = pdf->text;
    int count = 0;

    for (pt = pdf->next; pt != NULL && pt->level != pdf->level; pt = pt->next)
      { if (pt->level == pdf->level + 1) count++; }

    (void)cwritef("[/Title (");

    while (*t != 0)
      {
      int code;
      GETCHARINC(code, t);
      (void)cwritef(
        (code == HARD_SPACE)?    " " :
        (code == SOFT_HYPHEN)?   ""  :
        (code == ZERO_SPACE)?    ""  :
        (code == BREAK_PERMIT)?  ""  :
        (code == NO_BREAK_HERE)? ""  :
        (code == OPEN_SQUOTE)?   "'" :
        (code == CLOSE_SQUOTE)?  "'" :
        (code == OPEN_DQUOTE)?   "\"" :
        (code == CLOSE_DQUOTE)?  "\"" :
        (code == CHAR_FI)?       "fi" :
        (code == BULLET_DASH)?   "-" :
        (code == CHAR_ENDASH)?   "-" :
        (code == CHAR_EMDASH)?   "-" :
        (code > 255)? "?" :
        (code == '(' || code == ')' || code == '\\')? "\\%c" :
        (code >= 32  && code <= 126)? "%c" : "\\%03o", code);
      }
    (void)cwritef(") /Page %d /View [/XYZ null null 1] ",
      title_page_count + toc_page_count + pdf->page +
        (pdf->ispreface? -PREFACE_DUMMY_PAGE : preface_page_count));
    if (count > 0) (void)cwritef("/Count %d ", -count);
    (void)cwritef("/OUT pdfmark\n");
    }
  }

/* Find and possibly re-encode the actual fonts. */

afontnumber = 0;
for (af = afont_list; af != NULL; af = af->next)
  {
  (void)cwritef("%%%%IncludeResource: font %s\n", af->name);
  (void)cwritef("/af%d /af%d /%s inf\n", afontnumber, afontnumber+1,
    af->name);
  af->psnumber = afontnumber;
  afontnumber += 2;
  }

/* Now, for each vfont that is actually used, scale the appropriate fonts
and put them into an array. For each vfont there may be two PostScript fonts,
to allow for more than 256 characters. */

vfontcount = 0;
for (vf = vfont_list; vf != NULL; vf = vf->next) vfontcount++;

(void)cwritef("/vf %d array def\n", vfontcount * 2);

vfontcount = 0;
for (vf = vfont_list; vf != NULL; vf = vf->next)
  {
  vf->pnumber = vfontcount;
  (void)cwritef("vf %d af%d %s scalefont put\n", vfontcount++,
    vf->afont->psnumber, misc_formatfixed(vf->size));
  (void)cwritef("vf %d af%d %s scalefont put\n", vfontcount++,
    vf->afont->psnumber + 1, misc_formatfixed(vf->size));
  }

(void)cwritef("%%%%EndSetup\n\n");

/* Now output the selected pages. */


/* ---------- The title pages ---------- */

pagenumber = 0;                /* For title, TOC, preface */

i = title_item_list->next;     /* The first #PDATA (page data) item */
DEBUG(D_any) if (i != NULL) debug_printf("Writing title pages\n");

while (i != NULL && i->next != NULL)
  {
  (void)sprintf(CS arabicpage, "%d", ++pagenumber);
  (void)misc_roman(romanpage, pagenumber);
  DEBUG(D_any) running_page(US"Title ", romanpage);
  suppress = !okpage(pagenumber, TRUE);
  init_page(romanpage);
  DEBUG(D_write) debug_printf("Processing page data\n");
  ypos = ytoppage = page_full_length + margin_bottom;
  margin_left = ((pagenumber & 1) == 0)? margin_left_recto : margin_left_verso;
  i = write_page(i, pagenumber, FALSE);
  (void)cwritef("\npagesave restore showpage\n%%%%PageTrailer\n\n");
  }


/* ---------- The TOC pages ---------- */

i = toc_item_list->next;       /* The first #PDATA (page data) item */

DEBUG(D_any) if (i != NULL) debug_printf("Writing TOC\n");

while (i != NULL && i->next != NULL)
  {
  item *nexti;

  (void)sprintf(CS arabicpage, "%d", ++pagenumber);
  (void)misc_roman(romanpage, pagenumber);
  suppress = !okpage(pagenumber, TRUE);
  DEBUG(D_any) running_page(US"TOC ", romanpage);

  /* Choose appropriate head/foot definitions */

  if ((pagenumber & 1) == 0)             /* Even page number */
    {
    footleft = toc_headfoot.foot_left_verso;
    footcentre = toc_headfoot.foot_centre_verso;
    footright = toc_headfoot.foot_right_verso;
    headleft = toc_headfoot.head_left_verso;
    headcentre = toc_headfoot.head_centre_verso;
    headright = toc_headfoot.head_right_verso;
    }
  else                                   /* Odd page number */
    {
    footleft = toc_headfoot.foot_left_recto;
    footcentre = toc_headfoot.foot_centre_recto;
    footright = toc_headfoot.foot_right_recto;
    headleft = toc_headfoot.head_left_recto;
    headcentre = toc_headfoot.head_centre_recto;
    headright = toc_headfoot.head_right_recto;
    }

  /* OK, now we can write the data for the page */

  init_page(romanpage);
  margin_left = ((pagenumber & 1) == 0)? margin_left_recto : margin_left_verso;
  if (page_head_length > 0 && (i->flags & IF_NOHEADFOOT) == 0)
    {
    ypos = ytoppage = page_full_length + margin_bottom;
    DEBUG(D_write) debug_printf("Processing header\n");
    write_headfoot(toc_head_item_list, pagenumber);
    }

  DEBUG(D_write) debug_printf("Processing page data\n");
  ypos = ytoppage = page_full_length + margin_bottom - page_head_length;
  nexti = write_page(i, pagenumber, FALSE);

  if (page_foot_length > 0 && (i->flags & IF_NOHEADFOOT) == 0)
    {
    ypos = ytoppage = margin_bottom + page_foot_length;
    DEBUG(D_write) debug_printf("Processing footer\n");
    write_headfoot(toc_foot_item_list, pagenumber);
    }

  (void)cwritef("\npagesave restore showpage\n%%%%PageTrailer\n\n");

  /* Move on to next page */

  i = nexti;
  }


/* ---------- The preface pages ---------- */

if (preface_item_list != NULL)
  {
  chaptertitle = chapternumber = NULL;
  sectiontitle = sectionnumber = NULL;
  chaptitblock = secttitblock = NULL;

  i = preface_item_list->next;       /* The first #PDATA (page data) item */

  DEBUG(D_any) if (i != NULL) debug_printf("Writing preface\n");

  while (i != NULL && i->next != NULL)
    {
    (void)sprintf(CS arabicpage, "%d", ++pagenumber);
    (void)misc_roman(romanpage, pagenumber);
    suppress = !okpage(pagenumber, TRUE);
    margin_left = ((pagenumber & 1) == 0)? margin_left_recto:margin_left_verso;
    i = write_pbody_page(i, pagenumber, romanpage, &preface_headfoot,
      preface_head_item_list, preface_foot_item_list);
    }
  }


/* -------- The main body pages, appendices, indexes, colophons -------- */

pagenumber = 0;                 /* Restart the numbering */

chaptertitle = chapternumber = NULL;
sectiontitle = sectionnumber = NULL;
chaptitblock = secttitblock = NULL;

i = main_item_list->next;       /* The first #PDATA (page data) item */

DEBUG(D_any) if (i != NULL) debug_printf("Writing main body etc.\n");

while (i != NULL && i->next != NULL)
  {
  (void)sprintf(CS arabicpage, "%d", ++pagenumber);
  (void)misc_roman(romanpage, pagenumber);
  suppress = !okpage(pagenumber, FALSE);
  margin_left = ((pagenumber & 1) == 0)? margin_left_recto : margin_left_verso;
  i = write_pbody_page(i, pagenumber, arabicpage, &main_headfoot,
    main_head_item_list, main_foot_item_list);
  }

/* Write terminating stuff and close the file. */

suppress = FALSE;
(void)cwritef("%%%%Trailer\n%%%%Pages: %d\n", page_count);
(void)fclose(outfile);

DEBUG(D_any) debug_printf("Finished writing\n");

return TRUE;
}

/* End of write.c */
