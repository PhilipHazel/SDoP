/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains a number of miscellaneous small utility functions. */


#include "sdop.h"



/*************************************************
*               Malloc with check                *
*************************************************/

/* The program dies if the memory is not available. At the start of each block
we add a memchainstr to make a two-way chain. Blocks that are freed are taken
out of the chain; any that are left at the end are freed by an atexit()
function.

Argument:    size required
Returns:     pointer
*/

void *
misc_malloc(int size)
{
memchainstr *yield;
size += sizeof(memchainstr);
yield = malloc(size);
if (yield == NULL) (void)error(1, size);   /* Fatal error */
memory_used += size;
if (memory_used > memory_hwm) memory_hwm = memory_used;
yield->next = main_memory_chain;
yield->prev = NULL;
if (main_memory_chain != NULL) main_memory_chain->prev = yield;
main_memory_chain = yield;
return (void *)((char *)yield + sizeof(memchainstr));
}



/*************************************************
*                Free                            *
*************************************************/

/* Remove the block from the remembered chain before freeing it.

Argument:
  ptr       the pointer to free
  size      the amount being freed

Returns:    nothing
*/

void
misc_free(void *ptr, int size)
{
memchainstr *p = (memchainstr *)((char *)ptr - sizeof(memchainstr));

for (memchainstr *pp = main_memory_chain; pp != NULL; pp = pp->next)
  if (pp == p) break;

if (p == NULL) error(118);  /* Hard */

if (p->next != NULL) p->next->prev = p->prev;
if (p == main_memory_chain) main_memory_chain = p->next;
  else p->prev->next = p->next;

free(p);
memory_used -= size + sizeof(memchainstr);
}



/*************************************************
*        Find a file in the shared libraries     *
*************************************************/

/* Given a file name, this function searches the libraries given with -S (in
sdop_share) and also the inbuilt library for the first one that contains the
file. The complete file name is then returned.

Arguments:
  name         the name of the file
  buffer       where to put the FQN
  hard         TRUE if failure should force a hard error

Returns:       TRUE if file found, FALSE otherwise if hard == FALSE
*/

BOOL
misc_find_share(uschar *name, uschar *buffer, BOOL hard)
{
if (sdop_share != NULL)
  {
  uschar *s = sdop_share;
  while (s != NULL && *s != 0)
    {
    uschar *ss = Ustrchr(s, ':');
    if (ss != NULL) *ss = 0;
    (void)sprintf(CS buffer, "%s/%s", s, name);
    if (ss != NULL) *ss++ = ':';
    if (sys_exists(buffer)) return TRUE;
    s = ss;
    }
  }
(void)sprintf(CS buffer, "%s/%s", DATADIR, name);
if (!sys_exists(buffer))
  {
  if (!hard) return FALSE;
  if (sdop_share != NULL)
    error(69, name, sdop_share, DATADIR);  /* Hard */
  else
    error(70, name, DATADIR);  /* Hard */
  }
return TRUE;
}



/*************************************************
*          Set dummy item for head of list       *
*************************************************/

/* This function is called to create a dummy item for use at the start of the
various item lists.

Arguments:  none
Returns:    pointer to the item
*/

item *
misc_dummy_item(void)
{
item *yield = misc_malloc(sizeof(item));
yield->next = NULL;
yield->prev = NULL;
yield->partner = yield;
yield->linenumber = 0;
yield->flags = 0;
yield->name[0] = 0;
yield->p.param = NULL;
return yield;
}



/*************************************************
*          Insert an item into the chain         *
*************************************************/

/* This function inserts a single item before another. It is assumed that
there is always a previous item.

Arguments:
  new          the item
  old          the item to insert before

Returns:       nothing
*/

void
misc_insert_item(item *new, item *old)
{
new->prev = old->prev;
new->next = old;
old->prev->next = new;
old->prev = new;
}



/*************************************************
*          Insert an element pair                *
*************************************************/

/* This function is used to insert an element and its partner into a chain.
It is used when splitting up tables and when wrapping orphan text in a <para>
to avoid its being lost.

Arguments:
  name        the name of the element
  where       the element after which to insert

Returns:      the newly inserted element
*/

item *
misc_insert_element_pair(uschar *name, item *where)
{
item *n = misc_malloc(sizeof(item));
item *p = misc_malloc(sizeof(item));

n->prev = where;
n->next = p;
p->prev = n;

p->next = where->next;
if (where->next != NULL) where->next->prev = p;
where->next = n;

n->partner = p;
p->partner = n;

n->linenumber = p->linenumber = where->linenumber;
n->flags = p->flags = 0;
Ustrcpy(n->name, name);
Ustrcpy(p->name, "/");

n->p.param = p->p.param = NULL;

return n;
}



/*************************************************
*           Find a parameter for an item         *
*************************************************/

/* This function scans a list of parameters, looking for one by name.

Arguments:
   i           points to the item
   name        the name of the parameter

Returns:       pointer to the parameter, or NULL
*/

paramstr *
misc_param_find(item *i, uschar *name)
{
paramstr *p;
for (p = i->p.param; p != NULL; p = p->next)
  {
  if (Ustrcmp(p->name, name) == 0)
    {
    p->seen = TRUE;
    break;
    }
  }
return p;
}



/*************************************************
*           Find a title for an item             *
*************************************************/

/* Titles are always immediately after items, except that processing controls
and elements ending in "Info" or starting "#" may intervene. There may also be
otherwise empty newline text elements. We return the characters of the title in
a single lengthstring item. This function is called by toc_save_raw_ titles()
when it is saving the raw text so that it can be reprocessed later. See also
misc_find_rawtitle() below.

Arguments:
  i          the item (e.g. chapter, section)
  titlename  the name of the title ("title", "subtitle", or "titleabbrev")

Returns:     pointer to lengthstring item or NULL if no title found
*/

lengthstring *
misc_find_title(item *i, uschar *titlename)
{
lengthstring *ls;
uschar *p;
int count = 0;

if (i->partner == i) return NULL;  /* Pathological case */

/* Skip "info" and "#" elements */

for (i = i->next;; i = i->next)
  {
  if (i->name[0] == '?' || i->name[0] == '#') continue;
  if (Ustrlen(i->name) < 4 ||
      Ustrcmp(i->name + Ustrlen(i->name) - 4, "Info") != 0)
    break;
  i = i->partner;
  break;
  }

/* Seek title element */

for(;;)
  {
  if (i == NULL ||
      (Ustrcmp(i->name, "title") != 0 &&
       Ustrcmp(i->name, "subtitle") != 0 &&
       Ustrcmp(i->name, "titleabbrev") != 0 &&
       (Ustrcmp(i->name, "#PCDATA") != 0 ||
         i->p.txtblk->length != 1 ||
         i->p.txtblk->string[0] != '\n')
       ))
    return NULL;
  if (Ustrcmp(i->name, titlename) == 0) break;
  i = i->partner->next;
  }

for (item *ii = i->next; ii != i->partner; ii = ii->next)
  if (Ustrcmp(ii->name, "#PCDATA") == 0) count += ii->p.txtblk->length;

ls = misc_malloc(sizeof(lengthstring) + count);
p = ls->value;

for (item *ii = i->next; ii != i->partner; ii = ii->next)
  if (Ustrcmp(ii->name, "#PCDATA") == 0)
    {
    uschar *string = ii->p.txtblk->string;
    int length = ii->p.txtblk->length;

    /* The IF_NUMBERED flag is set if a chapter, section, or appendix number
    was inserted at the start of the title. In this case, we skip over digits,
    upper case letters, and dots, and then over the following white space. */

    if ((ii->flags & IF_NUMBERED) != 0)
      {
      while (isdigit(*string) || *string == '.' || isupper(*string))
        {
        string++;
        length--;
        }
      while (isspace(*string))
        {
        string++;
        length--;
        }
      }

    /* Build up the total string from all the #PCDATA items. */

    memcpy(p, string, length);
    p += length;
    }

/* Remove trailing space; in particular, there is often a newline. Set
the actual length before returning. */

while (p > ls->value && isspace(p[-1])) p--;

ls->length = p - ls->value;
*p = 0;
return ls;
}



/*************************************************
*         Find the raw title for an item         *
*************************************************/

/* The raw text of certain titles is saved in #RAWTITLE items early in the
processing so that they can be reprocessed for several different uses (e.g.
TOC, page footers). This function finds an item's raw title, which must follow
immediately, except for processing instructions and elements whose names end in
"Info".

Arguments:
  i          the item (e.g. chapter, section)
  titlename  the name of the item required ("#RAWTITLE", "#RAWSUBTITLE", ...)

Returns:     pointer to lengthstring item or NULL if no title found
*/

lengthstring *
misc_find_rawtitle(item *i, uschar *titlename)
{
for (i = i->next;; i = i->next)
  {
  if (i->name[0] == '?') continue;
  if (Ustrlen(i->name) < 4 ||
      Ustrcmp(i->name + Ustrlen(i->name) - 4, "Info") != 0)
    break;
  i = i->partner;
  }

for(;;)
  {
  if (i == NULL ||
      (Ustrcmp(i->name, "#RAWTITLE") != 0 &&
       Ustrcmp(i->name, "#RAWSUBTITLE") != 0 &&
       Ustrcmp(i->name, "#RAWTITLEABBREV") != 0))
    return NULL;
  if (Ustrcmp(i->name, titlename) == 0) return i->p.lngthstrng;
  i = i->next;
  }

/* Control cannot reach here */
}



/*************************************************
*        Check for convert-to-text element       *
*************************************************/

/* Only a very few elements convert into text that is part of what is being
typeset. They are treated specially because it is necessary to ensure that
spacing around them (including line breaks) does not get mangled. This function
is called while reading in the text.

Argument:  pointer after the initial '<' of an element
Returns:   TRUE if it's one of the specials
*/

BOOL
misc_istext_elname(uschar *s)
{
for (int k = 0; text_elements[k] != NULL; k++)
  {
  int len = Ustrlen(text_elements[k]);
  if (Ustrncmp(s, text_elements[k], len) == 0 &&
      (s[len] == ' ' || s[len] == '>'))
  return TRUE;
  }
return FALSE;
}



/*************************************************
*           Convert string to a number           *
*************************************************/

/* The argument is checked for consisting only of decimal digits.

Argument:  the string
Returns:   the number, or -1 on error
*/

int
misc_get_number(uschar *s)
{
int yield = 0;
while (isdigit(*s)) yield = yield * 10 + *s++ - '0';
return (*s == 0)? yield : -1;
}



/*************************************************
*     Read fixed point number and return end     *
*************************************************/

/*
Arguments:
  s         the string
  p         where to put the terminator pointer

Returns:    the number
*/

int
misc_get_fp(uschar *s, uschar **p)
{
int yield = 0;
while (isdigit(*s)) yield = yield * 10 + *s++ - '0';
yield *= 1000;

if (*s == '.')
  {
  int x = 100;
  s++;
  while (isdigit(*s))
    {
    yield += (*s++ - '0') * x;
    x /= 10;
    }
  }

*p = s;
return yield;
}


/*************************************************
*      Scale a number according to a dimension   *
*************************************************/

/*
Arguments:
  fp        the fixed-point number
  s         the following string, giving the units

Returns:    the scaled number, or -1 on error
*/

int
misc_scale_number(int fp, uschar *s)
{
if (*s == 0 || Ustrcmp(s, "pt") == 0) return fp;
if (Ustrcmp(s, "pc") == 0) return fp * 10;
if (Ustrcmp(s, "in") == 0) return fp * 72;
if (Ustrcmp(s, "mm") == 0) return MUL(fp, 2835);
if (Ustrcmp(s, "cm") == 0) return MUL(fp, 28346);
return -1;
}



/*************************************************
*          Convert string to a dimension         *
*************************************************/

/* The argument is checked for consisting only of decimal digits, optionally
followed a decimal fraction, and then by one of the standard abbreviations: pt,
pc, in, mm.

Argument:  the string
Returns:   the dimension, or -1 on error
*/

int
misc_get_dimension(uschar *s)
{
int yield = misc_get_fp(s, &s);
return misc_scale_number(yield, s);
}




/*************************************************
*        Read several dimensions from a string   *
*************************************************/

/* This function reads a given number of comma-separated dimensions.
They are allowed to be negative.

Arguments:
  n            the number of dimensions
  s            the string
  iptr         pointer to a vector of ints
  give_error   if TRUE, generate an error message for syntax errors

Returns:       TRUE if all went well
*/

BOOL
misc_get_dimensions(int n, uschar *s, int *iptr, BOOL give_error)
{
uschar *sorig = s;

for (int k = 0; k < n; k++)
  {
  int x;
  int sign = 1;

  if (*s == '-')
    {
    sign = -1;
    s++;
    }

  x = misc_get_fp(s, &s);
  if (*s != 0 && *s != ',')
    {
    int c;
    uschar *ss = s;
    while (*ss != 0 && *ss != ',') ss++;
    c = *ss;
    *ss = 0;
    x = misc_scale_number(x, s);
    *ss = c;
    s = ss;
    if (x < 0)
      {
      if (give_error) return error(51, "dimension", sorig);
        else return FALSE;
      }
    }

  iptr[k] = x * sign;

  if ((k == n-1 && *s != 0) ||
      (k != n-1 && *s != ','))
    {
    if (give_error) return error(95, n, sorig);
      else return FALSE;
    }
  s++;
  }

return TRUE;
}



/*************************************************
*       Convert character value to UTF-8         *
*************************************************/

/* This function takes an integer value in the range 0 - 0x7fffffff
and encodes it as a UTF-8 character in 1 to 6 bytes.

Arguments:
  cvalue     the character value
  buffer     pointer to buffer for result - at least 6 bytes long

Returns:     number of characters placed in the buffer
*/

int
misc_ord2utf8(int cvalue, uschar *buffer)
{
int i, j;
for (i = 0; i < 6; i++) if (cvalue <= utf8_table1[i]) break;
buffer += i;
for (j = i; j > 0; j--)
 {
 *buffer-- = 0x80 | (cvalue & 0x3f);
 cvalue >>= 6;
 }
*buffer = utf8_table2[i] | cvalue;
return i + 1;
}



/*************************************************
*        Format a number in Roman numerals       *
*************************************************/

/* Currently, it always uses lower case. A terminating zero is included.

Arguments:
  s          where to put the result
  n          the number

Returns:     number of characters placed in the buffer, excluding zero
*/

static uschar *romtable = US
  "    i   ii  iii iv  v   vi  vii viiiix  "
  "    x   xx  xxx xl  l   lx  lxx lxxxxc  "
  "    c   cc  ccc cd  d   dc  dcc dccccm  "
  "    m   mm  mmm mmmm?   ?   ?   ?    ?  ";

int
misc_roman(uschar *s, int n)
{
uschar buffer[24];
uschar *t = buffer;
int count = 0;
int offset = 40 * sprintf(CS buffer, "%d", n);

while (*t != 0)
  {
  int i, p;
  offset -= 40;
  p = 4*(*t++ - '0') + offset;
  for (i = 0; i < 4; i++)
    {
    int ch = romtable[p+i];
    if (ch == ' ') break;
    *s++ = ch;
    count++;
    }
  }

*s = 0;
return count;
}



/*************************************************
*       Format a number in alphabetic style      *
*************************************************/

/* Currently, it always uses lower case.

Arguments:
  s          where to put the result
  n          the number

Returns:     number of characters placed in the buffer
*/

static const char *alphabet = "abcdefghijklmnopqrstuvwxyz";

int
misc_alpha(uschar *s, int n)
{
int count = 0;
int d = 26*26*26;

while (d > 0)
  {
  int c = n/d;
  int r = n%d;
  if (c != 0)
    {
    *s++ = alphabet[c-1];
    count++;
    }
  n = r;
  d /= 26;
  }

*s = 0;
return count;
}



/*************************************************
*            Format a fixed-point number         *
*************************************************/

/* Called from the debug functions, and from the output writing and index
creation functions.

Argument:
  fn         an int containing a fixed-point number (usually millipoints)

Returns:     pointer to the formatted string
*/

char *
misc_formatfixed(int fn)
{
static uschar buffer[32];
uschar *p = buffer;
int n, d;

if (fn < 0)
  {
  *p++ = '-';
  fn = -fn;
  }

n = fn/1000;
d = fn%1000;

p += sprintf(CS p, "%d", n);

if (d != 0)
  {
  p += sprintf(CS p, ".%03d", d);
  while (p[-1] == '0') p--;
  *p = 0;
  }

return CS buffer;
}



/*************************************************
*         Set a vector of yes/no values          *
*************************************************/

/* This function is called to handle parameters whose value is a list of yes/no
strings, for example, those that select which sections are to be numbered or
included in the TOC. If the list is short, the remaining strings are taken as
"no".

Arguments:
  i          the ?sdop item
  pname      the parameter name
  vector     the vector
  size       the size of the vector

Returns:     TRUE if the parameter was found
*/

BOOL
misc_yesno_vector(item *i, uschar *pname, BOOL *vector, int size)
{
int ii;
uschar *s;
paramstr *p = misc_param_find(i, pname);
if (p == NULL) return FALSE;
ii = 0;
s = p->value;
while (*s != 0)
  {
  uschar *ss;
  int term;
  while (isspace(*s)) s++;
  for (ss = s; *ss != 0 && *ss != ',' && !isspace(*ss); ss++);
  term = *ss;
  *ss = 0;
  if (Ustrcmp(s, "yes") == 0)
    { if (ii < size) vector[ii++] = TRUE; }
  else if (Ustrcmp(s, "no") == 0)
    { if (ii < size) vector[ii++] = FALSE; }
  else (void) error(107, s);
  *ss = term;
  s = ss;
  if (*s == ',') s++;
  }
while (ii < size) vector[ii++] = FALSE;
return TRUE;
}



/*************************************************
*         Interpret a colour specification       *
*************************************************/

/* This function recognizes r,g,b triples.

Arguments:
  s         the string to interpret
  cp        pointer to int[3] for the RGB  values

Returns:    TRUE on success
*/

BOOL
misc_get_colour(uschar *s, int *cp)
{
double d[3];
if (sscanf(CS s, "%lf,%lf,%lf", &d[0], &d[1], &d[2]) != 3)
  return error(93);

if (d[0] < 0.0 || d[0] > 1.0 ||
    d[1] < 0.0 || d[1] > 1.0 ||
    d[2] < 0.0 || d[2] > 1.0)
  return error(94);

cp[0] = (int)(d[0]*1000);
cp[1] = (int)(d[1]*1000);
cp[2] = (int)(d[2]*1000);
return TRUE;
}


/* End of misc.c */
