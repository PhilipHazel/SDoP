/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains functions related to footnotes. */

#include "sdop.h"



/*************************************************
*      Remove newline before footnote[ref]       *
*************************************************/

/* This is called for both <footnote> and <footnoteref>. If the input text is
like this, with <footnote> on a new line:

  rhubarb rhubarb rhubarb
  <footnote>
  ....
  </footnote>
  rhubarb rhubarb rhubarb

then there will be a newline just before the <footnote>, and this will turn
into a space before the footnote key that we insert. This we don't want.
Rather than insist that <footnote> or <footnoteref> always appears right
against the previous word, and on the same line, we look backwards and remove
an immediately preceding newline, skipping over any closing elements (e.g.
</emphasis>) that may intervene.

Argument:    the <footnote[ref]> item
Returns:     nothing
*/

void
footnote_remove_newline(item *i)
{
for (item *id = i->prev; id != NULL; id = id->prev)
  {
  if (Ustrcmp(id->name, "/") == 0) continue;
  if (Ustrcmp(id->name, "#PCDATA") == 0)
    {
    textblock *tb = id->p.txtblk;
    if (tb->length > 0 && tb->string[tb->length-1] == '\n')
      tb->string[--(tb->length)] = 0;
    }
  break;
  }
}



/*************************************************
*       Insert a footnote reference key          *
*************************************************/

/* This function is called for <footnote> and <footnoteref>. It inserts a dummy
reference key.

Arguments:    the <footnote[ref]> item
              a flag discriminating <footnote> from <footnoteref>:
                either PIN_FNKEYREF or PIN_FNREFREF

Returns:      nothing
*/

void
footnote_insert_reference(item *i, unsigned int flag)
{
item *ii;
textblock *tb;

tb = misc_malloc(sizeof(textblock) + 3);  /* 3 digits seems ample! */
tb->next = tb->lastin = NULL;
tb->pin_flags = flag | PIN_SUPERSCRIPT;
tb->colour = 0;
tb->length = 1;
Ustrcpy(tb->string, "0");

ii = misc_malloc(sizeof(item));
ii->partner = ii;
ii->linenumber = i->linenumber;
ii->flags = 0;
Ustrcpy(ii->name, "#PCDATA");
ii->p.txtblk = tb;
misc_insert_item(ii, i);
}



/*************************************************
*                 Insert key texts               *
*************************************************/

/* Scan the chain for <footnote> items, and for each, insert a reference key
and a definition key. At this stage, we insert "0" for each key. Later, when
the text is paginated, this will be replaced by a digit in the range 1-9, which
will have the same width.

Argument:    the start of the item chain
Returns:     TRUE if all is well
*/

BOOL
footnote_insert_keys(item *item_list)
{
item *id;

DEBUG(D_any) debug_printf("Scanning for footnotes\n");

for (item *i = item_list; i != NULL; i = i->next)
  {
  item *idnext;

  if (Ustrcmp(i->name, "footnote") != 0) continue;

  /* Remove a preceding newline, to avoid an unwanted space. */

  footnote_remove_newline(i);

  /* Find the first text in the footnote, or the end of the footnote, but
  ignore a leading newline text. In fact, do more than ignore it: move it to
  after the footnote. This happens when <footnote> is the last thing on a line.
  The newline really belongs with the outer text. */

  for (id = i; id != i->partner; id = idnext)
    {
    idnext = id->next;
    if (Ustrcmp(id->name, "#PCDATA") != 0) continue;
    if (id->p.txtblk->length == 1 && id->p.txtblk->string[0] == '\n')
      {
      id->prev->next = id->next;
      id->next->prev = id->prev;
      misc_insert_item(id, i->partner->next);
      }
    else break;
    }

  if (id == i->partner) error(71); else
    {
    item *ii;
    textblock *tb;

    /* Insert a definition key. */

    tb = misc_malloc(sizeof(textblock) + 3);
    tb->next = tb->lastin = NULL;
    tb->pin_flags = PIN_FNKEYDEF | PIN_SUPERSCRIPT;
    tb->colour = 0;
    tb->length = 1;
    Ustrcpy(tb->string, "0");

    ii = misc_malloc(sizeof(item));
    ii->partner = ii;
    ii->linenumber = i->linenumber;
    ii->flags = 0;
    Ustrcpy(ii->name, "#PCDATA");
    ii->p.txtblk = tb;
    misc_insert_item(ii, id);

    /* Insert a reference key. */

    footnote_insert_reference(i, PIN_FNKEYREF);
    }

  /* Continue after the footnote */

  i = i->partner;
  }

return TRUE;
}

/* End of footnote.c */
