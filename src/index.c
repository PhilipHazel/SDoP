/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains code for creating one or more indexes. */

#include "sdop.h"


static short int *cased_table = NULL;
static short int *uncased_table = NULL;
static int max_col_char  = 0;

static uschar *inames[] = { US"primary", US"secondary", US"tertiary" };

static uschar *ixhlist = US"*0ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static int *ixh;



/*************************************************
*         Parse a line of collation data         *
*************************************************/

/* This function returns the next character from a line of collation data,
together with its flag (=, +, or blank).

Arguments:
  pp           pointer to position pointer (updated)
  pf           pointer to flag (updated)

Returns:       next character, -1 at end of line, -2 for a syntax error
*/

static int
next_collate_char(uschar **pp, int *pf)
{
uschar *p = *pp;
int c;

while (isspace(*p)) p++;
if (*p == 0 || *p == '#') return -1;

if (*p == '=' || *p == '+')
  {
  *pf = *p++;
  if (*p == 0 || isspace(*p)) return -2;
  }
else *pf = ' ';

/* Get the next UTF-8 character. If what follows is not end of line or
whitespace, expect a hex number. Otherwise, we already have the character. */

GETCHARINC(c, p);
if (*p != 0 && !isspace(*p))
  {
  int d = c;
  c = 0;
  for (;;)
    {
    if (isdigit(d)) c = (c << 4) + d - '0';
      else if (isxdigit(d)) c = (c << 4) + 10 + tolower(d) - 'a';
        else return -2;
    if (*p == 0 || isspace(*p)) break;
    GETCHARINC(d, p);
    }
  }
*pp = p;
return c;
}



/*************************************************
*            Create collation data               *
*************************************************/

/* This function reads a list of characters in collation order from the
"indexcollate" file in the shared data, and uses it to create two sorting
tables - case-sensitive and case-insensitive. The function may be called more
than once if there are multiple indexes.

Arguments:    none
Returns:      nothing
*/

static void
init_collation_data(void)
{
FILE *f;
uschar fn[128];
uschar buffer[256];
uschar *p;
int lineno = 0;
int c, flag, tablen;
int cs, us;

if (cased_table != NULL) return;     /* Already done the job */

DEBUG(D_index) debug_printf("Reading index collation data\n");

(void)misc_find_share(US"indexcollate", fn, TRUE);
f = Ufopen(fn, "rb");
if (f == NULL)   /* Hard error */
  (void)error(0, fn, "index collation file - required", strerror(errno));

/* We have to read the file twice. The first time finds the largest character
code that it contains. This determines the size of tables that we create. */

while (Ufgets(buffer, sizeof(buffer), f) != NULL)
  {
  lineno++;
  p = buffer;
  while ((c = next_collate_char(&p, &flag)) >= 0)
    if (c > max_col_char) max_col_char = c;
  if (c == -2) error(57, fn, lineno, p - buffer);  /* Hard */
  }

/* Create the two tables, large enough to hold all listed characters.
Initialize to -1 for unset entries. */

tablen = (max_col_char + 1) * sizeof(short int);
cased_table = misc_malloc(tablen);
uncased_table = misc_malloc(tablen);

memset(cased_table, 0xff, tablen);
memset(uncased_table, 0xff, tablen);

/* Re-read the file and fill in the tables. */

cs = 0;
us = 0;

rewind(f);
while (Ufgets(buffer, sizeof(buffer), f) != NULL)
  {
  p = buffer;
  while ((c = next_collate_char(&p, &flag)) >= 0)
    {
    if (flag == '+') cs++; else if (flag != '=') { cs++; us++; }
    cased_table[c] = cs;
    uncased_table[c] = us;
    }
  }

fclose(f);
}




/*************************************************
*     Comparison functions for index entries     *
*************************************************/

/* These functions are used both when sorting the entries and when subsequently
creating the index text. */


/* Compare page numbers */

static int
ixcompare_pagenumber(item *a, item *b)
{
int c = a->next->p.ndxstr->pagenumber - b->next->p.ndxstr->pagenumber;
if (c != 0) return c;
return a->next->p.ndxstr->endpage - b->next->p.ndxstr->endpage;
}


/* Compare the primary, secondary, or tertiary text of the entries. The
sorttext string contains all three strings, concatenated. Some of them may be
null. Code points less than max_col_char are looked up in the appropriate
collating table.

Arguments:
  a            first <indexterm> item
  b            second <indexterm> item
  which        0, 1, or 2
  caseless     as it says

Returns:       +ve, -ve, or zero
*/

static int
ixcompare_sorttext(item *a, item *b, int which, BOOL caseless)
{
int x, y, xc, yc;
short int *table = caseless? uncased_table : cased_table;
uschar *ta = a->next->p.ndxstr->sorttext;
uschar *tb = b->next->p.ndxstr->sorttext;

for (x = 0; x < which; x++)
  {
  while (*ta++ != 0);
  while (*tb++ != 0);
  }

for (;;)
  {
  GETCHARINC(x, ta);
  GETCHARINC(y, tb);
  if (x == 0) return (y == 0)? 0 : -1;
  if (y == 0) return +1;
  if (x != y)
    {
    xc = (x <= max_col_char)? table[x] : -1;
    yc = (y <= max_col_char)? table[y] : -1;
    if (xc < 0 || yc < 0 || xc != yc) break;
    }
  }

if (xc < 0) return (yc < 0)? (x - y) : +1;
if (yc < 0) return -1;

return xc - yc;
}


/* When texts are identical, we compare the fonts. The first function does the
compare for a given <primary>, <secondary>, or <tertiary> entry. In sync, scan
the #PCDATA items, comparing lengths and fonts. If the fonts are identical,
check for sub/superscripts and their depths. */

static int
ixcompare_textfont(item *xi, item *yi)
{
item *xii, *yii;

for (xii = xi->next, yii = yi->next; ; xii = xii->next, yii = yii->next)
  {
  int c;
  for (; xii != xi->partner; xii = xii->next)
    if (Ustrcmp(xii->name, "#PCDATA") == 0) break;
  if (xii == xi->partner) return 0;   /* No more text */

  for (; yii != yi->partner; yii = yii->next)
    if (Ustrcmp(yii->name, "#PCDATA") == 0) break;
  if (yii == yi->partner) return 0;   /* Should not occur */

  c = xii->p.txtblk->length - yii->p.txtblk->length;
  if (c != 0) return c;

  c = xii->p.txtblk->vfont->family - yii->p.txtblk->vfont->family;
  if (c != 0) return c;

  c = xii->p.txtblk->vfont->type - yii->p.txtblk->vfont->type;
  if (c != 0) return c;

  c = xii->p.txtblk->vfont->size - yii->p.txtblk->vfont->size;
  if (c != 0) return c;

  c = xii->p.txtblk->colour - yii->p.txtblk->colour;
  if (c != 0) return c;

  c = (int)(xii->p.txtblk->pin_flags &
         (PIN_SSPERCENT|PIN_SUBSCRIPT|PIN_SUPERSCRIPT)) -
      (int)(yii->p.txtblk->pin_flags &
         (PIN_SSPERCENT|PIN_SUBSCRIPT|PIN_SUPERSCRIPT));
  if (c != 0) return c;
  }

return 0;   /* Control doesn't get here. */
}


/* This function is called when sorting; the arguments are <indexterm>s. When
it is called, we know that both <indexitem>s must have the same text for the
current level, or no text at all. */

static int
ixcompare_termtextfont(item *a, item *b, int which)
{
item *xi, *yi;

/* Find the relevant <primary>, <secondary>, or <tertiary> items. Note that an
<indexterm> may be its own partner if it has no text (end of range). */

for (xi = a; xi != a->partner; xi = xi->next)
  if (Ustrcmp(xi->name, inames[which]) == 0) break;
if (xi == a->partner) return 0;   /* Neither has anything at this level */

for (yi = b; yi != b->partner; yi = yi->next)
  if (Ustrcmp(yi->name, inames[which]) == 0) break;
if (yi == b->partner) return 0;   /* Should not occur. */

return ixcompare_textfont(xi, yi);
}



/*************************************************
*        Compare function for index sorting      *
*************************************************/

/* This function is passed to qsort() to compare two index entries. If the sort
text compares equal, we look at the fonts involved. If those are the same, sort
by page number.

This is called after each <indexterm> has had a <#INDEXDATA> item inserted
after it.

Arguments:
  a           pointer to a pointer to one <indexterm> item
  b           pointer to a pointer to another <indexterm> item

Returns:      a comparison value (-1, 0, +1)
*/

static int
ixcompare(const void *a, const void *b)
{
int c, k;
item *ai = *((item **)a);
item *bi = *((item **)b);
for (k = 0; k < 3; k++)    /* Loop for primary, secondary, tertiary */
  {
  if ((c = ixcompare_sorttext(ai, bi, k, TRUE)) != 0) return c;  /* Caseless */
  if ((c = ixcompare_sorttext(ai, bi, k, FALSE)) != 0) return c; /* Caseful */
  if ((c = ixcompare_termtextfont(ai, bi, k)) != 0) return c;
  }
if ((c = ixcompare_pagenumber(ai, bi)) != 0) return c;
return 0;
}



/*************************************************
*         Find primary/secondary/tertiary        *
*************************************************/

/* This function finds the <primary>, <secondary>, <tertiary>, and <see> or
<seealso> elements within an <indexterm>, returning NULL for any that don't
exist. There can only be one of <see> or <seealso>.

Arguments:
  i            the <indexterm> item
  ptrs         vector for the primary/secondary/tertiary points
  seeptr       pointer to where to return the <see> or <seealso>

Returns:       FALSE if we can't even find a primary.
*/

static BOOL
find_psts(item *i, item **ptrs, item **seeptr)
{
item *ii;
ptrs[0] = ptrs[1] = ptrs[2] = *seeptr = NULL;
for (ii = i->next->next; ii != i->partner; ii = ii->next)
  {
  if (Ustrcmp(ii->name, "primary") == 0)
    {
    ptrs[0] = ii;
    ii = ii->partner;
    }
  else if (Ustrcmp(ii->name, "secondary") == 0)
    {
    ptrs[1] = ii;
    ii = ii->partner;
    }
  else if (Ustrcmp(ii->name, "tertiary") == 0)
    {
    ptrs[2] = ii;
    ii = ii->partner;
    }
  else if (Ustrcmp(ii->name, "see") == 0 ||
           Ustrcmp(ii->name, "seealso") == 0)
    {
    *seeptr = ii;
    break;            /* <see> or <seealso> has to be be last */
    }
  }
return ptrs[0] != NULL;
}



/*************************************************
*     Add page number(s) or "see" to output      *
*************************************************/

/* This function is called to add page number(s) or a "see" or "seealso" item
to an index line that is being created. It avoids repeating numbers - as they
are all sorted, repeats should come all together. A page number of INT_MAX
means "use the 'see' item".

Arguments:
  prefix       prefix text
  spage        start page number
  epage        end page number
  lpage        last written page number
  see          the "see"/"seealso" element, if there is one
  nest_stack   the nesting stack
  nest_ptrptr  pointer to the stack pointer

Returns:       the last page number written, or 0 after a "see"/"seealso" item
*/

static int
set_ref(uschar *prefix, int spage, int epage, int lpage, item *see,
  item **nest_stack, int *nest_ptrptr)
{
uschar buffer[24];

/* Handle a "see" item by moving its content to the current list position. */

if (spage == INT_MAX && see != NULL)
  {
  item *i;
  uschar *seetext = (Ustrcmp(see->name, "see") == 0)?
    US"<emphasis>see</emphasis> " :
    US"<emphasis>see also</emphasis> ";

  (void)read_string(prefix, nest_stack, nest_ptrptr, 0);
  (void)read_string(seetext, nest_stack, nest_ptrptr, 0);

  see->prev->next = see->partner->next;
  see->partner->next->prev = see->prev;

  read_addto->next->prev = see->partner;
  see->partner->next = read_addto->next;
  read_addto->next = see;
  see->prev = read_addto;
  read_addto = see->partner;

  /* Remove whitespace from the end of the item; this matters only if something
  else will follow on the index line - rare, but might as well get it right. */

  for (i = see->partner->prev; i != see; i = i->prev)
    {
    if (Ustrcmp(i->name, "#PCDATA") == 0)
      {
      while (i->p.txtblk->length > 0 &&
             isspace(i->p.txtblk->string[i->p.txtblk->length - 1]))
        i->p.txtblk->length--;
      break;
      }
    }

  return 0;
  }

/* Handle a page number item */

if (spage != lpage)
  {
  lpage = spage;
  (void)sprintf(CS buffer, "%s%d", prefix, spage);
  (void)read_string(buffer, nest_stack, nest_ptrptr, PIN_IREF);
  read_addto->p.txtblk->colour = xref_packed_colour;
  }

if (epage > spage)
  {
  lpage = epage;
  (void)sprintf(CS buffer, "%s%d", S_EN_DASH, epage);
  (void)read_string(buffer, nest_stack, nest_ptrptr, PIN_IREF);
  read_addto->p.txtblk->colour = xref_packed_colour;
  }

return lpage;
}



/*************************************************
*              Create an index if required       *
*************************************************/

/* This function scans the input document, and creates a specific index. We
stop when we hit the <index> item for the index we are creating.

Arguments:
  item_list    the list to be processed (the main list)
  index_item   the item that outputs this index

Returns:       TRUE to continue processing; FALSE to stop
               (currently always TRUE)
*/

BOOL
index_make(item *item_list, item *index_item)
{
BOOL rc = TRUE;
BOOL manyinitials = FALSE;
int lastinitial = -1;
int page = 0;
int index_entry_count = 0;
int nest_stackptr = 0;
int ixhnext = 0;
int lpage;
int ixn, k, kk;
item *i, *ii, *iii;
item *see, *iprev;
item *stn[3];
item *prevstn[3];
item *nest_stack[NESTSTACKSIZE];
item **index_sort_vector, **ivp;
uschar buffer[1024];

paramstr *p = misc_param_find(index_item, US"role");
uschar *role = (p == NULL)? US"" : p->value;

read_linenumber = index_item->linenumber;

DEBUG(D_any) debug_printf("===> Creating \"%s\" index \n", role);
read_what = US"processing index entries";

/* Add the index to the list of indexes */

for (ixn = 0; ixn < index_count; ixn++)
  if (Ustrcmp(role, index_names[ixn]) == 0)
    error(((*role == 0)? 60 : 61), role);           /* Hard */
if (index_count >= INDEXMAX) error(55, INDEXMAX);   /* Hard */
index_names[index_count++] = role;

/* Scan the list for <indexterm> entries that match the role, and accumulate
the count of entries. We also insert a #INDEXDATA entry that remembers the
index number, the page number, and a text string for sorting. */

for (i = item_list; i != NULL && i != index_item; i = i->next)
  {
  indexstr *ix;
  unsigned int bp;
  BOOL hassee;

  if (Ustrcmp(i->name, "#FILENAME") == 0)
    {
    read_filename = i->p.string;
    continue;
    }

  /* Keep the page number up to date; ignore all other elements except
  <indexterm> for the correct role. */

  if (Ustrcmp(i->name, "#PDATA") == 0) { page++; continue; }
    else if (Ustrcmp(i->name, "indexterm") != 0) continue;

  p = misc_param_find(i, US"role");

  if ((p == NULL && *role != 0) ||
      (p != NULL && Ustrcmp(role, p->value) != 0))
    continue;

  read_linenumber = i->linenumber;
  hassee = FALSE;

  /* Process an end of range entry */

  p = misc_param_find(i, US"class");
  if (p != NULL && Ustrcmp(p->value, "endofrange") == 0)
    {
    p = misc_param_find(i, US"startref");
    if (p == NULL) { error(58); continue; }

    /* This seems like a lot of work, searching the whole item list, but
    it doesn't in practice take very long, so I'm not fussed about doing
    something faster such as retaining a list of <indexterm> ids as we go
    along. */

    for (ii = item_list; ii != i; ii = ii->next)
      {
      paramstr *pp;
      if (Ustrcmp(ii->name, "indexterm") != 0) continue;
      pp = misc_param_find(ii, US"id");
      if (pp != NULL && Ustrcmp(pp->value, p->value) == 0) break;
      }

    if (ii == i) error(59, p->value);
      else ii->next->p.ndxstr->endpage = page;
    continue;
    }

  /* Ignore any other entry that is its own partner */

  if (i->partner == i) continue;

  /* Increment the count of actual entries */

  index_entry_count++;

  /* Create the text string for sorting. Note that an <indexterm> may have
  not contain any text at all, and may be its own partner, though the common
  end-of-range case is handled above. But nevertheless, code defensively. */

  bp = 0;
  for (ii = i; ii != i->partner; ii = ii->next)
    {
    if (Ustrcmp(ii->name, "primary") == 0 ||
        Ustrcmp(ii->name, "secondary") == 0 ||
        Ustrcmp(ii->name, "tertiary") == 0)
      {
      textblock *tb;
      uschar *tp;

      for (iii = ii; iii != ii->partner; iii = iii->next)
        {
        if (Ustrcmp(iii->name, "#PCDATA") != 0) continue;
        tb = iii->p.txtblk;
        if (bp + tb->length + 4 > sizeof(buffer)) error(56);  /* Hard */

        /* As we copy the text, some characters may be omitted. They are
        listed in ascending order in index_sort_omit, terminated by 0xffffffff,
        which is greater than any possible character. */

        for (tp = tb->string; tp < tb->string + tb->length; )
          {
          unsigned int *ip;
          int length = 1;
          unsigned int c = GETCHARLEN(c, tp, length);
          for (ip = index_sort_omit;; ip++)
            {
            if (c == *ip) break;      /* Skip this character */
            if (c < *ip)              /* Include this character */
              {
              if (c < 256) buffer[bp++] = c; else
                {
                memcpy(buffer + bp, tp, length);
                bp += length;
                }
              break;
              }
            }
          tp += length;
          }
        }

      while (bp > 0 && isspace(buffer[bp-1])) bp--;
      buffer[bp++] = 0;    /* Marks end of this string */
      ii = ii->partner;
      }

    else if (Ustrcmp(ii->name, "see") == 0 ||
             Ustrcmp(ii->name, "seealso") == 0)
      {
      hassee = TRUE;
      ii = ii->partner;
      }
    }

  /* Fill in three zeros in case there are no prim/sec/ter elements */

  buffer[bp++] = 0;
  buffer[bp++] = 0;
  buffer[bp++] = 0;

  /* At this point we have the sorting text for the index line but it may still
  contain internal newlines (we removed any that were at the end of the items).
  When we copy it, we turn the newlines into spaces.

  Create the additional data structure and hang it off an #INDEXDATA item that
  we insert immediately after <indexterm>. */

  ix = misc_malloc(sizeof(indexstr) + bp);
  ix->ixnumber = ixn;
  ix->pagenumber = hassee? INT_MAX : page;
  ix->endpage = 0;

  /* There are three zero-terminated strings to copy */

  for (bp = 0, k = 0; k < 3; k++)
    {
    for (; buffer[bp] != 0; bp++)
      {
      if (buffer[bp] == '\n') ix->sorttext[bp] = ' ';
        else ix->sorttext[bp] = buffer[bp];
      }
    ix->sorttext[bp++] = 0;
    }

  /* Check whether we have more than one initial character in this index. */

  if (!manyinitials)
    {
    if (lastinitial == -1) lastinitial = ix->sorttext[0];
      else if (lastinitial != ix->sorttext[0]) manyinitials = TRUE;
    }

  /* Now create the entry. */

  ii = misc_malloc(sizeof(item));
  Ustrcpy(ii->name, "#INDEXDATA");
  ii->linenumber = i->linenumber;
  ii->partner = ii;
  ii->flags = 0;
  ii->p.ndxstr = ix;
  misc_insert_item(ii, i->next);
  }

read_linenumber = 0;

DEBUG(D_any) debug_printf("%4d entr%s for \"%s\" index\n", index_entry_count,
  (index_entry_count == 1)? "y" : "ies", index_names[ixn]);

DEBUG(D_indexfull)
  debug_print_item_list(item_list, "after identifying index entries");

/* If there are no index terms for this index, do no more, and kill the <index>
item. */

if (index_entry_count == 0)
  {
  DEBUG(D_any) debug_printf("Index not generated\n");
  index_item->prev->next = index_item->partner->next;
  index_item->partner->next->prev = index_item->prev;
  return rc;
  }

/* Create a vector of pointers for each of the index terms so that we can sort
them, also initializing the moving pointer for filling them. */

ivp = index_sort_vector = misc_malloc(sizeof(item *) * index_entry_count);

/* Rescan the item list and fill the vector. */

for (i = item_list; i != NULL && i != index_item; i = i->next)
  {
  if (Ustrcmp(i->name, "indexterm") != 0 || i->partner == i) continue;
  if (Ustrcmp(i->next->name, "#INDEXDATA") == 0 &&
      i->next->p.ndxstr->ixnumber == ixn) *ivp++ = i;
  }

/* Initialize the sorting collation data, and create the list of collation
values for inserting headings in the index. */

init_collation_data();

ixh = misc_malloc(sizeof(int) * (Ustrlen(ixhlist) + 1));
ixh[0] = -999;
for (k = 1; ixhlist[k] != 0; k++) ixh[k] = uncased_table[ixhlist[k]];
ixh[k] = +999;

/* Sort the vectors */

DEBUG(D_index) debug_printf("Sorting \"%s\" index\n", index_names[ixn]);
qsort(index_sort_vector, index_entry_count, sizeof(item *), ixcompare);

DEBUG(D_index|D_indexfull)
  {
  uschar *s;

  debug_printf("----- Sorted \"%s\" index -----\n", index_names[ixn]);
  for (k = 0; k < index_entry_count; k++)
    {
    i = index_sort_vector[k];
    s = i->next->p.ndxstr->sorttext;
    for (kk = 0; kk < 3; kk++)
      {
      int slen = Ustrlen(s);
      debug_printf("[");
      debug_print_string(s, slen, "] ");
      s += slen + 1;
      }
    debug_printf("%d", i->next->p.ndxstr->pagenumber);
    if (i->next->p.ndxstr->endpage != 0)
      debug_printf("-%d", i->next->p.ndxstr->endpage);
    debug_printf("\n");
    }
  debug_printf("----- End \"%s\" index -----\n", index_names[ixn]);
  }

/* We now want to insert into the item list, just before the <index/> item,
text items for the index itself, suitably marked-up. The data items are moved
from their positions in the chain in order to accomplish this, with page number
additions, etc., as necessary. */

read_addto = index_item->partner->prev;    /* Add at this point */
iprev = NULL;                              /* Previous <indexterm> */
lpage = 0;                                 /* Last output number */

(void)sprintf(CS buffer,
  "<?sdop page_columns=\"%d\" page_column_separation=\"%s\"?>",
    index_page_columns, misc_formatfixed(index_page_colsep));
(void)read_string(buffer, nest_stack, &nest_stackptr, 0);

for (k = 0; k < index_entry_count; k++)
  {
  int j;
  int plevel = 0;

  i = index_sort_vector[k];
  read_linenumber = i->linenumber;

  /* Find the primary, secondary, tertiary, and "see"/"seealso" elements. There
  is an error if we can't find a primary. */

  if (!find_psts(i, stn, &see)) { error(64); continue; }

  /* If this is not the very first index entry, compare this entry with the
  previous entry. If they are identical, just add the next page number and
  continue. Otherwise, terminate the previous entry, and set the level from
  which to start printing the next line. */

  if (iprev != NULL)
    {
    for (j = 0; j < 3; j++)
      {
      if (ixcompare_sorttext(i, iprev, j, TRUE) != 0) break;
      if (ixcompare_sorttext(i, iprev, j, FALSE) != 0) break;
      if (stn[j] != NULL && ixcompare_textfont(stn[j], prevstn[j]) != 0) break;
      }
    if (j >= 3)
      {
      lpage = set_ref(US", ",             /* Prefix with comma and space */
        i->next->p.ndxstr->pagenumber,    /* First page number */
        i->next->p.ndxstr->endpage,       /* Last page number, or 0 */
        lpage,                            /* Previously written page number */
        see,                              /* <see> item, if any */
        nest_stack, &nest_stackptr);      /* Nesting stack workspace */

      /* Mark all the processing parameters in the duplicate one as seen;
      otherwise an error will be generated at the end, because only the
      original is actually processed. */

      for (ii = i->next; ii != i->partner; ii = ii->next)
        {
        paramstr *pp;
        if (Ustrcmp(ii->name, "?sdop") != 0) continue;
        for (pp = ii->p.param; pp != NULL; pp = pp->next) pp->seen = TRUE;
        }

      continue;
      }
    (void)read_string(US"</para>", nest_stack, &nest_stackptr, 0);
    plevel = j;
    }

  /* Output a heading when we change initial characters, unless the entire
  index has only one initial character, or headings are disabled. */

  if (manyinitials && index_headings_enabled)
    {
    int fc;
    uschar *ptr = i->next->p.ndxstr->sorttext;
    GETCHAR(fc, ptr);
    fc = (fc <= max_col_char)? uncased_table[fc] : -1;
    if (fc >= ixh[ixhnext])
      {
      uschar title[12];
      while (fc >= ixh[ixhnext+1]) ixhnext++;
      if (ixhlist[ixhnext] == '*') Ustrcpy(title, "Symbols\n");
      else if (ixhlist[ixhnext] == '0') Ustrcpy(title, "Digits\n");
      else
        {
        title[0] = ixhlist[ixhnext];
        title[1] = '\n';
        title[2] = 0;
        }
      (void)read_string(US"<section><title>", nest_stack, &nest_stackptr, 0);
      (void)read_string(title, nest_stack, &nest_stackptr, 0);
      (void)read_string(US"</title></section>\n", nest_stack, &nest_stackptr, 0);
      ixhnext++;
      }
    }

  /* Output one or more new lines containing text for the appropriate levels,
  and add the current entry's page number(s). */

  for (j = plevel; j < 3; j++)
    {
    int jj;
    item *this = stn[j];

    if (this == NULL) break;              /* There's no text at this level */

    if (j > plevel)                       /* Round this loop more than once */
      (void)read_string(US"</para>", nest_stack, &nest_stackptr, 0);

    (void)read_string(US"<para>", nest_stack, &nest_stackptr, 0);
    for (jj = 0; jj < j; jj++)
      (void)read_string(US S_HARD_SPACE4, nest_stack, &nest_stackptr, 0);

    this->prev->next = this->partner->next;
    this->partner->next->prev = this->prev;

    read_addto->next->prev = this->partner;
    this->partner->next = read_addto->next;
    read_addto->next = this;
    this->prev = read_addto;
    read_addto = this->partner;
    }

  lpage = set_ref(US S_HARD_SPACE2,     /* Prefix with some space */
    i->next->p.ndxstr->pagenumber,      /* First page number */
    i->next->p.ndxstr->endpage,         /* Last page number, or 0 */
    0,                                  /* Previously written page number */
    see,                                /* <see> item, if any */
    nest_stack, &nest_stackptr);        /* Nesting stack workspace */

  memcpy(prevstn, stn, sizeof(stn));    /* Remember previous 3 texts */
  iprev = i;                            /* Remember <indexterm> */
  }

/* Terminate the final entry (if there is one), and revert to the previous
column settings. */

if (iprev != NULL)
  (void)read_string(US"</para>", nest_stack, &nest_stackptr, 0);

(void)sprintf(CS buffer,
  "<?sdop page_columns=\"%d\" page_column_separation=\"%s\"?>",
    page_columns_save, misc_formatfixed(page_colsep_save));
(void)read_string(buffer, nest_stack, &nest_stackptr, 0);

/* Tidy up the sort vector before returning */

misc_free(index_sort_vector, sizeof(item *) * index_entry_count);
read_what = NULL;
read_linenumber = 0;

DEBUG(D_indexfull)
  debug_print_item_list(item_list, "after creating index items");

return rc;
}

/* End of index.c */
