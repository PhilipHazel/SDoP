/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains debugging functions. */

#include "sdop.h"


/*************************************************
*             Debug printing function            *
*************************************************/

/* Pretty obvious really. Looks just like printf(), but outputs to stderr.

Arguments:
  format        a format string
  ...           data for same

Returns:        nothing
*/

void
debug_printf(const char *format, ...)
{
va_list ap;
if (debug_need_nl) fprintf(stderr, "\n");
debug_need_nl = FALSE;
va_start(ap, format);
(void)vfprintf(stderr, format, ap);
va_end(ap);
}



/*************************************************
*              Print a string                    *
*************************************************/

/* Non-printing characters are converted to escapes.

Arguments:
  s           the string
  len         the length of the string
  post        a string to print afterwards

Returns:      nothing
*/

void
debug_print_string(uschar *s, int len, const char *post)
{
for (; len-- > 0; s++)
  {
  if (*s >= 32 && *s < 127) debug_printf("%c", *s);
  else if (*s == '\n') debug_printf("\\n");
  else if (*s == '\\') debug_printf("\\");
  else debug_printf("\\x%02x", *s);
  }
debug_printf("%s", post);
}



/*************************************************
*            Print a fixed-point number          *
*************************************************/

/* If the number is actually -1, print it as -1 because that's usually used
to mean "unset" and not the fixed point value -0.001.

Argument:
  fn         an int containing a fixed-point number (usually millipoints)
  after      a string to print afterwards

Returns:     nothing
*/

void
debug_printfixed(int fn, const char *after)
{
if (fn == -1) debug_printf("-1%s", after);
  else debug_printf("%s%s", misc_formatfixed(fn), after);
}



/*************************************************
*               Print a textblock                *
*************************************************/

/* This function prints the contents of a textblock, showing its font
characteristics, if present.

Argument:    the textblock
Returns:     nothing
*/

static void
debug_print_textblock(textblock *tb)
{
vfontstr *vf = tb->vfont;

debug_printf("%03x", tb->pin_flags);

if (vf != NULL)
  {
  debug_printf("{%s,%s,", family_names[vf->family], type_names[vf->type]);
  debug_printfixed(vf->size, "}");
  }
else debug_printf(":");

debug_print_string(tb->string, tb->length, "\n");
}



/*************************************************
*            Print a line's text                 *
*************************************************/

/* This function prints an output line, with no extra data

Argument:   pointer to the line
Returns:    nothing
*/

void
debug_print_line_text(outputline *ol)
{
textblock *tb;
for (tb = ol->txtblk; tb != NULL; tb = tb->next)
  debug_print_string(tb->string, tb->length, "");
debug_printf("\n");
}



/*************************************************
*            Print a paragraph                   *
*************************************************/

/* This function prints the contents of a paragraph, with information about its
parameters.

Argument:   the #PCPARA item
Returns:    nothing
*/

static void
debug_print_paragraph(item *i)
{
paragraph *pg = i->p.prgrph;
layoutparam *lp = pg->layparm;
outputline *ol;
textblock *tb;
int justify = (pg->justify == J_UNSET)? lp->justify : pg->justify;

debug_printf("========================================"
             "=======================================\n");
debug_printf("%-10s Width: ", i->prev->name);
debug_printfixed(pg->maxwidth, " Spacing: ");
debug_printfixed(lp->beforemax, " ");
debug_printfixed(lp->beforemin, " ");
debug_printfixed(lp->aftermax,  " ");
debug_printfixed(lp->aftermin,  " ");
debug_printfixed(lp->indent,    " ");
debug_printfixed(lp->endent,    " ");
debug_printf("%s", (justify == J_LEFT)? "left" :
                   (justify == J_RIGHT)? "right" :
                   (justify == J_CENTRE)? "centre" : "both");
if (lp->fill) debug_printf(" fill");
if ((i->flags & IF_PARACONTA) != 0) debug_printf(" A");
if ((i->flags & IF_PARACONTB) != 0) debug_printf(" B");
debug_printf("\n");

for (tb = pg->intxtblk; tb != NULL; tb = tb->next) debug_print_textblock(tb);
debug_printf("----- Formatted -----\n");

for (ol = pg->out; ol != NULL; ol = ol->next)
  {
  debug_printf("width=");
  debug_printfixed(ol->width, " stretch=");
  debug_printfixed(ol->stretch, " swidth=");
  debug_printfixed(ol->swidth, " scount=");
  debug_printf("%d depth=", ol->scount);
  debug_printfixed(ol->depth, " indent=");
  debug_printfixed(ol->indent, "\n");
  for (tb = ol->txtblk; tb != NULL; tb = tb->next)
    debug_print_textblock(tb);
  }

debug_printf("========================================"
             "=======================================\n");
}



/*************************************************
*             Output an item list                *
*************************************************/

/* This function scans an item list and writes it out. It's called from
various places when the appropriate debugging switches are set. However, to
avoid outputting head/foot/toc stuff when we don't want it, the action is
disabled in those cases unless D_internal is set.

Arguments:
  item_list  the start of the list
  when       text string for heading, indicating where called from

Returns:     nothing
*/

void
debug_print_item_list(item *item_list, const char *when)
{
if (internal_processing && (debug_selector & D_internal) == 0) return;
debug_printf("----- Item list %s -----\n", when);
for (item *i = item_list; i != NULL; i = i->next)
  {
  if (i->next != NULL && i->next->prev != i)
    {
    debug_printf("*** Chain error: i=%p next=%p next->prev=%p\n",
      (void *)(i), (void *)(i->next), (void *)(i->next->prev));
    debug_printf("*** name=%s nextname=%s\n", i->name, i->next->name);
    }

  /* The anchor item for the whole shebang. */

  if (i->name[0] == 0) debug_printf("#Anchor\n");

  /* An input text item; there can be dummies, inserted when very large
  paragraphs are split. */

  else if (Ustrcmp(i->name, "#PCDATA") == 0)
    {
    if (i->p.txtblk == NULL)
      debug_printf("Dummy text block\n");
    else debug_print_textblock(i->p.txtblk);
    }

  /* An output paragraph */

  else if (Ustrcmp(i->name, "#PCPARA") == 0)
    debug_print_paragraph(i);

  /* A rawtitle item */

  else if (Ustrcmp(i->name, "#RAWTITLE") == 0 ||
           Ustrcmp(i->name, "#RAWTITLEABBREV") == 0 ||
           Ustrcmp(i->name, "#RAWSUBTITLE") == 0)
    {
    lengthstring *ls = i->p.lngthstrng;
    debug_printf("<%s> ", i->name);
    debug_print_string(ls->value, ls->length, "\n");
    }

  /* A page data item */

  else if (Ustrcmp(i->name, "#PDATA") == 0)
    {
    pdatastr *pd = i->p.pdata;
    debug_printf("<#PDATA> avail=");
    debug_printfixed(pd->available, " used=");
    debug_printfixed(pd->used, " stretchable=");
    debug_printfixed(pd->stretchable, "\n");
    }

  /* A column data item */

  else if (Ustrcmp(i->name, "#PCOL") == 0)
    {
    pdatastr *pd = i->p.pdata;
    debug_printf("<#PCOL> avail=");
    debug_printfixed(pd->available, "\n");
    }

  /* An index data item */

  else if (Ustrcmp(i->name, "#INDEXDATA") == 0)
    {
    indexstr *ix = i->p.ndxstr;
    debug_printf("<#INDEXDATA> %d %d \"", ix->ixnumber, ix->pagenumber);
    debug_print_string(ix->sorttext, Ustrlen(ix->sorttext), "\"\n");
    }

   /* A table data item */

  else if (Ustrcmp(i->name, "#TDATA") == 0)
    {
    int k;
    tdatastr *td = i->p.tdata;
    debug_printf("<#TDATA> flags=%0x width=", td->flags);
    debug_printfixed(td->twidth, " indent=");
    debug_printfixed(td->indent, " toprowsize=");
    debug_printfixed(td->toprowsize, "\n");
    for (k = 1; k <= td->colcount; k++)
      {
      tcolstr *tcs = &(td->coldata[k]);
      debug_printf("  column %d width=", k);
      debug_printfixed(tcs->width, " ");
      debug_printf("align=%d sep=%d\n", tcs->align, tcs->sep);
      }
    }

  /* A source file name item */

  else if (Ustrcmp(i->name, "#FILENAME") == 0)
    {
    debug_printf("<#FILENAME> %s\n", i->p.string);
    }

  /* An unknown #-item is an internal error. */

  else if (i->name[0] == '#')
    {
    debug_printf("** Unknown special item %s\n", i->name);
    }

  /* An ending item */

  else if (i->name[0] == '/')
    {
    item *p = i->partner;
    if (p == NULL)
      debug_printf("***bad end: no partner\n");
    else
      debug_printf("</%s>\n", p->name);
    }

  /* A starting item */

  else
    {
    paramstr *p;
    debug_printf("<%s", i->name);
    for (p = i->p.param; p != NULL; p = p->next)
      {
      debug_printf(" %s='%s'", p->name, p->value);
      }
    if (i->partner == i) debug_printf((i->name[0] == '?')? "?" : "/");
    debug_printf(">\n");
    }
  }
}




/*************************************************
*           Output paragraph list                *
*************************************************/

/* This function scans an item list, but skips everything except paragraphs
and page breaks. Again, skip this for internal stuff unless explicitly asked
for.

Arguments:
  item_list  the list to be scanned
  last_item  stop if this item is reached
  when       text string for heading, indicating where called from

Returns:     nothing
*/

void
debug_print_para(item *item_list, item *last_item, const char *when)
{
int pagenumber = 0;
BOOL pending_page = FALSE;

if (internal_processing && (debug_selector & D_internal) == 0) return;
debug_printf("----- Paragraphs %s -----\n", when);
for (item *i = item_list; i != NULL && i != last_item; i = i->next)
  {
  if (Ustrcmp(i->name, "#PCPARA") == 0)
    {
    if (pending_page)
      {
      debug_printf("===> PAGE %d ===>\n", ++pagenumber);
      pending_page = FALSE;
      }
    debug_print_paragraph(i);
    }
  if (Ustrcmp(i->name, "#PDATA") == 0)
    { if (pending_page) pagenumber++; else pending_page = TRUE; }
  }
}

/* End of debug.c */
