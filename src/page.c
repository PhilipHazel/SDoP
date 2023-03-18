/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains functions for doing things to pages. */

#include "sdop.h"


enum { ACCEPT_OK, ACCEPT_NO, ACCEPT_SP };


/*************************************************
*               Static variables                 *
*************************************************/

static item *last_accepted;
static item *last_primary;
static int  last_after_min;
static int  last_after_max;
static int  stretchable;
static int  usedonpage;
static BOOL footnote_encountered;



/*************************************************
*       Find extra space needed after title      *
*************************************************/

/* This function is called when processing a title line, or a one-line table
that is treated as a kind of title line. It inspects the next "paragraph"
(whatever it actually is), and returns an amount of space needed if at least
some of it is to fit on the current page. If the next paragraph has 4 or more
lines, require there to be room for at least two of them. If it is 3 or fewer,
require there to be room for all of them.

Arguments:
  ip         pointer to the ending item of the title
  aftermin   the minimum space that follows the title or in-line table

Returns:   extra space needed or zero
*/

static int
find_title_extra(item *ip, int aftermin)
{
int extra = 0;
outputline *ol;

for (ip = ip->next; ip != NULL; ip = ip->next)
  {
  if (Ustrcmp(ip->name, "#PCPARA") == 0) break;
  if (Ustrcmp(ip->name, "mediaobject") == 0 ||
      Ustrcmp(ip->name, "inlinemediaobject") == 0)
    {
    int dummy;
    return object_find_size(ip, &dummy);
    }
  }

if (ip != NULL)
  {
  paragraph *pg = ip->p.prgrph;
  int xpcount = 0;
  int depth3 = 0;

  extra = (aftermin > pg->layparm->beforemin)?
    aftermin : pg->layparm->beforemin;

  for (ol = pg->out; ol != NULL; ol = ol->next)
    {
    if (++xpcount <= 2) extra += ol->depth;
    if (xpcount == 3) depth3 += ol->depth;
    }

  if (xpcount < 4) extra += depth3;
  }

return extra;
}



/*************************************************
*          Find level of an index line           *
*************************************************/

/* This function is called to determine the level of an index line, in order
not to place a primary that has secondaries last in a column.

Argument:  the index paragraph item
Returns:   TRUE if this is a primary entry
*/

static BOOL
isprimary(paragraph *pg)
{
if (pg->out != NULL)
  {
  textblock *tb = pg->out->txtblk;
  if (tb != NULL)
    {
    int c;
    uschar *s = tb->string;
    GETCHAR(c, s);
    if (c == HARD_SPACE) return FALSE;
    }
  }
return TRUE;
}



/*************************************************
*    Find extra space needed for an index line   *
*************************************************/

/* This function is called for a primary index line. It checks to see if it is
followed by a secondary, and sets up the extra space required for that case, so
that at least one secondary line is on the same page as its primary.

Arguments:
  ip         pointer to the item after the #PCPARA
  aftermin   the minimum space that follows the paragraph

Returns:   extra space needed or zero
*/

static int
find_index_extra(item *ip, int aftermin)
{
int extra = 0;

for (; ip != NULL; ip = ip->next)
  { if (Ustrcmp(ip->name, "#PCPARA") == 0) break; }

if (ip != NULL && !isprimary(ip->p.prgrph))
  {
  outputline *ol;
  paragraph *pg = ip->p.prgrph;
  extra = (aftermin > pg->layparm->beforemin)?
    aftermin : pg->layparm->beforemin;
  for (ol = pg->out; ol != NULL; ol = ol->next) extra += ol->depth;
  }

return extra;
}



/*************************************************
*       Handle the fitting of a table            *
*************************************************/

/* This function finds the depth of a table and decides whether it will fit
on the current page, and if not, whether to push it all onto the next page or
to split it.

Arguments:
  ip         table item

Returns:     ACCEPT_OK if the whole table is accepted
             ACCEPT_NO if none of the table is accepted
             ACCEPT_SP if table has been split
*/

static int
accept_table(item *ip)
{
int total = 0;
int titledepth = 0;
int extra = 0;
int fcount;
int rcount = 0;
int hrcount = 0;
int frcount = 0;
int rdepths[MAXTABLEROWS];
item *ritems[MAXTABLEROWS];
BOOL rseps[MAXTABLEROWS];
BOOL inhead = FALSE;
BOOL infoot = FALSE;
tdatastr *td = NULL;
tdatastr *tdnew;
item *j, *k, *fpushed, *lpushed;
item *tgroup = NULL;
item *tbody = NULL;
item *tfoot = NULL;

/* This variable is commented because its value is not currently used, and
compilers complain. */

/* item *thead = NULL; */

/* Look for a title, and find its depth */

for (j = ip->next; j != ip->partner; j = j->next)
  if (Ustrcmp(j->name, "title") == 0) break;

if (j != ip->partner)
  {
  for (k = j->next; k != j->partner; k = k->next)
    if (Ustrcmp(k->name, "#PCPARA") == 0) break;
  if (k != j->partner)
    {
    outputline *ol;
    for (ol = k->p.prgrph->out; ol != NULL; ol = ol->next)
      titledepth += ol->depth;
    }
  }

/* Scan the table, counting the number of rows and finding the depth for
each row. Save each row's item along with its depth. At the same time we
can remember the table's data block and where it's tbody and thead and tfoot
start. */

for (j = ip->next; j != ip->partner; j = j->next)
  {
  if (Ustrcmp(j->name, "#TDATA") == 0)
    {
    td = j->p.tdata;
    }

  else if (Ustrcmp(j->name, "tbody") == 0)
    {
    tbody = j;
    }

  else if (Ustrcmp(j->name, "thead") == 0)
    {
    /* thead = j;  not currently used */
    inhead = TRUE;
    }

  else if (Ustrcmp(j->name, "/") == 0 &&
           Ustrcmp(j->partner->name, "thead") == 0)
    {
    inhead = FALSE;
    }

  else if (Ustrcmp(j->name, "tfoot") == 0)
    {
    tfoot = j;
    infoot = TRUE;
    }

  else if (Ustrcmp(j->name, "/") == 0 &&
           Ustrcmp(j->partner->name, "tfoot") == 0)
    {
    infoot = FALSE;
    }

  else if (Ustrcmp(j->name, "tgroup") == 0)
    {
    tgroup = j;
    }

  else if (Ustrcmp(j->name, "row") == 0)
    {
    if (rcount >= MAXTABLEROWS) error(49, MAXTABLEROWS);   /* Hard */
    ritems[rcount] = j;
    rseps[rcount] = (j->flags & IF_ROWSEP) != 0;
    rdepths[rcount] = table_row_depth(td, j);
    total += rdepths[rcount++];
    if (inhead) hrcount++;
    if (infoot) frcount++;
    j = j->partner;
    }
  }    /* End of scan for row depths */

/* The variable frcount (number of rows in a foot) is not currently used, so we
need to avoid a compiler warning. */

(void)frcount;

/* Not finding the table's data block is a hard error. */

if (td == NULL) (void)error(42);

/* Allow for top and bottom frame space. */

if ((td->flags & TDF_TOPFRAME) != 0) total += table_top_frame_space;
if ((td->flags & TDF_BOTFRAME) != 0) total += table_bot_frame_space;

DEBUG(D_page) debug_printf("table: titledepth=%d rows=%d tabledepth=%d "
  "last_after_min/max=%d/%d used=%d\n", titledepth, rcount, total,
  last_after_min, last_after_max, usedonpage);

/* If we are not at the top of the page adjust spacing after the previous item
if this one requires more before it. */

if (usedonpage != 0)
  {
  if (last_after_min < td->layparm->beforemin)
    last_after_min = td->layparm->beforemin;
  if (last_after_max < td->layparm->beforemax)
    last_after_max = td->layparm->beforemax;
  }

/* If there are few rows, treat as a sort of "title" and make sure there's
enough space for what follows. */

if (rcount < 2) extra = find_title_extra(ip->partner, td->layparm->aftermin);

/* If everything previously accepted, plus the intermediate space, plus this
table, plus any extra requirement, fits on the page, accept the table. */

if (usedonpage + last_after_min + total + titledepth + extra <= page_length)
  {
  usedonpage += last_after_min + total + titledepth;
  stretchable += last_after_max - last_after_min;   /* Stretchable space */
  last_accepted = ip->partner;
  last_after_min = td->layparm->aftermin;
  last_after_max = td->layparm->aftermax;
  return ACCEPT_OK;
  }

/* Otherwise we have to decide whether and where to split the table. If there
are fewer than 4 rows, break the page before this table. Otherwise fit as much
as possible on this page, but always ensure that at least two rows are left for
the next page. */

/* >>>>> FIX ME >>>>> The current assumption is that a head or a foot consists
of just one row. The logic should be improved to handle other cases. */

if (rcount < 4) return ACCEPT_NO;

total = usedonpage + last_after_min + titledepth;

for (fcount = 0; fcount < rcount - 2; fcount++)
  {
  if (total + rdepths[fcount] > page_length) break;
  total += rdepths[fcount];
  }

DEBUG(D_page) debug_printf("table overflow: %d out of %d rows fit\n",
  fcount, rcount);

/* If fewer than 2 body rows fit, end the page before this table. */

if (fcount - hrcount < 2) return ACCEPT_NO;

/* We are going to split the table. Record how much the first part has used. */

usedonpage = total;
stretchable += last_after_max - last_after_min;

/* Set up the first and last items to be pushed, and remove them from the
current table. Also remove the <tfoot> part. */

fpushed = ritems[fcount];

for (lpushed = fpushed;
     lpushed->next != tbody->partner;
     lpushed = lpushed->next);

fpushed->prev->next = lpushed->next;
lpushed->next->prev = fpushed->prev;

if (tfoot != NULL)
  {
  tfoot->prev->next = tfoot->partner->next;
  tfoot->partner->next->prev = tfoot->prev;
  }

/* Now create a new table following this one, after the partner of the current
table. Give it the same parameters. */

last_accepted = ip->partner;
j = misc_insert_element_pair(ip->name, last_accepted);
j->p.param = ip->p.param;

tdnew = misc_malloc(sizeof(tdatastr) + td->colcount * sizeof(tcolstr));
tdnew->flags = td->flags;
tdnew->layparm = td->layparm;
tdnew->twidth = td->twidth;
tdnew->indent = td->indent;
tdnew->toprowsize = td->toprowsize;   /* <<<<<< FIX ME <<<<<< */
tdnew->colcount = td->colcount;
memcpy(tdnew->coldata, td->coldata, td->colcount * sizeof(tcolstr));

td->flags |= TDF_CONTA;          /* Mark first table continued */
td->flags &= ~TDF_BOTFRAME;      /* Cancel its bottom frame */

tdnew->flags |= TDF_CONTB;       /* Mark new table as continued */

/* Cancel its top frame if the last row had no sep */

if (rseps[fcount-1]) tdnew->flags |= TDF_TOPFRAME;
  else tdnew->flags &= ~TDF_TOPFRAME;

k = misc_malloc(sizeof(item));
k->partner = k;
k->linenumber = j->linenumber;
k->flags = 0;
Ustrcpy(k->name, "#TDATA");
k->p.tdata = tdnew;
misc_insert_item(k, j->next);

j = misc_insert_element_pair(US"tgroup", k);
if (tgroup != NULL) j->p.param = tgroup->p.param;
j = misc_insert_element_pair(US"tbody", j);
if (tbody != NULL) j->p.param = tbody->p.param;

/* Insert the cut out rows and the foot and we are done. */

j->next->prev = lpushed;
lpushed->next = j->next;
j->next = fpushed;
fpushed->prev = j;

if (tfoot != NULL)
  {
  tfoot->prev = j->partner;
  tfoot->partner->next = j->partner->next;
  j->partner->next->prev = tfoot->partner;
  j->partner->next = tfoot;
  }

return ACCEPT_SP;
}



/*************************************************
*       Handle the fitting of a paragraph        *
*************************************************/

/* This function finds the depth of a paragraph and decides whether it will fit
on the current page, and if not, whether to push it all onto the next page or
to split it. When a paragraph is split, the first part is put before the
argument item, so that that item becomes the second half. Thus, when there is a
page break, it's always before the argument item.

Arguments:
  ip           the paragraph item
  contiguous   TRUE if the paragraph should not be split
  isindex      TRUE if processing the index

Returns:       ACCEPT_OK if the whole paragraph is accepted
               ACCEPT_NO if none of the paragraph is accepted
               ACCEPT_SP if the paragraph has been split
*/

static int
accept_paragraph(item *ip, BOOL contiguous, BOOL isindex)
{
outputline *ol;
paragraph *pg = ip->p.prgrph;
paragraph *newpg;
textblock *lastt;
textblock *tb;
item *app, *bpp, *ipp, *pp;
int i;
int extra = 0;
int total = 0;
int pcount = 0;
int pdepths[MAXPARALINES];

/* Set up a vector of the depth of each line, including footnote depths where
relevant. The first footnote on a page adds an extra overhead. */

for (ol = pg->out; ol != NULL; ol = ol->next)
  {
  footnotestr *fn;
  int depth = ol->depth;
  if (pcount >= MAXPARALINES) error(48, MAXPARALINES);   /* Hard */

  /* Process each footnote */

  for (fn = ol->fnstr; fn != NULL; fn = fn->next)
    {
    tdatastr *td = NULL;
    item *fi = fn->footnote;

    if (!footnote_encountered)
      {
      depth += footnote_overhead;
      footnote_encountered = TRUE;
      }

    /* Scan the items that comprise the footnote in order to find its depth. If
    we hit "row", we are in a table, and need to find the row depth. Otherwise,
    just add up paragraph depths. */

    for (pp = fi->next; pp != fi->partner; pp = pp->next)
      {
      if (Ustrcmp(pp->name, "#TDATA") == 0)
        {
        td = pp->p.tdata;
        depth += td->layparm->beforemax;
        if ((td->flags & TDF_TOPFRAME) != 0) depth += table_top_frame_space;
        if ((td->flags & TDF_BOTFRAME) != 0) depth += table_bot_frame_space;
        }
      else if (Ustrcmp(pp->name, "row") == 0)
        {
        depth += table_row_depth(td, pp);
        pp = pp->partner;
        }
      else if (Ustrcmp(pp->name, "#PCPARA") == 0)
        {
        outputline *fol;
        depth += pp->p.prgrph->layparm->beforemax;
        for (fol = pp->p.prgrph->out; fol != NULL; fol = fol->next)
          depth += fol->depth;
        }
      }
    }

  /* We now have the depth of the line plus the depth of the associated
  foonotes, if any. */

  pdepths[pcount++] = depth;
  total += depth;
  }

/* If we are not at the top of the page adjust spacing after the previous item
if this one requires more before it. */

if (usedonpage != 0)
  {
  if (last_after_min < pg->layparm->beforemin)
    last_after_min = pg->layparm->beforemin;
  if (last_after_max < pg->layparm->beforemax)
    last_after_max = pg->layparm->beforemax;
  }

/* If we are dealing with a title, find the extra space needed for the
following item, or part thereof. */

if (Ustrcmp(ip->prev->name, "title") == 0)
  extra = find_title_extra(ip->prev->partner, pg->layparm->aftermin);

/* If we are dealing with an index item, see if it is a primary with a
following secondary; if so, require extra space. */

if (isindex && isprimary(pg))
  extra = find_index_extra(ip->next, pg->layparm->aftermin);

DEBUG(D_page) debug_printf("paragraph: lines=%d depth=%d extra=%d "
  "last_after_min/max=%d/%d used=%d\n", pcount, total, extra, last_after_min,
  last_after_max, usedonpage);

/* If everything previously accepted, plus the intermediate space, plus this
paragraph, plus any extra required space, fits on the page, accept the
paragraph. The ip item is a #PCPARA item; its predecessor is one of those that
can contain data, such as <para>, <title>, <literallayout>, etc. After
accepting, we set last_accepted to its partner, as the last accepted item. */

if (usedonpage + last_after_min + total + extra <= page_length)
  {
  usedonpage += last_after_min + total;
  stretchable += last_after_max - last_after_min;   /* Stretchable space */
  last_accepted = ip->prev->partner;                /* The </...> element */
  if (isindex && isprimary(pg)) last_primary = ip;  /* Save for overflow */
  last_after_min = pg->layparm->aftermin;
  last_after_max = pg->layparm->aftermax;
  return ACCEPT_OK;
  }

/* If extra is non-zero, we've failed to fit the title plus whatever follows.
If we are at the top of a page, this means there's a large image of some sort
that follows. (If a paragraph follows, we will have checked just for a couple
of lines worth of space, which there should be at the top of a page, and if
it's an index "paragraph" extra is set only near the bottom of the page.) If we
break the page, there will be an infinite loop, as a title won't be splittable.
So, test again without the extra space (and we know that usedonpage is 0 and so
last_after_min will also be 0. This should accept the title, but then push the
image on to the next page. (If it's too big for the page, accept_mediaobject()
will handle it.) */

if (extra > 0 && usedonpage == 0 && total <= page_length)
  {
  usedonpage += total;
  last_accepted = ip->prev->partner;                /* The </...> element */
  last_after_min = pg->layparm->aftermin;
  last_after_max = pg->layparm->aftermax;
  return ACCEPT_OK;
  }

/* In an index we never split paragraphs. */

if (isindex) return ACCEPT_NO;

/* Otherwise we have to decide whether and where to split the paragraph. If
it is non-splittable or if there are fewer than 4 lines, break the page before
this paragraph. Otherwise fit as much as possible on this page, but always
ensure that at least two lines are left for the next page. */

if (contiguous || pcount < 4) return ACCEPT_NO;

total = usedonpage + last_after_min;
for (i = 0; i < pcount - 2; i++)
  {
  if (total + pdepths[i] > page_length) break;
  total += pdepths[i];
  }

/* If fewer than 2 lines fit, end the page before this paragraph. */

if (i < 2) return ACCEPT_NO;

/* Find last line of first part, and last textblock of the last line. There are
occasions on which the last line is empty and has no textblocks. In this event,
we use the last text block from a previous line, if there is one. When
processing literal output, there may be no textblocks at all. */

lastt = NULL;
for (ol = pg->out; i > 1; i--)
  {
  ol = ol->next;
  if (ol->txtblk != NULL)
    for (lastt = ol->txtblk; lastt->next != NULL; lastt = lastt->next);
  }

/* Now set up a continuation paragraph, setting the appropriate flags in
the two blocks, before ending the page. */

usedonpage = total;
stretchable += last_after_max - last_after_min;

ip->flags |= IF_PARACONTA;

newpg = misc_malloc(sizeof(paragraph));
*newpg = *pg;

/* We position the second part of the split paragraph after the last #PCDATA
item that was used for the first part. This is so that <indexterm> items end up
on (hopefully) the correct page. First find the last input textblock that was
used for this paragraph. */

for (tb = pg->intxtblk; tb->next != NULL; tb = tb->next);

/* Now scan the main items, looking for the item that corresponds to the last
textblock used for the first part of the paragraph. If all the lines in the
paragraph are empty (literal copying of blank lines), or if we hit a dummy
#PCDATA (inserted when a paragraph containing only one data item is split),
just use the first item that is found. If we hit the final input textblock of
the paragraph, something has gone wrong, but we can still insert the next part.
If we hit the end of the chain, it's a disaster. */

for (ipp = ip; ipp != NULL; ipp = ipp->next)
  {
  if (Ustrcmp(ipp->name, "#PCDATA") != 0) continue;
  if (lastt == NULL ||
      (ipp->flags & IF_DUMMYDATA) != 0 ||
      ipp->p.txtblk == lastt->lastin) break;
  if (ipp->p.txtblk == tb)
    {
    error(65);
    break;
    }
  }
if (ipp == NULL) error(86);    /* Hard */

/* There is one anomalous case. If the entire paragraph has only one #PCDATA
item, which can happen for a long <literallayout> item, by inserting the new
paragraph after it, we leave the second part of the paragraph without any
#PCDATA items. If it in turn has to be split, there is nowhere to insert the
split, and SDoP bombs with error 86 above. So in this case we insert a dummy
#PCDATA item, simply to mark the point at which to do a future split. */

for (pp = ipp->next; pp != NULL; pp = pp->next)
  {
  if (Ustrcmp(pp->name, "#PCDATA") == 0) break;   /* Found another #PCDATA */
  if (Ustrcmp(pp->name, "#PCPARA") == 0)          /* Hit next para without */
    {
    pp = NULL;
    break;
    }
  }

/* Insert an empty data item, flagged so that it is picked up above. */

if (pp == NULL)
  {
  pp = misc_malloc(sizeof(item));
  Ustrcpy(pp->name, "#PCDATA");
  pp->partner = pp;
  pp->linenumber = ip->linenumber;
  pp->flags = IF_DUMMYDATA;
  pp->p.txtblk = NULL;
  pp->next = ipp->next;
  pp->prev = ipp;
  ipp->next->prev = pp;
  ipp->next = pp;
  }

/* New paragraph starts with the remaining lines, preceded by an end and
restart of whatever wrapped this paragraph (<para>, <title>, <literallayout>,
or whatever. */

newpg->out = ol->next;
ol->next = NULL;

/* New #PCPARA item */

pp = misc_malloc(sizeof(item));
Ustrcpy(pp->name, "#PCPARA");
pp->partner = pp;
pp->linenumber = ip->linenumber;
pp->flags = IF_PARACONTB;
pp->p.prgrph = newpg;

/* New </...> item to end the first part */

app = misc_malloc(sizeof(item));
Ustrcpy(app->name, "/");
app->linenumber = ipp->linenumber;
app->flags = 0;
app->partner = ip->prev;
app->p.param = NULL;

/* New <...> item to start the second part */

bpp = misc_malloc(sizeof(item));
Ustrcpy(bpp->name, ip->prev->name);
bpp->linenumber = ipp->linenumber;
bpp->flags = 0;
bpp->p.param = NULL;
bpp->partner = ip->prev->partner;
bpp->partner->partner = bpp;

/* Now can make the new </...> item end the first part */

ip->prev->partner = app;

/* Join three items into a mini-list */

app->next = bpp;
bpp->prev = app;
bpp->next = pp;
pp->prev = bpp;

/* Insert the new items. */

pp->next = ipp->next;
app->prev = ipp;

ipp->next->prev = pp;
ipp->next = app;

/* The last accepted item is the inserted </...> item. */

last_accepted = app;
return ACCEPT_SP;
}



/*************************************************
*     Handle the fitting of a "media object"     *
*************************************************/

/* This function finds the depth of a <[inline]mediaobject>and decides whether
it will fit on the current page. Media objects are never split.

Arguments:
  ip         table item

Returns:     ACCEPT_OK if the object is accepted
             ACCEPT_NO if the object is not accepted
*/

static int
accept_mediaobject(item *i)
{
int dummy;
int fudge = 0;
int depth = object_find_size(i, &dummy);

DEBUG(D_page) debug_printf("media object: depth=%d\n", depth);

if (usedonpage == 0 && depth > page_length)  /* Too big for a whole page */
  {
  error(78);                                 /* Give error */
  fudge = -depth;                            /* Force acceptance */
  }

if (usedonpage + last_after_min + depth + fudge <= page_length)
  {
  usedonpage += last_after_min + depth;
  last_accepted = i->partner;            /* The </mediaobject> element */
  last_after_min = 0;
  last_after_max = 0;
  return ACCEPT_OK;
  }

return ACCEPT_NO;
}



/*************************************************
*                Create pages                    *
*************************************************/

/* This function scans the item list for tables and formatted paragraphs, and
does the pagination to make pages. For each page, a #PDATA item is inserted
into the list. We stop either when we hit the end of the list, or when we hit
an <index> item.

Arguments:
  item_list   the start of the item list to be processed
  next_list   return where we finished
  even_pages  TRUE if the number of pages must be even
  isindex     TRUE if processing the index
  pnoptr      where to return the number of pages
  what        text for debugging

Returns:      TRUE to continue
              the next item is returned via next_list (unless NULL)
*/

BOOL
page_format(item *item_list, item **next_list, BOOL even_pages, BOOL isindex,
  int *pnoptr, uschar *what)
{
item *ip, *pp;
item *stop_at = NULL;
pdatastr *pd;
int pagenumber = *pnoptr;
BOOL hadcolophon = FALSE;

page_columns = page_columns_init;
page_colsep = page_colsep_init;;

DEBUG(D_any) debug_printf("Paginating %s: even_pages=%d\n", what, even_pages);

if (Ustrcmp(item_list->name, "index") == 0) stop_at = item_list->partner;

/* If this is the scan from the start of the document, insert the first page
start after the initial anchor item. */

if (item_list->name[0] == 0)
  {
  pd = misc_malloc(sizeof(pdatastr));
  pd->available = page_length;
  pd->used = pd->stretchable = 0;

  pp = misc_malloc(sizeof(item));
  Ustrcpy(pp->name, "#PDATA");

  pp->partner = pp;
  pp->linenumber = 1;
  pp->flags = 0;
  pp->p.pdata = pd;

  pp->next = item_list->next;
  pp->prev = item_list;
  if (item_list->next != NULL) item_list->next->prev = pp;
  item_list->next = pp;
  }

/* If we are continuing pagination after hitting <index>, or if we are
formatting an index, search back for the #PDATA item that must precede. */

else
  {
  for (pp = item_list->prev; pp != NULL; pp = pp->prev)
    {
    if (Ustrcmp(pp->name, "#PDATA") == 0) break;
    }
  if (pp == NULL) error(63);    /* Hard */
  pd = pp->p.pdata;
  hadcolophon = Ustrcmp(item_list->name, "colophon") == 0;
  if (hadcolophon) pp->flags |= IF_NOHEADFOOT;
  pp = item_list;               /* Start page just after the given item */
  }

/* Scan the input and insert page boundaries where required. */

for (;;)   /* Loop for each page */
  {
  BOOL stop = FALSE;
  BOOL forced = FALSE;
  int topofcolumn = 0;
  int incolumn = 1;

  usedonpage = 0;
  last_accepted = NULL;
  last_primary = NULL;
  footnote_encountered = FALSE;
  pagenumber++;

  DEBUG(D_page) debug_printf("Page %d\n", pagenumber);

  /* Loop for multiple columns */

  for (;;)
    {
    item *backup_last_accepted = NULL;
    int backup_last_after_min = 0;
    int backup_last_after_max = 0;
    int backup_usedonpage = 0;
    int backup_stretchable = 0;
    BOOL notlast = FALSE;

    last_after_min = 0;
    last_after_max = 0;
    stretchable = 0;

    /* Loop for items in a column, starting from "where we got to". */

    for (ip = pp->next; ip != NULL; ip = ip->next)
      {
      uschar *name = ip->name;

      if (Ustrcmp(ip->name, "#FILENAME") == 0)
        {
        read_filename = ip->p.string;
        continue;
        }

      if (Ustrcmp(ip->name, "title") == 0 ||
          Ustrcmp(ip->name, "subtitle") == 0 ||
          Ustrcmp(ip->name, "term") == 0)
        {
        notlast = TRUE;
        continue;
        }

      if (Ustrcmp(ip->name, "/") == 0 &&
          (Ustrcmp(ip->partner->name, "title") == 0 ||
           Ustrcmp(ip->partner->name, "subtitle") == 0 ||
           Ustrcmp(ip->partner->name, "term") == 0))
        {
        notlast = FALSE;
        continue;
        }

      read_linenumber = ip->linenumber;

      /* Handle processing instructions - page forcing, column number
      changes, index heading option, etc. */

      if (Ustrcmp(name, "?sdop") == 0)
        {
        paramstr *p = misc_param_find(ip, US"format");
        if (p != NULL && Ustrcmp(p->value, "newpage") == 0)
          {
          if (last_accepted != NULL)
            {
            forced = TRUE;
            break;
            }
          }
        else
          {
          int oldcols = page_columns;
          pin_change_columns(ip);
          pin_paging_changes(ip);

          /* If the number of columns has changed, force a page break if we are
          currently not in the first column. Otherwise, remember the current
          point as the column top position. */

          if (page_columns != oldcols)
            {
            if (incolumn > 1)
              {
              forced = TRUE;
              break;
              }
            topofcolumn = usedonpage + last_after_min;
            }
          }
        }

      /* Process a table; kill potential backup if anything is accepted */

      else if (Ustrcmp(name, "table") == 0 ||
               Ustrcmp(name, "informaltable") == 0)
        {
        int rc = accept_table(ip);
        if (rc != ACCEPT_NO) backup_last_accepted = NULL;
        if (rc != ACCEPT_OK) break;
        ip = ip->partner;
        }

      /* Process a media object */

      else if (Ustrcmp(name, "mediaobject") == 0 ||
               Ustrcmp(name, "inlinemediaobject") == 0)
        {
        int rc = accept_mediaobject(ip);
        if (rc != ACCEPT_OK) break;
        backup_last_accepted = NULL;
        ip = ip->partner;
        }

      /* Skip footnotes */

      else if (Ustrcmp(name, "footnote") == 0)
        {
        ip = ip->partner;
        }

      /* For paragraphs that are <title>s or <term>s, "notlast" is set TRUE.
      In this case, save the previous accept point for a possible backtrack if
      nothing following fits on the page. If it's already set (multiple
      <term>s) leave it alone. For anything else, if we accept the whole or
      part of the paragraph, kill the backtrack. */

      else if (Ustrcmp(name, "#PCPARA") == 0)
        {
        int rc;
        if (notlast && backup_last_accepted == NULL)
          {
          backup_last_accepted = last_accepted;
          backup_last_after_min = last_after_min;
          backup_last_after_max = last_after_max;
          backup_usedonpage = usedonpage;
          backup_stretchable = stretchable;
          }

        rc = accept_paragraph(ip, notlast, isindex);
        if (rc != ACCEPT_NO && !notlast) backup_last_accepted = NULL;
        if (rc != ACCEPT_OK) break;
        }

      else if (ip == stop_at || Ustrcmp(name, "index") == 0)
        {
        if (last_accepted != NULL) last_accepted = ip->prev;
        forced = stop = TRUE;
        break;
        }

      else if (usedonpage != 0 &&
                  (Ustrcmp(name, "chapter")  == 0 ||
                   Ustrcmp(name, "preface")  == 0 ||
                     (Ustrcmp(name, "appendix") == 0 &&
                      document_type != DOC_ARTICLE)))
        {
        if (last_accepted != NULL) last_accepted = ip->prev;
        forced = TRUE;
        break;
        }

      else if (usedonpage != 0 &&
               Ustrcmp(name, "colophon") == 0 &&
               !hadcolophon)
        {
        if (last_accepted != NULL) last_accepted = ip->prev;
        forced = TRUE;
        hadcolophon = TRUE;
        break;
        }
      }  /* Loop for items in one column */

    /* At this point we have filled a column. If backup_last_accepted is not
    NULL, the last thing that fitted was a <title> or a <term>, and we do not
    want that to happen. So we back up to the previous position. */

    if (backup_last_accepted != NULL)
      {
      last_accepted = backup_last_accepted;
      last_after_min = backup_last_after_min;
      last_after_max = backup_last_after_max;
      usedonpage = backup_usedonpage;
      stretchable = backup_stretchable;
      }

    /* Save the amount of vertical space used, and the amount of stretchable
    space. */

    pd->used = usedonpage - ((incolumn == 1)? 0 : topofcolumn);
    pd->stretchable = stretchable;

    /* If what we have not accepted is an index non-primary line, insert a
    "continued" item after the last accepted item. This is all very tedious,
    but it won't be obeyed very often. */

    if (isindex && last_accepted != NULL)
      {
      item *ipp;
      for (ipp = last_accepted->next; ipp != NULL; ipp = ipp->next)
        { if (Ustrcmp(ipp->name, "#PCPARA") == 0) break; }

      if (ipp != NULL && !isprimary(ipp->p.prgrph))
        {
        uschar buffer[256];
        int nest_stackptr = 0;
        item *nest_stack[NESTSTACKSIZE];
        item *anchor = misc_dummy_item();
        read_addto = anchor;

        /* The column data is re-initialized by para_format() so we have to
        re-instate it. */

        (void)sprintf(CS buffer,
          "<?sdop page_columns=\"%d\" page_column_separation=\"%s\"?>",
            page_columns, misc_formatfixed(page_colsep));
        (void)read_string(buffer, nest_stack, &nest_stackptr, 0);

        /* First, create a paragraph containing just " (continued)". Then copy
        the input lines from the last primary, chopping off the page numbers at
        the end, if any. */

        (void)read_string(US"<para>", nest_stack, &nest_stackptr, 0);
        (void)read_string(US S_HARD_SPACE, nest_stack, &nest_stackptr, 0);
        (void)read_string(US"<emphasis>(continued)</emphasis></para>",
          nest_stack, &nest_stackptr, 0);

        if (font_assign(anchor, FONTS_INDEX) &&
            font_loadalltables() &&
            para_identify(anchor, FONTS_INDEX, NULL))
          {
          if (last_primary != NULL)
            {
            item *newpara;
            for (newpara = anchor; newpara != NULL; newpara = newpara->next)
              { if (Ustrcmp(newpara->name, "#PCPARA") == 0) break; }

            if (newpara != NULL)
              {
              paragraph *pg = newpara->p.prgrph;
              textblock *tbcont = pg->intxtblk;
              textblock **ntbp = &(pg->intxtblk);
              textblock *tb, *ntb;
              for (tb = last_primary->p.prgrph->intxtblk; tb != NULL;
                   tb = tb->next)
                {
                if (Ustrncmp(tb->string, S_HARD_SPACE, Ustrlen(S_HARD_SPACE))
                  == 0) break;
                ntb = misc_malloc(sizeof(textblock) + tb->length + 1);
                memcpy(ntb, tb, sizeof(textblock) + tb->length + 1);
                ntb->next = NULL;
                *ntbp = ntb;
                ntbp = &(ntb->next);
                }
              *ntbp = tbcont;
              }
            }

          /* Format the new entry and insert it into the chain. */

          if (para_format(anchor))
            {
            anchor->prev = last_accepted;
            read_addto->next = last_accepted->next;
            last_accepted->next->prev = read_addto;
            last_accepted->next = anchor;
            }
          }
        }
      }

    /* If this is the final column or a page break is forced, the page is
    done. */

    if (ip == NULL || forced || incolumn >= page_columns) break;

    /* This is not the final column, it was terminated because something didn't
    fit, and there's something that can go in the next column. Insert a #PCOL
    item after the last accepted item. */

    DEBUG(D_page) debug_printf("End column %d: used=%d\n", incolumn,
      usedonpage);

    if (last_accepted != NULL)
      {
      pd = misc_malloc(sizeof(pdatastr));
      pd->available = page_length - topofcolumn;
      pd->used = pd->stretchable = 0;

      pp = misc_malloc(sizeof(item));
      Ustrcpy(pp->name, "#PCOL");

      pp->partner = pp;
      pp->linenumber = last_accepted->linenumber;
      pp->flags = 0;
      pp->p.pdata = pd;

      misc_insert_item(pp, last_accepted->next);
      }

    /* Now continue with the next column. */

    incolumn++;
    usedonpage = topofcolumn;
    }   /* Loop for columns on the page */

  /* At this point we have reached the end of the page. If ip != NULL we have
  stopped before the end of the item list. If the page break was forced, look
  to see whether there is any text or image that follows. If not, behave as if
  at the end of the file by setting ip = NULL. This deals with several special
  cases such as:

    . If we are processing an index we might have stopped at the stop_at
      element, or we might have stopped at a final column-resetting.

    . There might be a redundant <?sdop format="newpage"?> right at the end
      of the document.
  */

  if (ip != NULL && forced)
    {
    for (pp = ip->next; pp != NULL; pp = pp->next)
      if (Ustrcmp(pp->name, "#PCDATA") == 0 ||
          Ustrcmp(pp->name, "mediaobject") == 0 ||
          Ustrcmp(pp->name, "inlinemediaobject") == 0)
        break;
    if (pp == NULL) ip = NULL;
    }

  DEBUG(D_page)
    {
    debug_printf("End page: used=%d stretchable=%d\n", usedonpage,
      stretchable);
    if (ip == NULL || Ustrcmp(ip->name, "index") == 0)
      debug_printf("End section\n");
    }

  /* If ip is NULL, we have reached the end of the document. Break the outer
  loop unless we need to add a blank page to make up an even number. Otherwise
  (not at the end, or blank page required), insert a #PDATA item after the last
  accepted item, if there was one. */

  if (ip == NULL && (!even_pages || (pagenumber & 1) == 0)) break;

  if (last_accepted != NULL)
    {
    pd = misc_malloc(sizeof(pdatastr));
    pd->available = page_length;
    pd->used = pd->stretchable = 0;

    pp = misc_malloc(sizeof(item));
    Ustrcpy(pp->name, "#PDATA");

    pp->partner = pp;
    pp->linenumber = last_accepted->linenumber;
    pp->flags = (ip == NULL || hadcolophon)? IF_NOHEADFOOT : 0;
    pp->p.pdata = pd;
    pp->next = last_accepted->next;
    pp->prev = last_accepted;
    if (last_accepted->next != NULL) last_accepted->next->prev = pp;
    last_accepted->next = pp;

    /* If pp->next and ip are both NULL, we are forcing a page at the end of
    the file - this can happen for title pages. In this situation, we need to
    add *two* #PDATA items, in order to ensure a blank page. */

    if (pp->next == NULL && ip == NULL)
      {
      item *ppp = misc_malloc(sizeof(item));
      Ustrcpy(ppp->name, "#PDATA");

      ppp->partner = ppp;
      ppp->linenumber = last_accepted->linenumber;
      ppp->flags = IF_NOHEADFOOT;
      ppp->p.pdata = pd;

      ppp->next = NULL;
      ppp->prev = pp;
      pp->next = ppp;
      }
    }

  if (stop) break;     /* Hit an <index> element when processing main text, */
  }                    /* or </index> when processing an index. */

/* If next_list is not NULL, return where we got to, moving on one if it's the
end of an index. */

if (next_list != NULL)
  {
  if (ip != NULL && Ustrcmp(ip->name, "/") == 0) ip = ip->next;
  *next_list = ip;
  }

DEBUG(D_page) debug_print_item_list(item_list, "after page_format()");

*pnoptr = pagenumber;
return TRUE;
}

/* End of page.c */
