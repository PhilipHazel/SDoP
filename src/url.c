/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains code for doing things to URLs. */

#include "sdop.h"


/*************************************************
*          Check URL text for being the URL      *
*************************************************/

/* The <ulink> element defines the URL, and then text may follow before we get
to </ulink>. If the text is the same as the URL, we don't want to print the URL
as well. But if it's different, we want to show the URL afterwards. This
function sorts that out.

Argument:  the item list to scan
Returns:   TRUE
*/

BOOL
url_check(item *item_list)
{
for (item *i = item_list; i != NULL; i = i->next)
  {
  item *ii;
  int ulen;
  paramstr *p;
  textblock *t;

  if (Ustrcmp(i->name, "ulink") != 0) continue;
  p = misc_param_find(i, US"url");
  if (p == NULL) continue;
  ulen = Ustrlen(p->value);

  /* Find the last block of text before </ulink> */

  for (ii = i->partner; ii != i; ii = ii->prev)
    if (Ustrcmp(ii->name, "#PCDATA") == 0) break;

  /* If there is no text, create some, without parentheses. If the next input
  is on a different line, insert a newline at the end. */

  if (ii == i)
    {
    t = misc_malloc(sizeof(textblock) + ulen + 1);
    t->next = NULL;
    t->vfont = NULL;
    t->pin_flags = 0;
    t->colour = 0;
    t->length = ulen;
    Ustrcpy(t->string, p->value);

    if (i->linenumber != i->partner->next->linenumber)
      {
      t->length++;
      Ustrcpy(t->string + ulen, "\n");
      }

    ii = misc_malloc(sizeof(item));
    ii->partner = ii;
    ii->linenumber = i->linenumber;
    ii->flags = 0;
    Ustrcpy(ii->name, "#PCDATA");
    ii->p.txtblk = t;
    misc_insert_item(ii, i->next);
    }

  /* Otherwise, if the text (without any terminating white space) does not
  match the URL, add the URL in parentheses to the end of it, but before any
  terminating white space. */

  else
    {
    uschar *s;
    int slen;

    t = ii->p.txtblk;
    s = t->string;
    slen = Ustrlen(s);

    while (slen > 0 && isspace(s[slen-1])) slen--;

    if (slen != Ustrlen(p->value) || Ustrncmp(t->string, p->value, slen) != 0)
      {
      uschar *ss;
      int endspacelen = 0;
      textblock *tn = misc_malloc(sizeof(textblock) + t->length + ulen + 3);

      ss = t->string + t->length;
      while (ss > t->string && isspace(ss[-1])) { ss--; endspacelen++; }

      *tn = *t;
      tn->length += ulen + 3;
      s = tn->string;

      memcpy(s, t->string, t->length - endspacelen);
      s += t->length - endspacelen;

      *s++ = ' ';
      *s++ = '(';
      memcpy(s, p->value, ulen);
      s += ulen;
      *s++ = ')';

      memcpy(s, t->string + t->length - endspacelen, endspacelen);
      s += endspacelen;

      *s = 0;
      ii->p.txtblk = tn;
      }
    }

  i = i->partner;
  }

#if 0
debug_print_item_list(item_list, "after URL manipulation");
#endif

return TRUE;
}

/* End of url.c */
