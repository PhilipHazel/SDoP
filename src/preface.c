/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module handles initial processing of prefaces. */

#include "sdop.h"


/*************************************************
*           Do preface processing                *
*************************************************/

/* If there is a preface (there may be more than one), cut it (them) out from
the main item list and process it (them) on their own. The first <preface> item
that was encountered is set in preface_item_list.

As well as cutting out the preface(s) we need also to include any prior
processing instructions so that they are used for the preface. We have to do
the preface separately in order to eventually print it with roman page numbers,
following on from the TOC. Otherwise, it could have been done in-line with the
body.

Arguments:   none
Returns:     TRUE
*/

BOOL
preface_process(void)
{
item *ep;
item *pi = NULL;
item *pilast = NULL;

if (preface_item_list == NULL)
  {
  DEBUG(D_any) debug_printf("No preface found\n");
  return TRUE;
  }

/* Get a list of processing instructions. */

for (item *i = main_item_list; i != preface_item_list; i = i->next)
  {
  item *ii;
  if (Ustrcmp(i->name, "?sdop") != 0) continue;
  ii = misc_malloc(sizeof(item));
  *ii = *i;
  ii->next = NULL;
  if (pi == NULL)
    {
    pi = ii;
    ii->prev = NULL;
    }
  else
    {
    pilast->next = ii;
    ii->prev = pilast;
    }
  pilast = ii;
  }

/* Find the end of the preface(s) */

ep = preface_item_list->partner;
for(;;)
  {
  item *pp = ep->next;
  while (pp != NULL && pp->name[0] == '?') pp = pp->next;
  if (pp == NULL || Ustrcmp(pp->name, "preface") != 0) break;
  ep = pp->partner;
  }

/* Cut the preface(s) out of the main chain */

preface_item_list->prev->next = ep->next;
if (ep->next != NULL) ep->next->prev = preface_item_list->prev;
ep->next = NULL;

/* Create the preface chain */

ep = misc_dummy_item();
if (pi == NULL)
  {
  ep->next = preface_item_list;
  preface_item_list->prev = ep;
  }
else
  {
  ep->next = pi;
  pi->prev = ep;
  pilast->next = preface_item_list;
  preface_item_list->prev = pilast;
  }

/* Process the preface */

preface_item_list = ep;
return page_format(preface_item_list, NULL, preface_even_pages, FALSE,
  &preface_page_count, US"preface");
}

/* End of preface.c */
