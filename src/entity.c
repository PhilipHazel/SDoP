/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains code for scanning the input document and replacing
entities with their data values. */


#include "sdop.h"


static uschar utf8[8];   /* Returned with UTF-8 string */



/*************************************************
*         Find the value of a named entity       *
*************************************************/

/* This function is called by entity_find() below, and also by pin_cond() when
checking whether or not an entity exists.

Arguments:
  attname      the entity name
  vptr         where to put the value ("" if not found)
  quiet        TRUE if no error for not found
  where        string for errors

Returns:       nothing
*/

void
entity_find_byname(uschar *attname, uschar **vptr, BOOL quiet, uschar *where)
{
tree_node *tn;
entity_block *bot, *mid, *top;
ventity_block *vbot, *vmid, *vtop;

/* Search the tree of dynamic entities. */

tn = tree_search(entity_tree, attname);
if (tn != NULL)
  {
  *vptr = tn->data.ptr;
  return;
  }

/* Not found in tree; search the static list by binary chop. A replacement
that starts with "&#x" is re-interpreted as a hex character. */

bot = entity_list;
top = entity_list + entity_list_count;

while (top > bot)
  {
  int c;
  mid = bot + (top - bot)/2;
  c = Ustrcmp(mid->name, attname);
  if (c == 0) break;
  if (c < 0) bot = mid + 1; else top = mid;
  }

if (top > bot)
  {
  if (Ustrncmp(mid->value, "&#x", 3) == 0)
    {
    char *t2;
    unsigned int longvalue = strtoul(CS(mid->value + 3), &t2, 16);
    utf8[misc_ord2utf8(longvalue, utf8)] = 0;
    *vptr = utf8;
    }
  else *vptr = mid->value;
  return;
  }

/* Not found in the static list; search the variable list by binary chop. */

vbot = ventity_list;
vtop = ventity_list + ventity_list_count;

while (vtop > vbot)
  {
  int c;
  vmid = vbot + (vtop - vbot)/2;
  c = Ustrcmp(vmid->name, attname);
  if (c == 0) break;
  if (c < 0) vbot = vmid + 1; else vtop = vmid;
  }

if (vtop > vbot)
  {
  *vptr = (*(vmid->value) == NULL)? US"" : *(vmid->value);
  }
else
  {
  *vptr = US"";
  if (!quiet) (void)error(16, attname, where);
  }

return;
}



/*************************************************
*             Find one entity value              *
*************************************************/

/* This function is called from entity_expand_one() when an & character is
encountered. It reads the entity and finds its value. Because each line is
processed twice - the first time to find out the size of the expanded line, the
second to do the deed - this function is called twice for each entity. To avoid
duplicating error messages, an argument indicates which pass is in progress.

This function is also called from toc_make() when creating strings from chapter
and section titles for use in pdfmark items. It is also called when creating
title pages and head and foot lines that contain entities. Other uses are for
handling an entity used as the character in table "char" alignments, and for
dealing with entities in index sorting skip strings.

Arguments:
  p          points after the initial '&'
  vptr       pointer to where to put the pointer to the value
  pass2      TRUE for second pass (no error messages)
  where      string for errors

Returns:     pointer after the final ';'
*/

uschar *
entity_find(uschar *p, uschar **vptr, BOOL pass2, uschar *where)
{
size_t alen;
uschar *pn, *pp;
uschar attname[32];

pp = p - 1;    /* Point to '&' for error messages */

/* Handle a numeric entity */

if (*p == '#')
  {
  uschar *endptr;
  int base = 10;

  if (*(++p) == 'x')
    {
    base = 16;
    p++;
    }

  /* This mess is to avoid a warning about type-punned variables from gcc */
    {
    unsigned long longvalue;
    char *t1 = CS p;
    char *t2;
    longvalue = strtoul(t1, &t2, base);
    utf8[misc_ord2utf8(longvalue, utf8)] = 0;
    *vptr = utf8;
    endptr = US t2;
    }

  /* Error for missing semicolon */

  p = endptr;
  if (*p != ';')
    { if (!pass2) (void)error(14, endptr - pp, pp); }
  else p++;
  return p;
  }

/* Handle named entities. Some may be defined in the document - they are put
into a tree. There are also two built-in lists - one for static values, and one
for variables. */

pn = p;

/* The syntax of XML names is that they must start with a letter, underscore,
or colon, and continue with alphanumerics, underscores, colons, hyphens, and
dots. (I eventually found this on a web page after much searching.) Because we
assume we are dealing with valid XML, the check here is weaker than a full
check would be. */

while (isalnum(*p) || *p == '_' || *p == ':' || *p == '.' || *p == '-') p++;
if (*p == ';')
  alen = p++ - pn;
else
  {
  if (!pass2) (void)error(15, p - pp, pp);   /* Missing semicolon */
  alen = p - pn;
  }

/* Grumble and truncate an overlong name; then put it in a buffer. */

if (alen > sizeof(attname) - 1)
  {
  (void)error(17, alen, pn);
  alen = sizeof(attname) - 1;
  }

Ustrncpy(attname, pn, alen);
attname[alen] = 0;

/* Search for the named entity and set up its value. */

entity_find_byname(attname, vptr, pass2, where);
return p;
}



/*************************************************
*           Expand entities in a string          *
*************************************************/

/* This function goes through a string, expanding any entities that are
encoded as &name; or &#number; or &#xhexnumber;. It scans the string twice;
the first pass finds out how much memory is needed. If necessary, the string is
moved to a new block. In practice, this should be fairly rare.

Argument:    the anchor of the text block
Returns:     TRUE if OK, FALSE on error

At present, always returns TRUE. Errors provoke messages from entity_find(),
but we always continue. */

static BOOL
entity_expand_one(textblock **tbanchor)
{
BOOL found = FALSE;
textblock *tb = *tbanchor;
uschar *p, *stringend;
int spare = 0;

/* First pass: Scan the string for & characters, and compute the number of
spare bytes we will have after conversion. */

for (p = tb->string; *p != 0;)
  {
  uschar *value;
  uschar *pp = p;
  if (*p++ != '&') continue;
  found = TRUE;
  p = entity_find(p, &value, FALSE, US"");
  spare += p - pp - Ustrlen(value);
  }

/* We can skip the second pass if no '&' characters were found. */

if (!found) return TRUE;

/* If we have a negative number of spare bytes, we need to get a new chunk
of memory and copy the text block into it so that it can be expanded. */

if (spare < 0)
  {                /* Don't amalgamate the first two statements, as it causes */
  spare = -spare;  /* a runtime warning converting negative to unsigned. */
  textblock *newtb = misc_malloc(sizeof(textblock) + tb->length + spare);
  newtb->next = tb->next;    /* At this stage, will in fact always be NULL */
  newtb->vfont = tb->vfont;
  newtb->pin_flags = tb->pin_flags;
  newtb->colour = tb->colour;
  newtb->length = tb->length;
  memcpy(newtb->string, tb->string, tb->length + 1);
  *tbanchor = newtb;
  misc_free(tb, sizeof(textblock) + tb->length);
  tb = newtb;
  }

/* At this point we know we have enough space in the string buffer to do the
expansions. */

stringend = tb->string + tb->length;
for (p = tb->string; *p != 0;)
  {
  int nlen;
  uschar *value;
  uschar *pp = p;
  if (*p++ != '&') continue;
  p = entity_find(p, &value, TRUE, US"");
  nlen = Ustrlen(value);
  memmove(pp + nlen, p, stringend - p + 1);
  stringend += nlen - (p - pp);
  tb->length += nlen - (p - pp);
  Ustrncpy(pp, value, nlen);
  p = pp + nlen;
  }

return TRUE;
}



/*************************************************
*             Scan items and expand entities     *
*************************************************/

/* This function scans the input document for text items and passes each chain
of texts to the entity expansion function.

Argument:      the first item in the list to be processed
Returns:       TRUE to continue processing; FALSE to stop
*/

BOOL
entity_expand(item *item_list)
{
BOOL yield = TRUE;

DEBUG(D_any) debug_printf("Expanding entities\n");

#if 0
DEBUG(D_entity) debug_print_item_list(item_list, "before expanding entities");
#endif

for (item *i = item_list; i != NULL; i = i->next)
  {
  read_linenumber = i->linenumber;
  if (Ustrcmp(i->name, "#PCDATA") == 0 &&
      !entity_expand_one(&(i->p.txtblk)))
    yield = FALSE;
  else if (Ustrcmp(i->name, "#FILENAME") == 0)
    read_filename = i->p.string;
  }
read_linenumber = 0;

DEBUG(D_entity) debug_print_item_list(item_list, "after expanding entities");
return yield;
}

/* End of entity.c */
