/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains functions for handling tables. */

#include "sdop.h"


/*************************************************
*       Check table for just one group           *
*************************************************/

/* We support only single-group tables at present. We can assume that the items
are correctly nested. As well as finding the group, we also find the thead,
tfoot, and tbody items, if present.

Arguments:
  t          the table item
  thptr      where the save the <thead> pointer
  tfptr      where the save the <tfoot> pointer
  tbptr      where the save the <tbody> pointer

Returns:     the group item, or NULL
*/

static item *
table_check_onegroup(item *t, item **thptr, item **tfptr, item **tbptr)
{
item *tg = NULL;

for (item *i = t; i != t->partner; i = i->next)
  {
  if (Ustrcmp(i->name, "thead") == 0)
    {
    *thptr = i;
    i = i->partner;
    }
  else if (Ustrcmp(i->name, "tfoot") == 0)
    {
    *tfptr = i;
    i = i->partner;
    }
  else if (Ustrcmp(i->name, "tbody") == 0)
    {
    *tbptr = i;
    i = i->partner;
    }
  else if (Ustrcmp(i->name, "tgroup") == 0)
    {
    if (tg != NULL)
      {
      (void)error(33);             /* Too many tgroups */
      return NULL;
      }
    tg = i;
    }
  }

if (tg == NULL) (void)error(34);   /* Missing tgroup */
return tg;
}



/*************************************************
*        Interpret an alignment setting          *
*************************************************/

/* If the item given has an "align" setting, interpret it and put in into the
place specified. Also deal with "char" and "charoff" settings. If not found,
leave the value alone.

Arguments:
  aptr       points to where to put a new value for "align"
  cptr       points to where to put a new value for "char"
  coptr      points to where to put a new value for "charoff";
  i          the item
Returns:     nothing
*/

static void
set_align(int *aptr, int *cptr, int *coptr, item *i)
{
paramstr *p = misc_param_find(i, US"align");
if (p != NULL)
  {
  if (Ustrcmp(p->value, "center") == 0 ||
      Ustrcmp(p->value, "centre") == 0)
    *aptr = J_CENTRE;
  else if (Ustrcmp(p->value, "justify") == 0) *aptr = J_BOTH;
  else if (Ustrcmp(p->value, "left") == 0) *aptr = J_LEFT;
  else if (Ustrcmp(p->value, "right") == 0) *aptr = J_RIGHT;
  else if (Ustrcmp(p->value, "char") == 0) *aptr = J_CHAR;
  }

p = misc_param_find(i, US"char");
if (p != NULL)
  {
  int x = 0;
  uschar *s = p->value;
  if (*s == '&')
    {
    uschar *value;
    s = entity_find(s + 1, &value, FALSE, US" in \"char\" setting");
    GETCHAR(x, value);
    }
  else x = *s++;
  if (*s != 0) error(100); else *cptr = x;
  }

p = misc_param_find(i, US"charoff");
if (p != NULL)
  {
  int x = 0;
  uschar *s = p->value;
  while (isdigit(*s)) x = x*10 + (*s++) - '0';
  if (*s != 0) error(75, p->value, "charoff percentage");
    else *coptr = x;
  }
}



/*************************************************
*        Find the depth of a table row           *
*************************************************/

/* For each <entry> add up the depths of its paragraphs, keeping the largest
value. Then add on row separation space if a separator is to be drawn.

Arguments:
  td        the table data structure
  i         the <row> item

Returns:    the depth
*/

int
table_row_depth(tdatastr *td, item *i)
{
int celldepth;
int rowdepth = 0;

for (item *k = i->next; k != i->partner; k = k->next)
  {
  if (Ustrcmp(k->name, "entry") != 0) continue;
  celldepth = 0;
  for (item *kk = k->next; kk != k->partner; kk = kk->next)
    {
    outputline *ol;
    if (Ustrcmp(kk->name, "#PCPARA") != 0) continue;
    for (ol = kk->p.prgrph->out; ol != NULL; ol = ol->next)
      celldepth += ol->depth;
    }
  if (celldepth > rowdepth) rowdepth = celldepth;
  k = k->partner;
  }

if ((i->flags & IF_ROWSEP) != 0)
  {
  BOOL drawsep = FALSE;
  item *inext = i->partner->next;
  while (inext != NULL && inext->name[0] == '?') inext = inext->next;
  if (Ustrcmp(inext->name, "row") == 0) drawsep = TRUE;
    else if (Ustrcmp(inext->name, "/") == 0)
      drawsep = (Ustrcmp(inext->partner->name, "thead") == 0 &&
                  (td->flags & (TDF_HASBODY|TDF_HASFOOT)) != 0) ||
                (Ustrcmp(inext->partner->name, "tbody") == 0 &&
                  (td->flags & TDF_HASFOOT) != 0);
  if (drawsep) rowdepth += table_row_sep_space;
  }

return rowdepth;
}



/*************************************************
*        Identify tables and set parameters      *
*************************************************/

/* This function is called after paragraphs have been identified, to find any
tables, sort out and remember their column widths and other parameters via a
#TDATA item. If the table has a <tfoot>, we move that section to be after
<tbody>. This function stops when it hits the end of the list or an <index>
item.

Argument:
  item_list     the start of the item list to be processed
  stop_at       where to stop (non-NULL when processing a footnote)

Returns:     TRUE if all went well; FALSE otherwise, to stop further processing
*/

BOOL
table_identify(item *item_list, item *stop_at)
{
int fnindent = 0;
int linewidth = page_linewidth;
int colwidths[256];
int colratios[256];
uschar colset[256];

colwidths[0] = 0;  /* Not used, set for tidiness */
colratios[0] = 0;
colset[0] = FALSE;

table_overall_indent = 0;   /* Reset default for each set of tables */

DEBUG(D_any) debug_printf("Identifying tables\n");

if (Ustrcmp(item_list->name, "index") == 0) stop_at = item_list->partner;

/* Loop for all the tables in the document. */

for (item *i = item_list->next; i != NULL; i = i->next)
  {
  uschar *name = i->name;
  int colcount;
  int collast;
  int width_total;
  int colnum, rownum, toprowsize;
  int align_default;
  int align_char_default;
  int align_charoff_default;
  int entry_align;
  int entry_align_char;
  int entry_align_charoff;
  BOOL colsep_default;
  BOOL have_ratios;
  BOOL tablerowsep;
  BOOL tgrouprowsep;
  tdatastr *td;
  paramstr *frameparm, *p;

  item *tg, *j, *tdi;
  item *thead, *tfoot, *tbody;

  if (i == stop_at || Ustrcmp(name, "index") == 0) break;

  if (Ustrcmp(name, "#FILENAME") == 0)
    {
    read_filename = i->p.string;
    continue;
    }
  read_linenumber = i->linenumber;

  if (Ustrcmp(name, "footnote") == 0)
    {
    fnindent = footnote_indent;
    continue;
    }

  if (Ustrcmp(name, "/") == 0 && Ustrcmp(i->partner->name, "footnote") == 0)
    {
    fnindent = 0;
    continue;
    }

  if (Ustrcmp(name, "?sdop") == 0)
    {
    pin_table_layout_changes(i);
    continue;
    }

  if (Ustrcmp(name, "table") != 0 && Ustrcmp(name, "informaltable") != 0)
    continue;

  /* Initialize for a new table */

  colcount = 0;
  collast = 0;
  thead = tfoot = tbody = NULL;

  p = misc_param_find(i, US"rowsep");
  tablerowsep = (p != NULL)?
    (Ustrcmp(p->value, "0") != 0) : global_rowsep_default;
  tgrouprowsep = tablerowsep;

  /* We currently support one, and only one, tgroup in a table. It must contain
  the number of columns. */

  tg = table_check_onegroup(i, &thead, &tfoot, &tbody);
  if (tg == NULL) return FALSE;

  p = misc_param_find(tg, US"cols");
  if (p == NULL) return error(45, "tgroup", "cols");
  colcount = misc_get_number(p->value);
  if (colcount <= 0) return error(44, "cols", p->value);

  p = misc_param_find(tg, US"rowsep");
  if (p != NULL) tgrouprowsep = Ustrcmp(p->value, "0") != 0;

  /* If <tfoot> and <tbody> exist, move <tfoot> so that it follows <tbody> */

  if (tfoot != NULL && tbody != NULL)
    {
    tfoot->partner->next->prev = tfoot->prev;
    tfoot->prev->next = tfoot->partner->next;

    tfoot->prev = tbody->partner;
    tfoot->partner->next = tbody->partner->next;

    tbody->partner->next->prev = tfoot->partner;
    tbody->partner->next = tfoot;
    }

  /* Mark the columns unset */

  for (int x = 1; x <= colcount; x++)
    {
    colwidths[x] = 0;
    colratios[x] = 0;
    colset[x] = FALSE;
    }

  /* Create a #TDATA item in which to remember stuff. */

  td = misc_malloc(sizeof(tdatastr) + colcount * sizeof(tcolstr));
  td->flags = global_tableflags_default;

  /* Set flag if this is the special TOC table (non-standard DocBook) */

  p = misc_param_find(i, US"toc");
  if (p != NULL && Ustrcmp(p->value, "yes") == 0)
    td->flags |= TDF_TOC;

  /* Get default settings from <table> and <tgroup> */

  colsep_default = global_colsep_default;
  p = misc_param_find(i, US"colsep");
  if (p != NULL) colsep_default = Ustrcmp(p->value, "0") != 0;

  p = misc_param_find(tg, US"colsep");
  if (p != NULL) colsep_default = Ustrcmp(p->value, "0") != 0;

  align_default = global_align_default;
  align_char_default = global_align_char_default;
  align_charoff_default = global_align_charoff_default;

  set_align(&align_default, &align_char_default, &align_charoff_default, tg);

  for (int x = 1; x <= colcount; x++)
    {
    td->coldata[x].align = align_default;
    td->coldata[x].charval = align_char_default;
    td->coldata[x].charoff = align_charoff_default;
    td->coldata[x].sep = colsep_default;
    }

  frameparm = misc_param_find(i, US"frame");
  if (frameparm != NULL)
    {
    td->flags &= ~(TDF_TOPFRAME|TDF_BOTFRAME|TDF_SIDEFRAME);
    if (Ustrcmp(frameparm->value, "all") == 0)
      td->flags |= TDF_TOPFRAME|TDF_BOTFRAME|TDF_SIDEFRAME;
    else if (Ustrcmp(frameparm->value, "bottom") == 0)
      td->flags |= TDF_BOTFRAME;
    else if (Ustrcmp(frameparm->value, "sides") == 0)
      td->flags |= TDF_SIDEFRAME;
    else if (Ustrcmp(frameparm->value, "top") == 0)
      td->flags |= TDF_TOPFRAME;
    else if (Ustrcmp(frameparm->value, "topbot") == 0)
      td->flags |= TDF_TOPFRAME|TDF_BOTFRAME;
    }

  td->layparm = ((td->flags & (TDF_TOPFRAME|TDF_BOTFRAME)) != 0)?
    &framed_table_layparm : &table_layparm;

  /* We currently require the column widths to be specified. Search from the
  <tgroup> until we hit <thead>, <tfoot>, or <tbody>. */

  for (j = tg->next; j != tg->partner; j = j->next)
    {
    name = j->name;
    if (Ustrcmp(name, "thead") == 0 ||
        Ustrcmp(name, "tfoot") == 0 ||
        Ustrcmp(name, "tbody") == 0)
      break;

    if (Ustrcmp(name, "colspec") == 0)
      {
      int fp;
      uschar *s;

      colnum = collast + 1;                /* Default column number */
      read_linenumber = j->linenumber;     /* For errors in the colspec */

      /* Deal with an explicit column number */

      p = misc_param_find(j, US"colnum");
      if (p != NULL)
        {
        colnum = misc_get_number(p->value);
        if (colnum <= 0)
          return error(36, "positive number", "colnum", p->value);
        }
      if (colnum > colcount) return error(47, colnum, colcount);
      if (colset[colnum]) return error(37, colnum);

      /* Remember this as the last column */

      collast = colnum;

      /* Now deal with the width. We read a number and look at what follows to
      see if it's absolute or a ratio. The latter may be followed by an
      adjustment. */

      p = misc_param_find(j, US"colwidth");
      if (p == NULL) return error(46, "colspec", "colwidth");

      fp = misc_get_fp(p->value, &s);

      if (*s == '*')
        {
        colratios[colnum] = fp;
        colwidths[colnum] = 0;
        colset[colnum] = TRUE;
        s++;
        if (*s == '+' || *s == '-')
          {
          int sign = (*s == '+')? +1 : -1;
          fp  = misc_get_dimension(s+1);
          if (fp < 0) (void)error(44, "colwidth", p->value);
            else colwidths[colnum] = fp * sign;
          }
        else if (*s != 0) (void)error(44, "colwidth", p->value);
        }

      /* Fixed width */

      else
        {
        colwidths[colnum] = misc_scale_number(fp, s);
        if (colwidths[colnum] < 0)
          {
          (void)error(38, p->value, "100pt");
          colwidths[colnum] = 100000;
          }
        colratios[colnum] = 0;
        colset[colnum] = TRUE;
        }

      /* Deal with separation */

      p = misc_param_find(j, US"colsep");
      if (p != NULL) td->coldata[colnum].sep = Ustrcmp(p->value, "0") != 0;

      /* Set any column-specific alignment */

      set_align(&(td->coldata[colnum].align),
                &(td->coldata[colnum].charval),
                &(td->coldata[colnum].charoff),
                j);
      }
    }

  read_linenumber = i->linenumber;   /* Revert */

  /* We have now hit head/body/foot. Check that widths have been specified for
  all of the columns. */

  have_ratios = FALSE;
  for (int x = 1; x <= colcount; x++)
    {
    if (!colset[x])
      {
      (void)error(39, x, US"100pt");
      colwidths[x] = 100000;
      colratios[x] = 0;
      }
    if (colratios[x] != 0) have_ratios = TRUE;
    }

  /* If any widths were specified as ratios, we have to do some computation. */

  if (have_ratios)
    {
    int ratio_total = 0;
    int available_width = linewidth;
    for (int x = 1; x <= colcount; x++)
      {
      available_width -= colwidths[x];
      ratio_total += colratios[x];
      }
    for (int x = 1; x <= colcount; x++)
      colwidths[x] += MULDIV(available_width, colratios[x], ratio_total);
    }

  /* Now check that the total width of the columns is not too wide. */

  DEBUG(D_itable) debug_printf("Table with %d columns\nWidths:\n", colcount);

  width_total = 0;
  for (int x = 1; x <= colcount; x++)
    {
    DEBUG(D_itable) debug_printf(" %s", misc_formatfixed(colwidths[x]));
    width_total += colwidths[x];
    }
  DEBUG(D_itable) debug_printf(" Total = %s\n", misc_formatfixed(width_total));

  if (width_total + table_overall_indent > linewidth - fnindent)
    {
    uschar s1[32];
    uschar s2[32];
    uschar s3[32];
    Ustrcpy(s1, misc_formatfixed(width_total));
    Ustrcpy(s2, misc_formatfixed(linewidth - fnindent));
    if (table_overall_indent == 0)
      (void)error(40, s1, s2);
    else
      {
      Ustrcpy(s3, misc_formatfixed(table_overall_indent));
      (void)error(99, s1, s3, s2);
      }
    }

  /* Now reduce the width of each column by the left and right cell
  indentations. */

  for (int x = 1; x <= colcount; x++)
    {
    if (x != 1 || (td->flags & TDF_SIDEFRAME) != 0)
      colwidths[x] -= table_left_col_space;
    if (x != colcount || (td->flags & TDF_SIDEFRAME) != 0)
      colwidths[x] -= table_right_col_space;
    }

  /* Put the widths into the table's parameter block, and remember it via
  a #TDATA item in the chain. */

  td->twidth = width_total;
  td->indent = table_overall_indent;
  td->colcount = colcount;
  for (int x = 1; x <= colcount; x++) td->coldata[x].width = colwidths[x];

  tdi = misc_malloc(sizeof(item));
  tdi->partner = tdi;
  tdi->linenumber = tg->linenumber;
  tdi->flags = 0;
  Ustrcpy(tdi->name, "#TDATA");
  tdi->p.tdata = td;
  misc_insert_item(tdi, tg->next);

  /* Now we scan through the table's data, setting the widths and alignment
  requirements of all the #PCPARA items according to which column they are in.
  For rows, set the IF_ROWSEP flag if a row separator is required. For the
  first row, find the largest font. We need to remember which of head/body/foot
  are present. */

  colnum = 0;
  rownum = 0;
  toprowsize = 0;

  entry_align = J_LEFT;      /* These values are never used; they are set */
  entry_align_char = 0;      /* here just to stop compilers from complaining */
  entry_align_charoff = 0;   /* about uninitialized variables. */

  for (j = tdi->next; j != tg->partner; j = j->next)
    {
    if (Ustrcmp(j->name, "thead") == 0) td->flags |= TDF_HASHEAD;
    else if (Ustrcmp(j->name, "tfoot") == 0) td->flags |= TDF_HASFOOT;
    else if (Ustrcmp(j->name, "tbody") == 0) td->flags |= TDF_HASBODY;
    else if (Ustrcmp(j->name, "row") == 0)
      {
      rownum++;
      colnum = 0;
      entry_align = td->coldata[colnum].align;
      entry_align_char = td->coldata[colnum].charval;
      entry_align_charoff = td->coldata[colnum].charoff;
      p = misc_param_find(j, US"rowsep");
      if ((p != NULL && Ustrcmp(p->value, "0") != 0) ||
          (p == NULL && tgrouprowsep))
        j->flags |= IF_ROWSEP;
      }
    else if (Ustrcmp(j->name, "entry") == 0)
      {
      colnum++;
      if (colnum > colcount)
        {
        read_linenumber = j->linenumber;
        (void)error(41, colcount, (colcount == 1)? "" : "s");
        }
      else
        {
        entry_align = td->coldata[colnum].align;
        entry_align_char = td->coldata[colnum].charval;
        entry_align_charoff = td->coldata[colnum].charoff;
        set_align(&entry_align, &entry_align_char, &entry_align_charoff, j);
        }
      }
    else if (Ustrcmp(j->name, "#PCPARA") == 0)
      {
      j->flags |= IF_ISENTRY;
      j->p.prgrph->maxwidth = colwidths[colnum];
      j->p.prgrph->justify = entry_align;
      j->p.prgrph->charval = entry_align_char;
      j->p.prgrph->charoff = entry_align_charoff;
      if (rownum == 1)
        {
        textblock *tb;
        for (tb = j->p.prgrph->intxtblk; tb != NULL; tb = tb->next)
          if (tb->vfont->size > toprowsize) toprowsize = tb->vfont->size;
        }
      }
    }

  /* Save the largest font in the top row (for framing) */

  td->toprowsize = toprowsize;

  /* If there is a title, set the width of its paragraphs. The width is either
  fixed, or to be taken from the table itself.*/

  for (j = i->next; j != i->partner; j = j->next)
    if (Ustrcmp(j->name, "title") == 0) break;

  if (j != i->partner)
    {
    int width = (table_title_width > 0)? table_title_width : td->twidth;
    if (width <= page_linewidth)
      {
      item *k;
      for (k = j->next; k != j->partner; k = k->next)
        {
        if (Ustrcmp(k->name, "#PCPARA") != 0) continue;
        if (table_title_justify != J_UNSET)
          k->p.prgrph->justify = table_title_justify;
        k->p.prgrph->maxwidth = width;
        }
      }
    }

  /* Continue after the current table */

  i = i->partner;
  }

DEBUG(D_itable) debug_print_para(item_list, NULL, "after table_identify()");

return TRUE;
}

/* End of table.c */
