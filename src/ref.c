/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains code for resolving cross references. */


#include "sdop.h"


/*************************************************
*               Resolve references               *
*************************************************/

/* This function goes through the document and resolves references.

Argument:     the start of the items to be processed
Returns:      TRUE/FALSE
*/

BOOL
ref_resolve(item *item_list)
{
DEBUG(D_any) debug_printf("Resolving references\n");

for (item *i = item_list; i != NULL; i = i->next)
  {
  item *refitem;
  paramstr *p;
  textblock *tb;
  tree_node *tn;
  uschar *ref;

  if (Ustrcmp(i->name, "#FILENAME") == 0)
    {
    read_filename = i->p.string;
    continue;
    }
  read_linenumber = i->linenumber;

  if (Ustrcmp(i->name, "xref") != 0 &&
      Ustrcmp(i->name, "footnoteref") != 0) continue;

  /* We have an <xref> or <footnoteref> element; search for its "linkend"
  parameter. */

  for (p = i->p.param; p != NULL; p = p->next)
    { if (Ustrcmp(p->name, "linkend") == 0) break; }
  if (p == NULL) continue;

  /* Now see if the reference was set. */

  tn = tree_search(id_tree, p->value);
  if (tn == NULL)
    {
    (void)error(12, p->value);
    continue;
    }
  refitem = (item *)tn->data.ptr;

  /* Handle <footnoteref>. Like <footnote>, we want to remove any preceding
  newline, to avoid an unwanted space. Then insert a dummy reference key (which
  will be replaced later). */

  if (Ustrcmp(i->name, "footnoteref") == 0)
    {
    footnote_remove_newline(i);
    footnote_insert_reference(i, PIN_FNREFREF);
    }

  /* Handle <xref>. The referenced item should have a number parameter, called
  "#number". */

  else
    {
    p = misc_param_find(refitem, US"#number");
    if (p == NULL)
     {
     (void)error(13, refitem->name);
     ref = US"???";
     }
    else ref = p->value;

    /* Unless the reference is empty, which it can be for pathological figure
    and table references, create a new item to be inserted into the chain after
    the xref element. This contains the text of the reference. */

    if (ref[0] != 0)
      {
      item *new = misc_malloc(sizeof(item));
      new->partner = new;
      new->linenumber = i->linenumber;
      new->flags = 0;
      Ustrcpy(new->name, "#PCDATA");
      misc_insert_item(new, i->next);

      tb = misc_malloc(sizeof(textblock) + Ustrlen(ref));
      tb->next = NULL;
      tb->vfont = NULL;
      tb->pin_flags = PIN_XREF;
      tb->colour = xref_packed_colour;
      Ustrcpy(tb->string, ref);
      tb->length = Ustrlen(ref);
      new->p.txtblk = tb;
      }
    }
  }

read_linenumber = 0;

DEBUG(D_ref) debug_print_item_list(item_list, "after resolving references");

return TRUE;
}


/*************************************************
*         Set reference page numbers             *
*************************************************/

/* In order to generate pdfmark links in the PostScript, we need to know the
page numbers for each cross reference. This function is called just before
writing the output. It scans the entire text, keeping track of absolute page
numbers in the file (starting from 1, as pdfmark requires), and updating the
tree of cross references.

Arguments:   none
Returns:     TRUE if all has gone well, FALSE otherwise
*/

BOOL
ref_set_pages(void)
{
static item** list_of_lists[] =   /* These are the lists of items for */
  {                               /* different parts of the document. */
  &title_item_list,
  &toc_item_list,
  &preface_item_list,
  &main_item_list,
  NULL
  };

int pagenumber = 0;
int ypos = 0;

if (!xref_links) return TRUE;     /* Disabled */

for (item ***ipp = list_of_lists, **ip = *ipp++; ip != NULL; ip = *ipp++)
  {
  if (*ip == NULL) continue;      /* Empty list (not even a dummy header) */

  /* Start at the first #PDATA (page data) item (if present) */

  for (item *i = (*ip)->next; i != NULL; i = i->next)
    {
    paramstr *p;
    tree_node *tn;
    item *refitem;
    uschar *name = i->name;

    /* Keep track of absolute page numbers */

    if (Ustrcmp(name, "#PDATA") == 0)
      {
      pagenumber++;
      ypos = 0;
      continue;
      }

    /* Keep track of the vertical position on the page. */

    if (Ustrcmp(name, "#PCPARA") == 0)
      {
      outputline *ol;
      for (ol = i->p.prgrph->out; ol != NULL; ol = ol->next) ypos += ol->depth;
      continue;
      }

    /* Ignore terminators and specials and anything that does not have an
    "id" parameter. */

    if (name[0] == '/' || name[0] == '#') continue;
    p = misc_param_find(i, US"id");
    if (p == NULL) continue;

    /* Find the tree node for this id; if it does not exist, an error will
    already have been given, so just ignore it. */

    tn = tree_search(id_tree, p->value);
    if (tn == NULL) continue;
    refitem = (item *)tn->data.ptr;

    /* Add page number and vertical position parameters to the item pointed at
    by the tree node. */

    p = misc_malloc(sizeof(paramstr) + 8);
    p->seen = TRUE;
    Ustrcpy(p->name, "#pagenumber");
    sprintf(CS p->value, "%d", pagenumber);
    p->next = refitem->p.param;
    refitem->p.param = p;

    p = misc_malloc(sizeof(paramstr) + 8);
    p->seen = TRUE;
    Ustrcpy(p->name, "#pageyposition");
    sprintf(CS p->value, "%s",
      misc_formatfixed(page_full_length + margin_bottom - ypos));
    p->next = refitem->p.param;
    refitem->p.param = p;
    }
  }

return TRUE;
}

/* End of ref.c */
