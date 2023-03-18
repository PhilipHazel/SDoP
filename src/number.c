/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains code for scanning the input document and numbering
titles that needs to be numbered. This must be done before paragraphs are
identified. We know that everything is nested correctly. */

#include "sdop.h"


/*************************************************
*             Scan items and number titles       *
*************************************************/

/* This function scans the input document and numbers appropriate titles.

Argument:      the first item in the list to be processed
Returns:       TRUE to continue processing; FALSE to stop
               (currently always TRUE)
*/

BOOL
number_titles(item *item_list)
{
BOOL oktitle = FALSE;
int depth = -1;
int example_number = 0;
int figure_number = 0;
int table_number = 0;
uschar *chapter_number = US"";
uschar buffer[256];

DEBUG(D_any) debug_printf("Numbering titles\n");

for (item *i = item_list; i != NULL; i = i->next)
  {
  /* If we hit a processing instruction, adjust numbering requirements if
  that's what it does. */

  if (Ustrcmp(i->name, "?sdop") == 0)
    {
    (void)misc_yesno_vector(i, US"numbertitles", number_sections, MAXSECTDEPTH);
    pin_figtab_format_changes(i);
    continue;
    }

  /* When we hit the end of a chapter, section, or appendix, decrease the
  nesting depth for checking whether numbers are wanted. For safety, turn off
  the flag that says a changeable title is expected. */

  if (Ustrcmp(i->name, "/") == 0 &&
     (ISSECT(i->partner->name) ||
      Ustrcmp(i->partner->name, "chapter")  == 0 ||
      Ustrcmp(i->partner->name, "appendix") == 0))
    {
    depth--;
    oktitle = FALSE;
    continue;
    }

  /* When we hit the start of a chapter, section, or appendix, find its number,
  set up extra text that follows an inserted number, and increase the nesting
  depth. Set oktitle TRUE if titles for this depth of nesting are to be
  numbered. If the nesting is too deep, assume FALSE.

  NOTE: Sections in a Preface are not numbered so won't have a #number
  attribute. */

  if (ISSECT(i->name) ||
      Ustrcmp(i->name, "chapter")  == 0 ||
      Ustrcmp(i->name, "appendix") == 0)
    {
    paramstr *p = misc_param_find(i, US"#number");

    depth++;

    if (p != NULL)
      {
      if (i->name[0] != 's')
        {
        chapter_number = p->value;    /* For figures and tables */
        (void)sprintf(CS buffer, "%s. ", p->value);
        }
      else
        {
        (void)sprintf(CS buffer, "%s ", p->value);
        }
      oktitle = (depth < MAXSECTDEPTH)? number_sections[depth] : FALSE;
      }
    else oktitle = FALSE;

    if (i->name[0] != 's')
      {
      if (figure_nformat_pcount >= 2) figure_number = 0;
      if (table_nformat_pcount >= 2) table_number = 0;
      }
    continue;
    }

  /* Handle figures. Unlike chapters etc, the numbers are not generated at
  read-in time. The format can change as we go through (see
  pin_figtab_format_changes() above). We have to generate a #number attribute
  for use in any later cross-references, so it can be used here as it is for
  chapters etc. There is a separate format string that allows for the insertion
  of the word "Figure", if required, and punctuation following the number. */

  if (Ustrcmp(i->name, "figure") == 0)
    {
    paramstr *p;
    uschar fnbuff[16];
    uschar ffbuff[16];

    (void)sprintf(CS fnbuff, "%d", ++figure_number);

    /* Some versions of gcc give warnings if a non-literal format string has
    no format arguments; for that reason, we give an argument in case 0, even
    though we know it is not needed. */

    switch(figure_nformat_pcount)
      {
      case 0:
      (void)sprintf(CS ffbuff, CS figure_number_format, fnbuff);
      break;

      case 1:
      (void)sprintf(CS ffbuff, CS figure_number_format, fnbuff);
      break;

      default:
      (void)sprintf(CS ffbuff, CS figure_number_format, chapter_number, fnbuff);
      break;
      }

    p = misc_malloc(sizeof(paramstr) + Ustrlen(fnbuff));
    Ustrcpy(p->name, "#number");
    Ustrcpy(p->value, ffbuff);
    p->next = i->p.param;
    p->seen = TRUE;
    i->p.param = p;

    (void)sprintf(CS buffer, CS figure_title_format, ffbuff);

    oktitle = TRUE;
    continue;
    }

  /* Handle tables. Unlike chapters etc, the numbers are not generated at
  read-in time. The format can change as we go through (see
  pin_figtab_format_changes() above). We have to generate a #number attribute
  for use in any later cross-references, so it can be used here as it is for
  chapters etc. There is a separate format string that allows for the insertion
  of the word "Table", if required, and punctuation following the number. */

  if (Ustrcmp(i->name, "table") == 0)
    {
    paramstr *p;
    uschar fnbuff[16];
    uschar ffbuff[16];

    (void)sprintf(CS fnbuff, "%d", ++table_number);

    /* Some versions of gcc give warnings if a non-literal format string has
    no format arguments; for that reason, we give an argument in case 0, even
    though we know it is not needed. */

    switch(table_nformat_pcount)
      {
      case 0:
      (void)sprintf(CS ffbuff, CS table_number_format, fnbuff);
      break;

      case 1:
      (void)sprintf(CS ffbuff, CS table_number_format, fnbuff);
      break;

      default:
      (void)sprintf(CS ffbuff, CS table_number_format, chapter_number, fnbuff);
      break;
      }

    p = misc_malloc(sizeof(paramstr) + Ustrlen(fnbuff));
    Ustrcpy(p->name, "#number");
    Ustrcpy(p->value, ffbuff);
    p->next = i->p.param;
    p->seen = TRUE;
    i->p.param = p;

    (void)sprintf(CS buffer, CS table_title_format, ffbuff);

    oktitle = TRUE;
    continue;
    }

  /* Handle examples. Unlike chapters etc, the numbers are not generated at
  read-in time. The format can change as we go through (see
  pin_figtab_format_changes() above). We have to generate a #number attribute
  for use in any later cross-references, so it can be used here as it is for
  chapters etc. There is a separate format string that allows for the insertion
  of the word "Example", if required, and punctuation following the number. */

  if (Ustrcmp(i->name, "example") == 0)
    {
    paramstr *p;
    uschar fnbuff[16];
    uschar ffbuff[16];

    (void)sprintf(CS fnbuff, "%d", ++example_number);

    /* Some versions of gcc give warnings if a non-literal format string has
    no format arguments; for that reason, we give an argument in case 0, even
    though we know it is not needed. */

    switch(example_nformat_pcount)
      {
      case 0:
      (void)sprintf(CS ffbuff, CS example_number_format, fnbuff);
      break;

      case 1:
      (void)sprintf(CS ffbuff, CS example_number_format, fnbuff);
      break;

      default:
      (void)sprintf(CS ffbuff, CS example_number_format, chapter_number, fnbuff);
      break;
      }

    p = misc_malloc(sizeof(paramstr) + Ustrlen(fnbuff));
    Ustrcpy(p->name, "#number");
    Ustrcpy(p->value, ffbuff);
    p->next = i->p.param;
    p->seen = TRUE;
    i->p.param = p;

    (void)sprintf(CS buffer, CS example_title_format, ffbuff);

    oktitle = TRUE;
    continue;
    }

  /* If none of the above and a numberable title is not permitted, carry on to
  the next element. */

  if (!oktitle) continue;

  /* Otherwise see if we have reached it, and if so, do the numbering. */

  if (Ustrcmp(i->name, "title") == 0)
    {
    item *j;
    textblock *old, *new;

    /* We can stop as soon as we've processed the first text block */

    for (j = i->next; j != i->partner; j = j->next)
      {
      int newlength;
      if (Ustrcmp(j->name, "#PCDATA") != 0) continue;

      j->flags |= IF_NUMBERED;

      old = j->p.txtblk;
      newlength = old->length + Ustrlen(buffer);
      new = misc_malloc(sizeof(textblock) + newlength);

      *new = *old;
      new->length = newlength;
      Ustrcpy(new->string, buffer);
      Ustrcat(new->string, old->string);

      j->p.txtblk = new;
      misc_free(old, sizeof(textblock) + old->length);
      break;
      }

    oktitle = FALSE;  /* No more processing till next titleable item */
    }

  /* Otherwise skip over any element whose name ends with "info" */

  else if (Ustrlen(i->name) >= 4 &&
          Ustrcmp(i->name + Ustrlen(i->name) - 4, "info") == 0)
    i = i->partner;

  /* Otherwise, a title may no longer happen */

  else oktitle = FALSE;
  }

DEBUG(D_number) debug_print_item_list(item_list, "after numbering");
return TRUE;
}

/* End of number.c */
