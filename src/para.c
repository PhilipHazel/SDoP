/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains functions for doing things to paragraphs. */

#include "sdop.h"

/* Looseness parameters for analyzing lines. */

#define L_BAD     95
#define L_ACCEPT  60
#define L_VGOOD   30


/*************************************************
*          Local static variables                *
*************************************************/

static unsigned int pin_flags = 0;
static int error53_linenumber = 0;
static item *fni;



/*************************************************
*        Find an <attribution> element           *
*************************************************/

/* This is called when we hit <blockquote> or <epigraph>.

Argument:  the element within which to search
Returns:   the <attribution> element, or NULL
*/

static item *
find_attribution(item *i)
{
for (item *ii = i->next; ii != i->partner; ii = ii->next)
  if (Ustrcmp(ii->name, "attribution") == 0) return ii;
return NULL;
}



/*************************************************
*         Add rules at top/bottom                *
*************************************************/

/* This function is called when an item is to be ruled off. It adds the
relevant rule flags to the first and last paragraphs of the item. The start and
end are passed separately, because a logical item may in fact be more than one
pair (e.g. block quote + attribution).

Arguments:
  begin     start item
  end       end item

Returns:    nothing
*/

static void
add_rules(item *begin, item *end)
{
for (item *i = begin->next; i != end; i = i->next)
  {
  if (Ustrcmp(i->name, "#PCPARA") == 0)
    {
    i->flags |= IF_RULEABOVE;
    break;
    }
  }
for (item *i = end->prev; i != begin; i = i->prev)
  {
  if (Ustrcmp(i->name, "#PCPARA") == 0)
    {
    i->flags |= IF_RULEBELOW;
    break;
    }
  }
}



/*************************************************
*    Set figure title width and justification    *
*************************************************/

/* This is called when we reach the end of a figure, to scan for a title and
set the widths of its paragraphs. If we can't find an appropriate width, set
nothing. The default - the full line width - will then be used. This function
also sets an overriding justify if one has been set.

Argument:  the figure item
Returns:   nothing
*/

static void
set_figure_title_width(item *i)
{
item *t;
int width;

for (t = i->next; t != i->partner; t = t->next)
  if (Ustrcmp(t->name, "title") == 0) break;
if (t == i->partner) return;     /* No title found */

/* The title width is either fixed, or to be taken from a mediaobject that
forms the figure. */

if (figure_title_width > 0) width = figure_title_width; else
  {
  item *m;
  for (m = t->partner->next; m != i->partner; m = m->next)
    if (Ustrcmp(m->name, "mediaobject") == 0 ||
        Ustrcmp(m->name, "inlinemediaobject") == 0)
      break;
  if (m == i->partner) width = 0;
    else (void)object_find_size(m, &width);
  }

if (width > page_linewidth) width = 0;

/* Scan the title's paragraphs */

for (i = t->next; i != t->partner; i = i->next)
  {
  if (Ustrcmp(i->name, "#PCPARA") != 0) continue;
  if (figure_title_justify != J_UNSET)
    i->p.prgrph->justify = figure_title_justify;
  if (width > 0) i->p.prgrph->maxwidth = width;
  }
}



/*************************************************
*    Set example title width and justification   *
*************************************************/

/* This is called when we reach the end of an example, to scan for a title and
set the widths of its paragraphs. If we can't find an appropriate width, set
nothing. The default - the full line width - will then be used. This function
also sets an overriding justify if one has been set.

Argument:  the example item
Returns:   nothing
*/

static void
set_example_title_width(item *i)
{
item *t;
int width = 0;

for (t = i->next; t != i->partner; t = t->next)
  if (Ustrcmp(t->name, "title") == 0) break;
if (t == i->partner) return;     /* No title found */

/* The title width is either fixed, or left unset (=> full line width). */

if (example_title_width > 0 &&
    example_title_width <= page_linewidth) width = example_title_width;

/* Scan the title's paragraphs */

for (i = t->next; i != t->partner; i = i->next)
  {
  if (Ustrcmp(i->name, "#PCPARA") != 0) continue;
  if (example_title_justify != J_UNSET)
    i->p.prgrph->justify = example_title_justify;
  if (width > 0) i->p.prgrph->maxwidth = width;
  }
}



/*************************************************
*             Create paragraph                   *
*************************************************/

/* This function inserts a #PCPARA item into the item list, hangs a paragraph
item off it, and chains the relevant input text blocks onto the paragraph item.

Arguments:
  i            the item that starts the paragraph
  lp           the layout parameters
  fnindent     0 for normal paragraphs, the footnote indent for footnotes

Returns:       the partner item
*/

static item *
create_para(item *i, layoutparam *lp, int fnindent)
{
item *ii, *inew;
paragraph *pg;
textblock **tbptr;

/* Set up the paragraph block for the text, and hang it off an inserted #PCPARA
item. */

pg = misc_malloc(sizeof(paragraph));
pg->layparm = lp;                                /* Layout parameters */
pg->out = NULL;                                  /* No output yet */
pg->intxtblk = NULL;                             /* No input yet */
pg->justify = J_UNSET;                           /* Use value from layout */
pg->extra_leading = extra_leading;               /* The dynamic value */
tbptr = &(pg->intxtblk);                         /* Where to chain the input */

/* The maximum width is computed from the page width and the number of columns
on the page. */

pg->maxwidth =
  ((page_linewidth - (page_columns - 1)*page_colsep)/page_columns) - fnindent;

/* Now the new item. */

inew = misc_malloc(sizeof(item));
Ustrcpy(inew->name, "#PCPARA");
inew->partner = inew;

inew->next = i->next;
inew->prev = i;
if (i->next != NULL) i->next->prev = inew;
i->next = inew;

inew->linenumber = i->linenumber;
inew->flags = 0;
inew->p.prgrph = pg;

/* Now scan for the text items within this item, and arrange for them to be
chained together. Update the processing flags in each textblock, and also
update the current flag set if we pass a processing instruction. */

for (ii = i; ii != i->partner; ii = ii->next)
  {
  if (Ustrcmp(ii->name, "#PCDATA") == 0)
    {
    ii->p.txtblk->pin_flags = (ii->p.txtblk->pin_flags &
      (PIN_FIXED|PIN_SSPERCENT)) | pin_flags;
    *tbptr = ii->p.txtblk;
    tbptr = &(ii->p.txtblk->next);
    }

  /* Skip index items (which may contain their own #PCDATA blocks) */

  else if (Ustrcmp(ii->name, "indexterm") == 0) ii = ii->partner;

  /* Handle processing flag changes and dynamic layparm changes - though some
  of the latter won't take effect till the next paragraph. */

  else if (Ustrcmp(ii->name, "?sdop") == 0)
    {
    pin_flags = pin_change_flags(ii, pin_flags);
    pin_dynamic_layparm(ii);
    }

  /* Handle footnotes by calling para_identify() to deal with the paragraphs of
  the footnote. */

  else if (Ustrcmp(ii->name, "footnote") == 0)
    {
    para_identify(ii->next, FONTS_FOOTNOTE, ii->partner);
    ii = ii->partner;
    }

  /* Handle <[inline]mediaobject> by ending the current paragraph before it,
  and starting a new paragraph after it. */

  else if (Ustrcmp(ii->name, "mediaobject") == 0 ||
           Ustrcmp(ii->name, "inlinemediaobject") == 0)
    {
    item *opartner = i->partner;

    inew = misc_malloc(sizeof(item));
    Ustrcpy(inew->name, "/");
    inew->p.param = NULL;
    inew->partner = i;
    inew->linenumber = ii->linenumber;
    inew->flags = 0;
    i->partner = inew;
    misc_insert_item(inew, ii);

    inew = misc_malloc(sizeof(item));
    Ustrcpy(inew->name, "para");
    inew->p.param = NULL;
    inew->partner = opartner;
    opartner->partner = inew;
    inew->linenumber = ii->partner->linenumber;
    inew->flags = 0;
    misc_insert_item(inew, opartner);

    break;
    }
  }

/* If the paragraph consists just of a single &#x200b; character (a zero-width
space), with optional white space following, remove the text itself so no
(invisible) line is actually printed. This has the effect of inserting a bit of
vertical white space, but less than if a blank line (e.g. a hard space) is
printed. */

if (pg->intxtblk != NULL &&
    pg->intxtblk->next == NULL)   /* Just one text block */
  {
  textblock *tb = pg->intxtblk;
  if (tb->length > 0)
    {
    int c;
    uschar *ptr = tb->string;
    GETCHARINC(c, ptr);
    if (c == ZERO_SPACE)
      {
      while (isspace(*ptr)) ptr++;
      if (*ptr == 0) pg->intxtblk = NULL;
      }
    }
  }

return i->partner;
}



/*************************************************
*                Deal with a title               *
*************************************************/

/* A title ends up as a single paragraph, of course. This function is called
when a title-capable item is encountered. As we search for the title, we keep
track of potential changes to pin_flags, but only instantiate them if we
actually do process a title. A title may or may not be followed by a subtitle
in some circumstances.

Arguments:
  i            the item
  lp           layout parameters for main title when no subtitle
  lp2          layout parameters for main title when there is a subtitle
  lps          if not NULL, layout parameters for subtitle
               if NULL, do not seek a subtitle

Returns:       the title (or subtitle) partner, if a title is found
               the original item if not
*/

static item *
para_title(item *i, layoutparam *lp, layoutparam *lp2, layoutparam *lps)
{
item *ii;
unsigned int potential_pin_flags = pin_flags;

/* Search forward to see if the item does in fact have a title. Ignore orphan
newline data blocks, which can arise from newlines between elements. */

for (ii = i->next; ii != NULL; ii = ii->next)
  {
  if (ISSECT(ii->name) ||
        (Ustrcmp(ii->name, "#PCDATA") == 0 &&
         (ii->p.txtblk->length != 1 || ii->p.txtblk->string[0] != '\n')) ||
      Ustrcmp(ii->name, "varlistentry") == 0 ||
      ii->partner == i) return i;                /* No title is present */
  if (Ustrcmp(ii->name, "title") == 0) break;
  if (Ustrcmp(ii->name, "?sdop") == 0)
    potential_pin_flags = pin_change_flags(ii, potential_pin_flags);
  }

/* We should not have ii == NULL here, but allow for it, just in case. */

if (ii == NULL) return i;
pin_flags = potential_pin_flags;
i = create_para(ii, lp, 0);

/* If we have a subtitle layout parameter, search for a subtitle */

if (lps == NULL) return i;

for (ii = i->next; ii != NULL; ii = ii->next)
  {
  if (ISSECT(ii->name) ||
        (Ustrcmp(ii->name, "#PCDATA") == 0 &&
         (ii->p.txtblk->length != 1 || ii->p.txtblk->string[0] != '\n')) ||
      Ustrcmp(ii->name, "varlistentry") == 0 ||
      ii->partner == i) return i;                /* No title is present */
  if (Ustrcmp(ii->name, "subtitle") == 0) break;
  if (Ustrcmp(ii->name, "?sdop") == 0)
    potential_pin_flags = pin_change_flags(ii, potential_pin_flags);
  }

if (ii == NULL) return i;

/* Adjust the layout parameters for the main title in the light of there being
a subtitle before creating the subtitle and returning. */

i->partner->next->p.prgrph->layparm = lp2;
pin_flags = potential_pin_flags;
return create_para(ii, lps, 0);
}



/*************************************************
*              Identify paragraphs               *
*************************************************/

/* This function scans an item list, looking for the starts and ends of
generalized "paragraphs", which can be normal text paragraphs, or collections
of lines of other kinds. At the start of each paragraph, a #PCPARA item is
inserted into the item list, pointing to a paragraph block. This points to the
first textblock of the paragraph; the remaining text blocks are then chained
on.

During the scan, we take note of processing instructions that affect the way
paragraphs are handled and set the relevant flags in the textblocks. This
affects kerning and hyphenation.

This function stops at the end of the list or when it hits an <index> item,
unless it starts at an <index> item, in which case it treats that like a
<chapter>. This is the case when it is called after creating the index.

Argument:
  item_list   the start of the item list to be processed
  fonts       FONTS_MAIN when processing the main text
              FONTS_TOC when processing the TOC
              FONTS_INDEX when processing the index
              FONTS_FOOTNOTE when processing a footnote
              FONTS_HEADFOOT when processing head/foot lines
  stop_at     where to stop (non-NULL when pulling out footnotes)

Returns:      TRUE to continue processing
*/

BOOL
para_identify(item *item_list, int fonts, item *stop_at)
{
item *saved_attribution = NULL;
layoutparam *lpptr;
layoutparam *temp_lpptr = NULL;
layoutparam *lpstack[MAXLISTNEST];
int list_nest_depth = 0;
int section_nest_depth = 0;
int term_count = 0;
int mainorfn;
int fnindent;

switch(fonts)
  {
  case FONTS_INDEX:
  lpptr = &ixpara_layparm;
  mainorfn = LP_MAIN;         /* Though won't ever be used in an index */
  fnindent = 0;
  DEBUG(D_any) debug_printf("Identifying paragraphs\n");
  break;

  case FONTS_FOOTNOTE:
  lpptr = &footnote_para_layparm;
  mainorfn = LP_FOOTNOTE;
  fnindent = footnote_indent;
  DEBUG(D_any) debug_printf("Identifying paragraphs in a footnote\n");
  break;

  /* This is used both for main text and for head/foot lines */

  default:
  lpptr = &para_layparm;
  mainorfn = LP_MAIN;
  fnindent = 0;
  DEBUG(D_any) debug_printf("Identifying paragraphs\n");
  break;
  }

pin_flags = pin_init_flags();
page_columns = page_columns_init;
page_colsep = page_colsep_init;
extra_leading = 0;

/* Process an initial <index> item */

if (Ustrcmp(item_list->name, "index") == 0)
  {
  stop_at = item_list->partner;
  read_linenumber = item_list->linenumber;
  item_list = para_title(item_list, &chapter_layparm, &chapter2_layparm,
    &chapsubt_layparm);
  }

/* Now the rest of the list */

for (item *i = item_list; i != NULL; i = i->next)
  {
  uschar *name = i->name;

  if (Ustrcmp(i->name, "#FILENAME") == 0)
    {
    read_filename = i->p.string;
    continue;
    }

  read_linenumber = i->linenumber;

  /* Stop when we hit the start of an index (when processing main text) or the
  end of an index (when processing an index). */

  if (i == stop_at || Ustrcmp(name, "index") == 0) break;

  /* Titles for <book> items are handled elsewhere. Skip them here. */

  if (Ustrcmp(name, "book") == 0)
    {
    for (i = i->next; i != NULL; i = i->next)
      {
      if (i->name[0] != '?' && i->name[0] != '#' && i->name[0] != '/' &&
          Ustrcmp(i->name, "title") != 0 &&
          Ustrcmp(i->name, "subtitle") != 0 &&
          Ustrcmp(i->name, "titleabbrev") != 0)
        break;
      }
    name = i->name;
    }

  /* Certain elements that don't contribute to the normal text on the
  page are just skipped. */

  if (Ustrcmp(name, "articleinfo") == 0 ||
      Ustrcmp(name, "audiodata")   == 0 ||
      Ustrcmp(name, "audioobject") == 0 ||
      Ustrcmp(name, "bookinfo")    == 0 ||
      Ustrcmp(name, "indexterm")   == 0 ||
      Ustrcmp(name, "titleabbrev") == 0 ||
      Ustrcmp(name, "videodata")   == 0 ||
      Ustrcmp(name, "videoobject") == 0)
    i = i->partner;

  /* Handle chapterish titles */

  else if (Ustrcmp(name, "chapter")  == 0 ||
           Ustrcmp(name, "preface")  == 0 ||
           Ustrcmp(name, "colophon") == 0 ||
           Ustrcmp(name, "article")  == 0 ||
             (Ustrcmp(name, "appendix") == 0 &&
              document_type != DOC_ARTICLE))
    i = para_title(i, &chapter_layparm, &chapter2_layparm, &chapsubt_layparm);

  /* Handle section titles (includes appendix in an article) */

  else if (ISSECT(name) || Ustrcmp(name, "appendix") == 0)
    {
    switch (section_nest_depth++)
      {
      case 0:  i = para_title(i, &section_layparm, NULL, NULL); break;
      default: i = para_title(i, &subsection_layparm, NULL, NULL); break;
      }
    }

  /* Handle formal paragraph titles */

  else if (Ustrcmp(name, "formalpara") == 0)
    i = para_title(i, lptable[mainorfn + LP_FORMALPARA], NULL, NULL);

  /* Handle figure titles */

  else if (Ustrcmp(name, "figure") == 0)
    i = para_title(i, lptable[mainorfn + LP_FIGURETITLE], NULL, NULL);

  /* Handle table titles */

  else if (Ustrcmp(name, "table") == 0)
    i = para_title(i, lptable[mainorfn + LP_TABLETITLE], NULL, NULL);

  /* Handle example titles */

  else if (Ustrcmp(name, "example") == 0)
    i = para_title(i, lptable[mainorfn + LP_EXAMPLETITLE], NULL, NULL);

  /* Block quotes  */

  else if (Ustrcmp(name, "blockquote") == 0)
    {
    item *bq = i;
    i = para_title(i, lptable[mainorfn + LP_BLOCKQUOTE], NULL, NULL);
    if (i != bq)    /* There was a title */
      {
      item *ii;
      for (ii = bq->next; ii != i; ii = ii->next)
        {
        if (Ustrcmp(ii->name, "#PCPARA") == 0)
          {
          (ii->p.prgrph)->justify = blockquote_title_justify;
          break;
          }
        }
      }
    if ((saved_attribution = find_attribution(bq)) != NULL)
      i = saved_attribution->partner;
    if (list_nest_depth >= MAXLISTNEST) error(114);  /* Fatal */
    lpstack[list_nest_depth++] = lpptr;
    lpptr = lptable[mainorfn + LP_BLOCKQUOTE];
    }

  /* A <note> is like a block quote, but without an attribution  */

  else if (Ustrcmp(name, "note") == 0)
    {
    item *note = i;
    i = para_title(i, lptable[mainorfn + LP_NOTE], NULL, NULL);
    if (i != note)    /* There was a title */
      {
      item *ii;
      for (ii = note->next; ii != i; ii = ii->next)
        {
        if (Ustrcmp(ii->name, "#PCPARA") == 0)
          {
          (ii->p.prgrph)->justify = note_title_justify;
          break;
          }
        }
      }
    if (list_nest_depth >= MAXLISTNEST) error(114);  /* Fatal */
    lpstack[list_nest_depth++] = lpptr;
    lpptr = lptable[mainorfn + LP_NOTE];
    }

  /* A <sidebar> is like a block quote, but without an attribution  */

  else if (Ustrcmp(name, "sidebar") == 0)
    {
    item *sidebar = i;
    i = para_title(i, lptable[mainorfn + LP_SIDEBAR], NULL, NULL);
    if (i != sidebar)    /* There was a title */
      {
      item *ii;
      for (ii = sidebar->next; ii != i; ii = ii->next)
        {
        if (Ustrcmp(ii->name, "#PCPARA") == 0)
          {
          (ii->p.prgrph)->justify = sidebar_title_justify;
          break;
          }
        }
      }
    if (list_nest_depth >= MAXLISTNEST) error(114);  /* Fatal */
    lpstack[list_nest_depth++] = lpptr;
    lpptr = lptable[mainorfn + LP_SIDEBAR];
    }

  /* An epigraph is like a block quote, but without a title */

  else if (Ustrcmp(name, "epigraph") == 0)
    {
    if ((saved_attribution = find_attribution(i)) != NULL)
      i = saved_attribution->partner;
    if (list_nest_depth >= MAXLISTNEST) error(114);  /* Fatal */
    lpstack[list_nest_depth++] = lpptr;
    lpptr = lptable[mainorfn + LP_BLOCKQUOTE];
    }

  /* While we are in list of some sort, use a different default layout.
  Need to keep track of nesting. */

  else if (Ustrcmp(name, "itemizedlist") == 0)
    {
    if (list_nest_depth >= MAXLISTNEST) error(114);  /* Fatal */
    lpstack[list_nest_depth++] = lpptr;
    lpptr = lptable[mainorfn + LP_ILISTPARA];
    }

  else if (Ustrcmp(name, "orderedlist") == 0)
    {
    if (list_nest_depth >= MAXLISTNEST) error(114);  /* Fatal */
    lpstack[list_nest_depth++] = lpptr;
    lpptr = lptable[mainorfn + LP_OLISTPARA];
    }

  else if (Ustrcmp(name, "variablelist") == 0)
    {
    i = para_title(i, &vlisttitle_layparm, NULL, NULL);
    if (list_nest_depth >= MAXLISTNEST) error(114);  /* Fatal */
    lpstack[list_nest_depth++] = lpptr;
    lpptr = lptable[mainorfn + LP_VLISTPARA];
    }

  else if (Ustrcmp(name, "varlistentry") == 0)
    {
    term_count = 0;
    }

  /* We need to distinguish between a single occurence of <term> and the first,
  middle, and last occurrences if there is a sequence of <term>s. This is to
  allow for less than normal paragraph vertical space between them.

  When we have processed the last <term>, set temporary layout parameters for
  the first following paragraph, to reduce the space above it. */

  else if (Ustrcmp(name, "term") == 0)
    {
    BOOL term_follows = Ustrcmp(i->partner->next->name, "term") == 0;
    i = create_para(i, lptable[mainorfn +
      ((term_count++ == 0)?
      (term_follows? LP_TERMFIRST : LP_TERM) :
      (term_follows? LP_TERMMID : LP_TERMLAST)
      )], fnindent);
    if (!term_follows) temp_lpptr = lptable[mainorfn + LP_VLISTPARA1];
    }

  /* Unstack environment at the end of texty things. */

  else if (Ustrcmp(name, "/") == 0)
    {
    if (Ustrcmp(i->partner->name, "itemizedlist") == 0 ||
        Ustrcmp(i->partner->name, "orderedlist")  == 0 ||
        Ustrcmp(i->partner->name, "variablelist") == 0)
      lpptr = lpstack[--list_nest_depth];

    /* At the end of a block quote or epigraph, if there was an attribution,
    move it to the end of the quote and create an appropriate paragraph,
    forcing right-justification. Add rules for blockquote if wanted. */

    else if (Ustrcmp(i->partner->name, "blockquote") == 0 ||
        Ustrcmp(i->partner->name, "epigraph") == 0)
      {
      item *orig = i->partner;
      if (saved_attribution != NULL)
        {
        saved_attribution->prev->next = saved_attribution->partner->next;
        saved_attribution->partner->next->prev = saved_attribution->prev;
        i->next->prev = saved_attribution->partner;
        saved_attribution->partner->next = i->next;
        saved_attribution->prev = i;
        i->next = saved_attribution;
        i = create_para(saved_attribution, lpptr, fnindent);
        saved_attribution->next->p.prgrph->justify = J_RIGHT;
        }
      if (orig->name[0] == 'b' && blockquote_ruled) add_rules(orig, i);
      lpptr = lpstack[--list_nest_depth];
      }

    /* At the end of a <note>, add rules if wanted. */

    else if (Ustrcmp(i->partner->name, "note") == 0)
      {
      if (note_ruled) add_rules(i->partner, i);
      lpptr = lpstack[--list_nest_depth];
      }

    /* At the end of a figure, sort out its title width and justification. */

    else if (Ustrcmp(i->partner->name, "figure") == 0)
      set_figure_title_width(i->partner);

    /* At the end of an example, sort out its title width and justification. */

    else if (Ustrcmp(i->partner->name, "example") == 0)
      set_example_title_width(i->partner);

    /* At the end of a section, decrement the nesting depth */

    else if (ISSECT(i->partner->name)) section_nest_depth--;
    }

  /* Paragraphs are otherwise delimited by the following elements: entry,
  literallayout, para, programlisting, screen, simpara, term. For literal
  paragraphs, apply the leading space fudge if enabled. */

  else if (Ustrcmp(name, "literallayout") == 0 ||
           Ustrcmp(name, "programlisting") == 0 ||
           Ustrcmp(name, "screen") == 0)
    {
    item *ii = create_para(i, lptable[mainorfn + LP_LITERALPARA], fnindent);

    /* If enabled, scan the lines of the paragraph to find the maximum number
    of leading spaces that they all have. Then remove this number of spaces.
    This fudge is so that sources compatible with other DocBook processors can
    be handled unchanged. The problem arises when the literal paragraph is
    within an otherwise indented block such as an orderered list. This is
    handled correctly by sdop, which takes note of the outer indent, but other
    processors have been seen to ignore the indent, thus leaving the literal
    block too far left. Consequently, people often put leading spaces in the
    source to compensate. */

    if (literal_indent_fudge)
      {
      textblock *t;
      paragraph *p = i->next->p.prgrph;
      int count = 1000;
      BOOL lastwasnl = TRUE;

      for (t = p->intxtblk; t != NULL; t = t->next)
        {
        uschar *s = t->string;

        while (*s != 0)
          {
          if (lastwasnl)
            {
            int c = 0;
            while (*s == ' ')
              {
              s++;
              c++;
              }
            if (c < count) count = c;
            }
          while (*s != 0 && *s != '\n') s++;
          lastwasnl = *s == '\n';
          if (lastwasnl) s++;
          }
        }

      if (count > 0)
        {
        lastwasnl = TRUE;
        for (t = p->intxtblk; t != NULL; t = t->next)
          {
          uschar *s = t->string;
          while (*s != 0)
            {
            if (lastwasnl)
              {
              t->length -= count;
              memmove(s, s + count, t->length - (s - t->string) + 1);
              }
            while (*s != 0 && *s != '\n') s++;
            lastwasnl = *s == '\n';
            if (lastwasnl) s++;
            }
          }
        }
      }

    i = ii;
    }

  /* <entry> and <caption> are simple cases */

  else if (Ustrcmp(name, "entry")   == 0 ||
           Ustrcmp(name, "caption") == 0)
    i = create_para(i, lpptr, fnindent);

  /* If a paragraph contains nothing but <indexterm>s, ignore it. */

  else if (Ustrcmp(name, "para") == 0 ||
           Ustrcmp(name, "simpara") == 0)
    {
    item *ii;
    for (ii = i->next; ii != i->partner; ii = ii->next)
      {
      if (Ustrcmp(ii->name, "indexterm") != 0) break;
      ii = ii->partner;
      }
    if (ii != i->partner)
      {
      i = create_para(i, (temp_lpptr != NULL)? temp_lpptr : lpptr, fnindent);
      temp_lpptr = NULL;
      }
    else i = i->partner;
    }

  /* We may encounter processing instructions between paragraphs, and
  these may change the way subsequent text is processed. */

  else if (Ustrcmp(name, "?sdop") == 0)
    {
    pin_flags = pin_change_flags(i, pin_flags);
    pin_change_columns(i);
    pin_figex_layout_changes(i);
    pin_dynamic_layparm(i);
    }

  /* Wrap orphan text that isn't inside a relevant element in <para> so that it
  isn't lost, though the output may well be odd. However, a text item that is
  just a newline can arise from linebreaks between elements; remove such items
  from the chain (that is, ignore them).

  Since we don't have access to the anchor of the current list, we handle
  non-empty orphan text by inserting afterwards and then swapping the elements'
  values (preserving the chain pointers), just in case this is the very first
  element. Then generate an error, but because there may be several chunks on
  the same input line, avoid repeating the error for the same line. */

  else if (Ustrcmp(name, "#PCDATA") == 0)
    {
    if (i->p.txtblk->length == 1 && i->p.txtblk->string[0] == '\n')
      {
      i->prev->next = i->next;
      if (i->next != NULL) i->next->prev = i->prev;
      i = i->prev;
      }
    else
      {
      item *p = misc_insert_element_pair(US"para", i);
      item temp = *i;

      *i = *p;
      i->prev = temp.prev;
      i->next = temp.next;
      temp.prev = p->prev;
      temp.next = p->next;
      *p = temp;
      p->next->partner = i;

      i = create_para(i, (fonts == FONTS_INDEX)? &ixpara_layparm :
        lptable[mainorfn + LP_PARA], fnindent);

      if (read_linenumber != error53_linenumber)
        {
        error(53);
        error53_linenumber = read_linenumber;
        }
      }
    }
  }

read_linenumber = 0;
DEBUG(D_ipara) debug_print_item_list(item_list, "after para_identify()");
return TRUE;
}



/*************************************************
*           Insert a rule "line"                 *
*************************************************/

/* This function is called to insert a dummy line that represents a horizontal
rule.

Arguments:
  olanchor      where to attach the line
  indent        indent for the rule
  width         width of rule

Returns:        the new anchor point
*/

static outputline **
add_rule(outputline **olanchor, int indent, int width)
{
outputline *ol = misc_malloc(sizeof(outputline));
ol->next = NULL;
*olanchor = ol;
ol->indent = indent;
ol->width = width;
ol->txtblk = NULL;
ol->flags = OLF_RULE;
ol->fnstr = NULL;
ol->stretch = ol->swidth = ol->scount = 0;
ol->depth = 12000;
return &(ol->next);
}



/*************************************************
*         Search for an automatic hyphen         *
*************************************************/

/* This function is called when a line that is being filled would be above a
certain looseness threshold if broken at the last whitespace break point, and
there is the possibility of hyphenating the following word.

Arguments:
  tb             the textblock
  ah_p           points to the start of the word
  ah_lw          points to the linewidth just before the word
  offset         offset to the maximum part-word that fits (always > 0)
  maxlinewidth   as it says

Returns:         TRUE if a hyphenation has been found
                 the values pointed to by ah_p and ah_lw are updated
*/

static BOOL
try_hyphen(textblock *tb, uschar **ah_p, int *ah_lw, int offset,
  int maxlinewidth)
{
uschar *hp = *ah_p;
int hlw = *ah_lw;
int ho = 0;
int startoffset;
int hyphenwidth;
int nbhcount = 0;
int nbh[128];
uschar buffer[128];
uschar singular[128];
uschar *bp = buffer;
uschar *bend = buffer + sizeof(buffer);

/* Get the next word by seeking forward from the start. Keep a list of "no
break here" offsets in the copied word. */

while (*hp != 0 && !isspace(*hp) && bp < bend)
  {
  int c;
  GETCHARINC(c, hp);
  if (c == NO_BREAK_HERE) nbh[nbhcount++] = bp - buffer;
    else bp += misc_ord2utf8(c, bp);
  }
if (bp >= bend) return FALSE;
*bp = 0;
nbh[nbhcount] = 1000; /* Bigger than any possible offset */

/* We have a word, but if it is the last word in a paragraph, we do not
want to try to hyphenate it. */

DEBUG(D_hyphen) debug_printf("maybe hyphenate '%s' offset=%d\n", buffer,
  offset);

if (tb->next == NULL)
  {
  while (isspace(*hp)) hp++;
  if (*hp == 0)
    {
    DEBUG(D_hyphen) debug_printf("no hyphenation for last word in paragraph\n");
    return FALSE;
    }
  }

/* Hyphen_Prepare() removes punctuation from either end, and also the suffix
's, and yields the number of bytes removed from the start. A negative return
means there were no letters in the word. */

startoffset = Hyphen_Prepare(buffer);
if (startoffset < 0)
  {
  DEBUG(D_hyphen) debug_printf("no letters found\n");
  return FALSE;
  }

/* Hyphen_DePlural() de-plurals a word using a heuristic that works for most
English words. */

(void)Hyphen_DePlural(buffer + startoffset, singular);

DEBUG(D_hyphen) debug_printf("startoffset=%d prepared word is '%s'\n",
  startoffset, singular);

/* Now seek hyphenation points, in order from right to left. */

hyphenwidth = font_charwidth('-', tb->vfont, NULL);

for (;;)
  {
  int k;
  int nbhskipped = 0;

  if ((ho = Hyphen_Next(singular, ho)) <= 0) return FALSE;
  DEBUG(D_hyphen) debug_printf("hyphen offset %d\n", ho);
  if (ho + startoffset > offset) continue;

  for (k = 0;; k++)
    {
    if (ho < nbh[k]) break;
    if (ho == nbh[k])
      {
      DEBUG(D_hyphen) debug_printf("rejected by 'no break here'\n");
      break;
      }
    nbhskipped += 2;  /* Allow for NBH in original word */
    }
  if (ho == nbh[k]) continue;
  hlw = *ah_lw + hyphenwidth;
  for (hp = *ah_p; hp < *ah_p + startoffset + ho + nbhskipped; )
    {
    int c;
    GETCHARINC(c, hp);
    hlw += font_charwidth(c, tb->vfont, NULL);
    }
  if (hlw <= maxlinewidth)
    {
    ho += nbhskipped;
    break;
    }
  DEBUG(D_hyphen) debug_printf("too wide: hlw=%d max=%d\n", hlw, maxlinewidth);
  }

DEBUG(D_hyphen) debug_printf("hyphen found: ho=%d\n", ho);

*ah_p = *ah_p + startoffset + ho;
*ah_lw = hlw;
return TRUE;
}



/*************************************************
*      Pick off the next line in a paragraph     *
*************************************************/

/* This function scans the text in a paragraph, adding up the character widths,
until it has a complete line. The current position in the paragraph is
indicated by a triple: textblock (for font info), textstr (current string
fragment) and offset (in the string). The last two are updated; the new
textblock pointer is returned as the yield of the function.

In filling mode, newlines are turned into spaces, and the line is ended when it
is full. In non-filling mode, the line ends at a newline character, but an
error is given if it is longer than the maximum length.

Arguments:
  tb              points to first textblock
  offsetptr       points to byte offset in textblock's string
  maxlinewidth    maximum line width
  ol              output line block
  fill            TRUE if filling
  stretch         TRUE if line to be stretched

Returns:          pointer to remaining textblock(s)
*/

static textblock *
para_get_next_line(textblock *tb, int *offsetptr, int maxlinewidth,
  outputline *ol, BOOL fill, BOOL stretch)
{
BOOL lastwasspace = FALSE;
BOOL firstcharinline = TRUE;
int maxlinedepth = 0;
int linewidth = 0;
int spacewidth = 0;
int spacecount = 0;
int lastc = -1;
int startoffset = *offsetptr;
int chtype;
BOOL kerning, hyphening, fiok, stdencoding;
BOOL okhyphen = TRUE;
uschar *p;
textblock *ctb;
textblock *last_tb = NULL;
textblock **ntbanchor = &(ol->txtblk);
footnotestr **fnsanchor = &(ol->fnstr);

/* Data for last break point at white space when filling. */

int lbp_offset = 0;
int lbp_linewidth = 0;
int lbp_spacewidth = 0;
int lbp_spacecount = 0;
textblock *lbp_tb = NULL;

/* Data for the last hyphen point when filling. This isn't the same as the last
break point, because we won't hyphenate unless the line is too loose. */

int lhp_offset = 0;
int lhp_linewidth = 0;
int lhp_spacewidth = 0;
int lhp_spacecount = 0;
textblock *lhp_tb = NULL;

/* Data for potential start of an automatically hyphenated word. */

int h_lw = 0;
textblock *h_tb = NULL;
uschar *h_p = NULL;

/* If the first textblock is a footnote key definition, copy it over and
otherwise ignore it. */

if ((tb->pin_flags & PIN_FNKEYDEF) != 0)
  {
  textblock *ntb = misc_malloc(sizeof(textblock) + tb->length);
  *ntb = *tb;
  *ntbanchor = ntb;
  ntbanchor = &(ntb->next);
  ntb->next = NULL;
  ntb->lastin = tb;
  tb = tb->next;
  if (tb == NULL) error(32);     /* Hard error */
  }

/* Initialize things from the first proper text block */

kerning = (tb->pin_flags & PIN_KERN) != 0;
hyphening = (tb->pin_flags & PIN_HYPH) != 0;
fiok = tb->vfont->afont->hasfi && !tb->vfont->afont->fixedpitch;
stdencoding = tb->vfont->afont->stdencoding;
p = tb->string + startoffset;
ctb = tb;    /* Save for later copying */

/* Initialize the line depth from the first font. */

maxlinedepth = tb->vfont->size + tb->vfont->leading;

/* If we are filling, white space at the start of a line is ignored. This can
occur at the start of a paragraph when spaces follow <para> before the text
starts. White space at the end of a line is skipped over below, in order to
detect the end of a paragraph at that point, so an internal line shouldn't
start with space. */

if (fill) for (;;)
  {
  while (*p == ' ' || *p == '\n')
    {
    p++;
    startoffset++;
    }
  if (*p != 0) break;                   /* More input in this block */
  if (tb->next == NULL) break;          /* No more data */
  ctb = tb = tb->next;                  /* Next block */
  p = tb->string;
  startoffset = 0;
  }

/* Scan the line till we hit the end. If we do hit the end, make a text block,
join it on, copy the line's data, and return.

If we don't hit the end, set up the text block etc, remember the width (if
necessary), and loop to make another text block. */

for (;;)
  {
  int c, cw, k, w;
  BOOL splitspace;

  /* Hit end of input text without ending the line. Move on to the next text
  block, or end the paragraph if there isn't one. This case should occur only
  when filling because the last character of a paragraph will always be a
  newline. */

  while (*p == 0)
    {
    int newlinedepth;
    tb = tb->next;

    /* If we run out of text and are in filling mode, and the last character
    was whitespace, adjust the space data back to the last break point, because
    all trailing whitespace is removed below. */

    if (tb == NULL)          /* End of paragraph */
      {
      if (lastwasspace && fill)
        {
        linewidth = lbp_linewidth;
        spacewidth = lbp_spacewidth;
        spacecount = lbp_spacecount;
        }
      goto END_LINE;
      }

    /* Set up for carrying on with the next text block */

    p = tb->string;          /* Start on new input text string */
    h_tb = NULL;             /* Forget auto-hyphen point */
    lastc = -1;              /* Can't kern with previous */
    kerning = (tb->pin_flags & PIN_KERN) != 0;
    hyphening = (tb->pin_flags & PIN_HYPH) != 0;
    fiok = tb->vfont->afont->hasfi && !tb->vfont->afont->fixedpitch;
    stdencoding = tb->vfont->afont->stdencoding;
    newlinedepth = tb->vfont->size + tb->vfont->leading;
    if (newlinedepth > maxlinedepth) maxlinedepth = newlinedepth;
    }

  /* Get the next character. Certain characters are handled specially, provided
  that the font is standardly encoded. Some Unicode characters do not exist in
  PostScript fonts but may be substituted. Arrange to use the appropriate
  widths, and remember the different kinds of space. */

  GETCHARINC(c, p);
  splitspace = FALSE;

  if (stdencoding) switch (c)
    {
    case '\t':                     /* Treat tab as space */
    c = ' ';
    /* Fall through */

    case ' ':                      /* Normal splitting space */
    cw = ' ';
    splitspace = TRUE;
    break;

    case '\n':                     /* When filling, newline is a space */
    cw = ' ';
    splitspace = TRUE;
    break;

    case 'f':                      /* Handle fi ligature */
    if (*p == 'i' && fiok)
      {
      cw = c = CHAR_FI;
      p++;
      }
    else cw = c;
    break;

    case SOFT_HYPHEN:              /* Take width of normal hyphen */
    cw = '-';
    break;

    case HARD_SPACE:               /* Take width of space */
    cw = ' ';
    break;

    case ZERO_SPACE:               /* Width will be zero */
    case BREAK_PERMIT:
    cw = c;
    splitspace = TRUE;
    break;

    case NO_BREAK_HERE:            /* Width will be zero */
    cw = c;
    break;

    default:
    cw = c;
    break;
    }
  else cw = c;                     /* Not standard encoding; don't interpret */

  /* If no width can be found, it means that neither the current font nor one
  of the auxiliary fonts has this character. */

  w = font_charwidth(cw, tb->vfont, &chtype);

  if (chtype == CHTYPE_UNKNOWN)
    {
    uschar utf[8];
    tree_node *t;
    utf[misc_ord2utf8(c, utf)] = 0;

    /* Warn once for each unknown character */

    t = tree_search(unknown_char_tree, utf);
    if (t == NULL)
      {
      tree_node *tt = misc_malloc(sizeof(tree_node) + 6);
      Ustrcpy(tt->name, utf);
      (void)tree_insertnode(&(unknown_char_tree), tt);
      if (warn_unsupported_chars)
        (void)error(30, c, tb->vfont->afont->name);    /* Warning */
      }

    /* Replace with the substitute character, which should always be available
    in standardly encode fonts. */

    if (stdencoding)
      {
      w = font_charwidth(UNKNOWN_CHAR, tb->vfont, &chtype);
      if (chtype == CHTYPE_UNKNOWN) (void)error(29);   /* Hard */
      c = cw = UNKNOWN_CHAR;
      }

    /* For a non-standard encoding, use the first available character. */

    else
      {
      int i;
      for (i = 0; i < 256; i++)
        {
        w = font_charwidth(i, tb->vfont, &chtype);
        if (chtype != CHTYPE_UNKNOWN) break;
        }
      if (i == 256)
        {
        (void)error(104, tb->vfont->afont->name, c);
        continue;
        }
      else
        {
        if (t == NULL) (void)error(103, i, c, tb->vfont->afont->name);
        c = cw = i;
        }
      }
    }

  /* If this is not whitespace, and not a character from an auxiliary font,
  check for kerning. If we are dealing with a soft hyphen, we'll calculate
  the kerning as if for a hyphen, but if the soft hyphen is accepted, the width
  and kerning is reset to zero below. We don't change lastc so that in the case
  of an accepted soft hyphen (which is not printed), the kerning between
  surrounding characters works. */

  if (stdencoding && kerning && !splitspace && c != HARD_SPACE &&
      chtype == CHTYPE_STD)
    {
    k = font_kernwidth(lastc, cw, tb->vfont);
    if (c != SOFT_HYPHEN) lastc = cw;
    }
  else
    {
    k = 0;
    lastc = -1;
    }

  /* If not filling, the line end is indicated by newline.*/

  if (!fill)
    {
    if (c == '\n') break;
    }

  /* Otherwise, the line end is computed by summing character widths. If adding
  this character makes the line too long, go back to the last break point, or
  mess around with hyphenation. If we cannot find anywhere to break, just carry
  on. We'll break at a future break point, if any, but will end up with an
  overlong line, which will be diagnosed. The precise logic is like this:

  (0) If this character is a break point (fortuitously), break here.

  (1) If a previous break point exists, and the resulting line is not too
      loose, take that break point. This avoids over-zealous hyphenation.

  (2) If there's an explicit hyphenation point (necessarily after any break
      point), take it.

  (3) If hyphenation is permitted, try hyphenating the last word.

  (4) If there is a break point, take it. This will give an overly loose line,
      but it's the best we can do.

  (5) We can't split the line; carry on with an overlong line. This will be
      diagnosed in due course.
  */

  else if (linewidth + w + k > maxlinewidth)
    {
    int looseness = -1;

    /* Check for reaching a break point exactly at the end of a line. This also
    applies when a space is encountered after the overflow of an unbreakable,
    overlong line. */

    if (splitspace)
      {
      DEBUG(D_fill) debug_printf("line overflowed at space\n");
      for (;;)
        {
        int cp;
        uschar *pp = p - 1;
        BACKCHAR(pp);
        GETCHAR(cp, pp);
        if (cp != ' ' && cp != '\n' && cp != ZERO_SPACE && cp != BREAK_PERMIT)
          break;
        p = pp;
        }
      break;
      }

    /* Accept a previous break point if the line isn't too loose */

    if (lbp_tb != NULL && lbp_spacewidth > 0)
      {
      looseness = ((maxlinewidth - lbp_linewidth)*100)/lbp_spacewidth;
      if (looseness < L_BAD)
        {
        tb = lbp_tb;
        p = tb->string + lbp_offset;
        linewidth = lbp_linewidth;
        spacewidth = lbp_spacewidth;
        spacecount = lbp_spacecount;
        DEBUG(D_fill) debug_printf("fill break looseness OK: %d\n", looseness);
        break;
        }
      }
    DEBUG(D_fill) debug_printf("fill break looseness not OK: %d\n", looseness);

    /* Break at explicit hyphen if there is one (hard or soft). This will have
    been set up only if hyphenation is permitted. */

    if (lhp_tb != NULL)
      {
      tb = lhp_tb;
      p = tb->string + lhp_offset;
      linewidth = lhp_linewidth;
      spacewidth = lhp_spacewidth;
      spacecount = lhp_spacecount;
      ol->flags |= OLF_HYPHENATED;
      DEBUG(D_fill) debug_printf("fill break at %s hyphen\n",
        (p[-1] == '-')? "hard" : "soft");
      break;
      }

    /* Try to hyphenate if permitted and a dictionary is available. We also
    need to have a plausible point from which to search for a hyphenable word.
    If there has been previous white space in the line, this will have been
    remembered. Otherwise, if this is the first textblock of the line, we start
    at the beginning. */

    if (okhyphen && hyphening && main_hyphenfile != NULL)
      {
      if (h_tb == NULL && tb == ctb)
        {
        h_tb = tb;
        h_p = tb->string + startoffset;
        h_lw = 0;
        }

      if (h_tb != NULL)    /* We have somewhere to search from */
        {
        uschar *pp = p - 1;
        BACKCHAR(pp);
        if (pp - h_p > 0)
          {
          if (try_hyphen(tb, &h_p, &h_lw, pp - h_p, maxlinewidth))
            {
            p = h_p;
            linewidth = h_lw;
            ol->flags |= (OLF_HYPHENATED | OLF_ADD_HYPHEN);
            break;
            }
          }
        }
      }

    /* Accept any breakpoint, whatever the looseness */

    if (lbp_tb != NULL)
      {
      tb = lbp_tb;
      p = tb->string + lbp_offset;
      linewidth = lbp_linewidth;
      spacewidth = lbp_spacewidth;
      spacecount = lbp_spacecount;
      break;
      }

    /* If we get here, we have failed to find a way of breaking the line. The
    character will be accepted, and give rise to an overlong line diagnostic in
    due course. Disable automatic hyphenation attempts for subsequent
    characters. We can't just set hyphening FALSE, because it might get reset
    by the next text block. */

    okhyphen = FALSE;
    }

  /* Accept this character for the line. Remember the textblock from which the
  last character came. */

  last_tb = tb;

  /* A hard space does count towards stretching, since it is a "no break"
  space, not a "no stretch" space. */

  if (stdencoding && c == HARD_SPACE)
    {
    spacecount++;
    spacewidth += w;
    }

  /* A newline will be printed as a space. If any space is the first in a
  sequence, remember this as the last break point. In any case, accumulate
  space statistics. */

  else if (splitspace)
    {
    if (!lastwasspace)
      {
      uschar *sp = p - 1;
      BACKCHAR(sp);
      lhp_tb = NULL;                        /* Cancel last hyphen point */
      lbp_tb = tb;                          /* Last break point */
      lbp_offset = sp - tb->string;
      lbp_linewidth = linewidth;            /* Without this char */
      lbp_spacewidth = spacewidth;
      lbp_spacecount = spacecount;
      lastwasspace = TRUE;
      }
    if (c != ZERO_SPACE && c != BREAK_PERMIT)
      {
      spacecount++;                         /* Number of spaces */
      spacewidth += w;                      /* Space does not kern */
      }
    h_tb = NULL;                            /* Kill auto-hyphen start */
    }

  /* Not a space character. If this follows a run of spaces, remember it as
  a possible point to start looking for an automatic hyphen. Otherwise, if it's
  a hyphen character, remember it as a possible manual hyphen point. */

  else
    {
    if (lastwasspace || firstcharinline)
      {
      h_tb = tb;
      h_p = p - 1;
      BACKCHAR(h_p);
      h_lw = linewidth;
      lastwasspace = FALSE;
      }
    else if (okhyphen && hyphening && stdencoding && cw == '-' &&
             linewidth + w + k <= maxlinewidth)
      {
      lhp_tb = tb;                          /* Last hyphen point */
      lhp_offset = p - tb->string;
      lhp_linewidth = linewidth + w + k;
      lhp_spacewidth = spacewidth;
      lhp_spacecount = spacecount;
      }
    if (stdencoding && c == SOFT_HYPHEN) w = k = 0 ; /* No width for no split */
    }

  /* For both the filling and non-filling cases, add this character's kerned
  width into the linewidth. This allows us to diagnose overlong lines in both
  cases. */

  linewidth += w + k;
  firstcharinline = FALSE;
  }

/* Reached the end of the line (possibly the end of the paragraph). Set up
textblocks for everything from the line start to the current point. The
variable ctb was set to the original tb at the start. Remove a final newline,
whether filling or not. Intermediate newlines will be present only when
filling; turn them into spaces. */

END_LINE:

for (; ctb != NULL; ctb = ctb->next)
  {
  int length;
  uschar *nlp;
  textblock *ntb;

  /* If we are to copy a textblock that has the PIN_FNKEYREF flag, it means
  that this line has an attached footnote. Search for the relevant <footnote>
  and add it to the chain of footnotes for this output line. */

  if ((ctb->pin_flags & PIN_FNKEYREF) != 0)
    {
    footnotestr *fns = misc_malloc(sizeof(footnotestr));
    fns->next = NULL;
    *fnsanchor = fns;
    fnsanchor = &(fns->next);

    while (Ustrcmp(fni->name, "footnote") != 0)
      {
      if (fni->next == NULL ||
          Ustrcmp(fni->name, "#PCPARA") == 0)
        error(72);                                  /* Hard */
      fni = fni->next;
      }

    if (Ustrcmp(fni->prev->name, "#PCDATA") != 0 ||
        fni->prev->p.txtblk != ctb)
      error(72);                                    /* Hard */

    fns->footnote = fni;
    fni = fni->partner;                             /* For next in line */
    }

  /* If copying from a textblock that is the one we are going to pass back
  because it contains the start of the next line, copy up to the current point,
  less any final newline. Otherwise, copy from the start to the end of the
  text. If this is the final text of a paragraph, do not include trailing
  whitespace. */

  if (ctb == tb)
    {
    length = p - ctb->string - startoffset;
    if (length > 0 && p[-1] == '\n') length--;
    }
  else
    {
    length = ctb->length - startoffset;
    if (tb == NULL && ctb->next == NULL)
      {
      while (length > 0 && isspace(ctb->string[startoffset + length - 1]))
        length--;
      }
    }

  /* Create a new textblock and add it into the line's chain, provided it
  has some text in it. (It can be empty if it was just a newline that got
  removed above.) */

  if (length > 0)
    {
    ntb = misc_malloc(sizeof(textblock) + length);
    *ntbanchor = ntb;
    ntbanchor = &(ntb->next);
    ntb->next = NULL;
    ntb->lastin = last_tb;

    memcpy(ntb->string, ctb->string + startoffset, length);
    ntb->string[length] = 0;
    ntb->length = length;
    ntb->vfont = ctb->vfont;
    ntb->pin_flags = ctb->pin_flags;
    ntb->colour = ctb->colour;

    /* Turn intermediate (filling) newlines into spaces. */

    while ((nlp = Ustrchr(ntb->string, '\n')) != NULL) *nlp = ' ';
    }

  /* If we've dealt with the current (non-NULL) text block, break. Otherwise,
  reset the starting offset to zero and continue with the next block, if there
  is one. */

  if (ctb == tb) break;
  startoffset = 0;
  }

/* If we have not reached the end of the paragraph, skip spaces at a filling
break point, and move on to the next text block if at the end of an input
block. Spaces are also ignored at the start of a filled line (to cover the case
when the paragraph starts with spaces), but we also do this here in order to
discover whether we are at the end of the paragraph. */

while (tb != NULL)
  {
  if (fill) while (*p == ' ' || *p == '\n') p++;
  if (*p != 0) break;                   /* More input in this block */
  tb = tb->next;                        /* Move on to the next one */
  if (tb != NULL) p = tb->string;
  }

/* Set up data for the line, and pass back the new position for starting the
next line, if any. */

ol->width = linewidth;
ol->stretch = stretch? maxlinewidth - linewidth : -1;
ol->swidth = spacewidth;
ol->scount = spacecount;
ol->depth = maxlinedepth;

*offsetptr = (tb == NULL)? 0 : p - tb->string;
return tb;
}



/*************************************************
*             Polish a filled paragraph          *
*************************************************/

/* This function is called when filling. We scan the lines of the paragraph to
see if there are any instances of a very tight (but not hyphenated) line
followed by a very loose line that is not the last line. When we find such a
pair, we check whether it is possible to move the last word from the first line
onto the second line, but we do not do this if it would make the first line
unacceptably loose.

Arguments:
  pg             the paragraph block
  nestindent     the nesting indent

Returns:         nothing
*/

static void
para_polish(paragraph *pg, int nestindent)
{
outputline *ol1, *ol2;
int mwidth = pg->maxwidth - pg->layparm->indent - nestindent;

for (ol1 = pg->out;
     ol1 != NULL && (ol2 = ol1->next) != NULL && ol2->next != NULL;
     ol1 = ol2)
  {
  int c, len, nw, nsw, sc, sw, w, l1, l1n, l2;
  textblock *tb, *tbnew;
  uschar *p, *q, *pend;

  /* Loop, because it may be possible to move more than one word from one
  line to another. */

  for (;;)
    {
    if ((ol1->flags & OLF_HYPHENATED) != 0 ||
        ol1->swidth == 0 ||
        ol2->swidth == 0 ||
        (l1 = (mwidth - ol1->width)*100/ol1->swidth) >= L_VGOOD ||
        (l2 = (mwidth - ol2->width)*100/ol2->swidth) <= L_BAD)
      break;

    DEBUG(D_para)
      {
      debug_printf("Found tight followed by loose: %d %d\n", l1, l2);
      debug_printf("========================================"
                   "=======================================\n");
      debug_print_line_text(ol1);
      debug_printf("========================================"
                   "=======================================\n");
      debug_print_line_text(ol2);
      debug_printf("========================================"
                   "=======================================\n");
      }

    /* Find the last text block of the first line, and then find the last word
    in it - we know there is at least one space in the line (because the space
    width was tested above). However, the space may be in a previous text
    block. For the moment, we just handle the simplest case where the space is
    in the current text block. Maybe one day in future - but the other case
    must surely be pretty rare. */

    for (tb = ol1->txtblk; tb->next != NULL; tb = tb->next);

    p = pend = tb->string + tb->length;
    w = 0;

    while (p > tb->string && p[-1] != ' ')
      {
      p--;
      BACKCHAR(p);
      GETCHAR(c, p);
      w += font_charwidth(c, tb->vfont, NULL);
      }

    if (p <= tb->string)
      {
      DEBUG(D_para) debug_printf("No space found in last textblock\n");
      break;
      }

    DEBUG(D_para)
      {
      debug_printf("Last word (width %s) is: ", misc_formatfixed(w));
      debug_print_string(p, pend - p, "\n");
      }

    /* Now see if moving this word onto the next line is possible, and if so,
    check that the first line is left with an acceptable looseness. */

    sw = font_charwidth(' ', tb->vfont, NULL);

    if (ol2->width + w + sw > mwidth)
      {
      DEBUG(D_para) debug_printf("Too wide to move by %s\n",
        misc_formatfixed(ol2->width + w + sw - mwidth));
      break;
      }

    q = p - 1;
    sc = 1;
    while (q > tb->string && q[-1] == ' ')
      {
      q--;
      sc++;
      }

    /* What the new width, space width, and looseness would be. */

    nw = ol1->width - w - sc*sw;
    nsw = ol1->swidth - sc*sw;
    l1n = (mwidth - nw)*100/nsw;

    if (l1n >= L_ACCEPT)
      {
      DEBUG(D_para) debug_printf("First would be too loose: %d\n", l1n);
      break;
      }

    /* OK, we can move the word onto the next line. */

    *q = 0;  /* End the first line early */
    tb->length -= pend - q;

    /* Adjust values in first line */

    ol1->width = nw;
    ol1->swidth = nsw;
    ol1->scount -= sc;
    if (ol1->stretch > 0) ol1->stretch += w + sc*sw;

    /* Add in a new textblock at the front of the second line */

    len = pend - p + 1;
    tbnew = misc_malloc(sizeof(textblock) + len);
    tbnew->next = ol2->txtblk;
    ol2->txtblk = tbnew;
    tbnew->lastin = tb->lastin;     /* May not be 100% right, but... */
    tbnew->vfont = tb->vfont;
    tbnew->pin_flags = tb->pin_flags;
    tbnew->colour = tb->colour;
    tbnew->length = len;

    memcpy(tbnew->string, p, len - 1);
    tbnew->string[len-1] = ' ';
    tbnew->string[len] = 0;

    ol2->width += w + sw;
    ol2->swidth += sw;
    ol2->scount++;
    if (ol2->stretch > 0) ol2->stretch -= w + sw;

    DEBUG(D_para)
      {
      debug_printf("Word moved\n");
      debug_printf("========================================"
                   "=======================================\n");
      debug_print_line_text(ol1);
      debug_printf("========================================"
                   "=======================================\n");
      debug_print_line_text(ol2);
      debug_printf("========================================"
                   "=======================================\n");
      }
    }
  }
}



/*************************************************
*         Format the list of paragraphs          *
*************************************************/

/* This functions scans each paragraph and copies it into a list of lines,
according to whether the paragraph is being filled or not. Various width data
are preserved for subsequent justification when the lines are output. For
paragraphs within table entries, there is the concept of "soft overflow" for
which warnings are suppressed. The function stops when it hits the end of the
list or an <index> item.

Argument:    the start of the item list to be processed
Returns:     TRUE to continue processing
*/

BOOL
para_format(item *item_list)
{
item *stop_at = NULL;
paragraph *pg;
tdatastr *td = NULL;
int justify = J_LEFT;
int colnumber = -1;
int widest_line = 0;
int nestindent = 0;
int nestendent = 0;
int extraliteralindent = 0;
int list_nest_depth = 0;
int mainorfn = LP_MAIN;
int currlistindent[MAXLISTNEST];
int currlistendent[MAXLISTNEST];

int  prev_justify = 0;
int  prev_read_linenumber = 0;
int  prev_skipped_warning = 0;
int  prev_widest_line = 0;

BOOL skipped_warning = FALSE;

currlistindent[0] = 0;
currlistendent[0] = 0;

DEBUG(D_any)
  {
  if (!inheadorfoot || debug_selector != D_any)
    debug_printf("Formatting paragraphs\n");
  }

if (Ustrcmp(item_list->name, "index") == 0) stop_at = item_list->partner;

/* Process the items, starting at the second, so as to skip the initial dummy
or <index> item. */

for (item *i = item_list->next; i != NULL; i = i->next)
  {
  layoutparam *lp;
  textblock *tb;
  outputline **olanchor;
  int offset;
  int indent;

  read_linenumber = i->linenumber;
  if (Ustrcmp(i->name, "#FILENAME") == 0)
    {
    read_filename = i->p.string;
    continue;
    }

  /* Stop when we hit the start of an index (when processing main text) or the
  end of an index (when processing an index). */

  if (i == stop_at || Ustrcmp(i->name, "index") == 0) break;

  /* The only reason for recognizing table entries here is so that we can
  suppress "overlong line" warnings for columns that are not in fact going to
  overprint anything. This is helpful for sources that use one-line tables for
  printing left-centre-right fields in a kind of heading line, but of course it
  can apply to all kinds of tables. */

  if (Ustrcmp(i->name, "#TDATA") == 0)
    {
    td = i->p.tdata;
    continue;
    }

  else if (Ustrcmp(i->name, "row") == 0)
    {
    colnumber = 0;
    prev_skipped_warning = 0;
    continue;
    }

  else if (Ustrcmp(i->name, "entry") == 0)
    {
    colnumber++;
    skipped_warning = FALSE;
    widest_line = 0;
    continue;
    }

  /* Blockquotes, epigraphs, sidebard, and notes are treated like other
  indented lists. */

  else if (Ustrcmp(i->name, "blockquote") == 0 ||
           Ustrcmp(i->name, "epigraph") == 0)
    {
    nestindent += currlistindent[list_nest_depth];
    nestendent += currlistendent[list_nest_depth++];
    currlistindent[list_nest_depth] = lptable[mainorfn + LP_BLOCKQUOTE]->indent;
    currlistendent[list_nest_depth] = lptable[mainorfn + LP_BLOCKQUOTE]->endent;
    continue;
    }

  else if (Ustrcmp(i->name, "note") == 0)
    {
    nestindent += currlistindent[list_nest_depth];
    nestendent += currlistendent[list_nest_depth++];
    currlistindent[list_nest_depth] = lptable[mainorfn + LP_NOTE]->indent;
    currlistendent[list_nest_depth] = lptable[mainorfn + LP_NOTE]->endent;
    continue;
    }

  else if (Ustrcmp(i->name, "sidebar") == 0)
    {
    nestindent += currlistindent[list_nest_depth];
    nestendent += currlistendent[list_nest_depth++];
    currlistindent[list_nest_depth] = lptable[mainorfn + LP_SIDEBAR]->indent;
    currlistendent[list_nest_depth] = lptable[mainorfn + LP_SIDEBAR]->endent;
    continue;
    }

  else if (Ustrcmp(i->name, "itemizedlist") == 0)
    {
    nestindent += currlistindent[list_nest_depth];
    nestendent += currlistendent[list_nest_depth++];
    currlistindent[list_nest_depth] = lptable[mainorfn + LP_ILISTPARA]->indent;
    currlistendent[list_nest_depth] = lptable[mainorfn + LP_ILISTPARA]->endent;
    continue;
    }

  else if (Ustrcmp(i->name, "orderedlist") == 0)
    {
    nestindent += currlistindent[list_nest_depth];
    nestendent += currlistendent[list_nest_depth++];
    currlistindent[list_nest_depth] = lptable[mainorfn + LP_OLISTPARA]->indent;
    currlistendent[list_nest_depth] = lptable[mainorfn + LP_OLISTPARA]->endent;
    continue;
    }

  /* The varlist_tempindent variable makes the title unindented. We turn
  it off when the first <varlistentry> is encountered, but it is also turned on
  and off during <term> processing so that the <term> text is not indented. */

  else if (Ustrcmp(i->name, "variablelist") == 0)
    {
    nestindent += currlistindent[list_nest_depth];
    nestendent += currlistendent[list_nest_depth++];
    currlistindent[list_nest_depth] = lptable[mainorfn + LP_VLISTPARA]->indent;
    currlistendent[list_nest_depth] = lptable[mainorfn + LP_VLISTPARA]->endent;
    continue;
    }

  else if (Ustrcmp(i->name, "literallayout") == 0)
    {
    extraliteralindent += currlistindent[list_nest_depth];
    continue;
    }

  else if (Ustrcmp(i->name, "footnote") == 0)
    {
    mainorfn = LP_FOOTNOTE;
    continue;
    }

  else if (Ustrcmp(i->name, "/") == 0)
    {
    if (Ustrcmp(i->partner->name, "itemizedlist") == 0 ||
        Ustrcmp(i->partner->name, "orderedlist") == 0 ||
        Ustrcmp(i->partner->name, "blockquote") == 0 ||
        Ustrcmp(i->partner->name, "epigraph") == 0 ||
        Ustrcmp(i->partner->name, "note") == 0 ||
        Ustrcmp(i->partner->name, "sidebar") == 0)
      {
      nestindent -= currlistindent[--list_nest_depth];
      nestendent -= currlistendent[list_nest_depth];
      }

    else if (Ustrcmp(i->partner->name, "variablelist") == 0)
      {
      nestindent -= currlistindent[--list_nest_depth];
      nestendent -= currlistendent[list_nest_depth];
      }

    else if (Ustrcmp(i->partner->name, "literallayout") == 0)
      extraliteralindent = 0;

    else if (Ustrcmp(i->partner->name, "row") == 0) colnumber = -1;

    else if (Ustrcmp(i->partner->name, "entry") == 0)
      {
      /* If we skipped an overlong line warning on the previous entry, it means
      the previous left-justified column's data overflowed, but we didn't warn
      because a soft overflow into this column is permitted. Check for this and
      give the warning if necessary. For any justification, we can omit the
      warning if this column's width is zero (empty column), unless the
      overflow goes over this column too, in which case we either give an error
      (if this is the last column), or set up to check at the next one. */

      if (prev_skipped_warning > 0)
        {
        int overflow = 0;
        int prev_plus_colspace = td->coldata[colnumber-1].width +
          table_right_col_space + table_left_col_space;

        /* Overflow into this cell will always be ok, but see if the overflow
        goes on into the next cell. If so, we have to arrange for this check to
        happen, with appropriate data, next time, unless this is the last
        column, in which case an error is given. */

        if (widest_line == 0)
          {
          overflow = prev_widest_line - prev_plus_colspace -
            td->coldata[colnumber].width;
          if (overflow > 0)
            {
            if (colnumber < td->colcount)    /* There's another column */
              {
              overflow = 0;
              skipped_warning = TRUE;
              justify = prev_justify;
              read_linenumber = prev_read_linenumber;
              widest_line = prev_widest_line - prev_plus_colspace;
              }
            }
          }

        /* Only a teeny overflow can happen into a left-justified column that
        is not empty. */

        else if (justify == J_LEFT)
          overflow = prev_widest_line - prev_plus_colspace;

        /* Handle overflow into centred and right-justified cells */

        else overflow = prev_widest_line -
          prev_plus_colspace -
          (td->coldata[colnumber].width - widest_line)/
            ((justify == J_CENTRE)? 2 : 1);

        /* Give an error for a previous column if there is an overprinting
        overflow. */

        if (overflow > 0)
          {
          read_linenumber = prev_read_linenumber;
          (void)error(62, misc_formatfixed(overflow),
            colnumber - prev_skipped_warning);
          }
        }

      /* Remember this column's characteristics */

      if (skipped_warning) prev_skipped_warning++;
      prev_widest_line = widest_line;
      prev_justify = justify;
      prev_read_linenumber = read_linenumber;
      }

    else if (Ustrcmp(i->partner->name, "footnote") == 0)
      {
      mainorfn = LP_MAIN;
      }

    continue;
    }

  /* The only other thing we recognize here is a paragraph. */

  else if (Ustrcmp(i->name, "#PCPARA") != 0) continue;

  /* Process a paragraph */

  pg = i->p.prgrph;
  lp = pg->layparm;
  tb = pg->intxtblk;
  olanchor = &(pg->out);

  justify = (pg->justify != J_UNSET)? pg->justify : lp->justify;

  /* If we are filling, we remove spaces at the start of the paragraph, and we
  also coalesce multiple spaces in the middle. This assumes that when there are
  font changes, the widths of spaces are the same in each font. */

  if (lp->fill)
    {
    BOOL lastwasspace = TRUE;
    textblock *stb;

    for (stb = tb; stb != NULL; stb = stb->next)
      {
      uschar *p;
      uschar *pend = stb->string + stb->length;

      for (p = stb->string; *p != 0; p++)
        {
        if (*p != ' ' && *p != '\n') lastwasspace = FALSE;
        else if (!lastwasspace) lastwasspace = TRUE;
        else
          {
          uschar *pp = p + 1;
          while (*pp == ' ' || *pp == '\n') pp++;
          stb->length -= pp - p;
          if (*pp == 0)
            {
            *p = 0;
            break;
            }
          memmove(p, pp, pend - pp + 1);
          pend -= pp - p;
          lastwasspace = FALSE;
          }
        }
      }
    }

  /* Insert a rule at the start, if required. */

  if ((i->flags & IF_RULEABOVE) != 0)
    olanchor = add_rule(olanchor, 0, pg->maxwidth);

  /* Loop till the end of the paragraph, creating the lines. Note that the
  first line has its own indent. */

  fni = i->next;         /* For finding footnotes */
  offset = 0;
  indent = lp->indent1 + nestindent + extraliteralindent;

  while (tb != NULL)
    {
    int rightindent;
    outputline *ol = misc_malloc(sizeof(outputline));

    ol->next = NULL;
    *olanchor = ol;
    olanchor = &(ol->next);

    ol->indent = indent;
    ol->txtblk = NULL;
    ol->flags = 0;
    ol->fnstr = NULL;

    /* This function sets ol->width, ol->stretch, ol->swidth, ol->scount,
    and ol->depth, as well as hooking on the text blocks and any footnote
    pointers. */

    tb = para_get_next_line(tb, &offset,
      pg->maxwidth - indent - lp->endent - nestendent,
      ol, lp->fill, justify == J_BOTH);

    /* Add in any additional leading imposed for this paragraph. */

    ol->depth += pg->extra_leading;

    /* Check the maximum line width. In a table, there are two flags that
    control overflow warnings. HARDOF means that overflow is hard and should
    always cause a warning. SOFTOF means that warnings can be suppressed in the
    following two cases:

    (1) If this is a left-justified non-last column and there's no column
    separator, and the width is less than the rest of the width of the table,
    delay a possible warning. When the next column has been formatted, there is
    a check to see if the overflow is, in fact, soft. See the code above that
    is run for </entry>. The full table width check is necessary in case there
    are no further <entry>s for the row.

    (2) Conversely, do a similar check for a right-justified non-first column.
    In this case we have all the data here, and can just skip giving the
    warning.

    If neither HARDOF nor SOFTOF is set, overflow in a table is ignored. */

    if (ol->width > pg->maxwidth - indent &&
         (colnumber <= 0 ||
           (pg->intxtblk->pin_flags & (PIN_HARDOF|PIN_SOFTOF)) != 0))
      {
      int overflow = ol->width - (pg->maxwidth - indent);

      /* Not the last column in a table, not HARDOF, left justified, no
      following separator: set the flag for checking at the end of the next
      column. */

      if (colnumber > 0 &&
          colnumber < td->colcount &&
          (pg->intxtblk->pin_flags & PIN_HARDOF) == 0 &&
          justify == J_LEFT &&
          !td->coldata[colnumber].sep)
        {
        int k;
        int avail = td->coldata[colnumber].width;
        for (k = colnumber + 1; k <= td->colcount; k++)
          avail += table_left_col_space + td->coldata[k].width +
                   table_right_col_space;
        if (ol->width > avail)
          (void)error(62, misc_formatfixed(overflow), colnumber);
        else
          skipped_warning = TRUE;
        }

      /* Error if not in a table, or in the first column, or HARDOF is set, or
      current is not right justified, or previous was right-justified and not
      empty, or there was a separator, or there was an overprint. */

      else if (colnumber <= 1 ||
               (pg->intxtblk->pin_flags & PIN_HARDOF) != 0 ||
               justify != J_RIGHT ||
               (prev_justify == J_RIGHT && prev_widest_line > 0) ||
               td->coldata[colnumber-1].sep ||
               ol->width >
                 td->coldata[colnumber].width +
                 table_right_col_space +
                 table_left_col_space +
                 (td->coldata[colnumber-1].width - prev_widest_line)/
                   ((prev_justify == J_CENTRE)? 2 : 1))
        {
        if (colnumber > 0)
          (void)error(62, misc_formatfixed(overflow), colnumber);  /* table */
        else
          (void)error(25, misc_formatfixed(overflow));    /* not in a table */
        }
      }

    /* Save the maximum width in a table entry */

    if (ol->width > widest_line) widest_line = ol->width;

    /* Handle right, centre, and character alignment. */

    rightindent = pg->maxwidth - lp->endent - indent - ol->width;
    if (justify == J_RIGHT) ol->indent += rightindent;
      else if (justify == J_CENTRE) ol->indent += rightindent/2;

    /* If the alignment is J_CHAR, it must have come from a paragraph's own
    setting, as this happens only for tables; the value cannot be set in a
    layparm. This code is messy, since we have to scan along the text blocks to
    the alignment character, to find the width of what is to the left of it. */

    else if (justify == J_CHAR && pg->charval != 0)
      {
      int w = 0;
      int incindent;
      textblock *oltb;

      for (oltb = ol->txtblk; oltb != NULL; oltb = oltb->next)
        {
        uschar *wanted;
        uschar utf8buf[8];
        utf8buf[misc_ord2utf8(pg->charval, utf8buf)] = 0;

        wanted = Ustrstr(oltb->string, utf8buf);

        if (wanted == NULL)
          w += font_stringwidth(oltb->string, oltb->vfont);
        else
          {
          int save = *wanted;
          *wanted = 0;
          w += font_stringwidth(oltb->string, oltb->vfont);
          *wanted = save;
          break;
          }
        }

      /* If we didn't find the character, we'll have the width of the entire
      line; thus we behave as if the wanted char is at the end. Do not allow a
      negative indent, or one that is greater than the right-justified value. */

      incindent = ((pg->maxwidth - lp->endent - indent) * pg->charoff)/100 - w;
      if (incindent < 0)
        {
        incindent = 0;
        error(101);
        }
      else if (incindent > rightindent)
        {
        incindent = rightindent;
        error(102);
        }
      ol->indent = incindent;
      }

    /* Reset indent for 2nd and subsequent lines */

    indent = lp->indent + nestindent + extraliteralindent;
    }   /* Loop for lines in a paragraph */

  /* Insert a rule at the end, if required. */

  if ((i->flags & IF_RULEBELOW) != 0)
    olanchor = add_rule(olanchor, 0, pg->maxwidth);

  /* If we are filling, we can try to improve the layout of a paragraph. */

  if (lp->fill) para_polish(pg, nestindent);
  }

DEBUG(D_para) debug_print_para(item_list, NULL, "after para_format()");

read_linenumber = 0;
return TRUE;
}

/* End of para.c */
