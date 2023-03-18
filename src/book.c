/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains code for fishing the data out of <book>, <bookinfo>, or
<article> elements. */

#include "sdop.h"

/* Tables for information we recognize in <bookinfo> or <articleinfo>. Some of
the elements are recursive in that they point to their own lists of
sub-elements that are separately extracted. Others require the removal of
their contained item lists for independent processing later.

I tried to use a union in the structure to save storage, but the complications
involved in initializing defeated me. The problem is the dreaded "dereferencing
type-punned pointer will break strict-aliasing rules" warning from GCC. So I've
just used a bit more space and given the different types each their own
variable in the structure. */

enum { ETYPE_STRING, ETYPE_RECURSE, ETYPE_ITEMLIST };

typedef struct bookinfo_element {
  uschar  *name;
  int      type;
  uschar **varptr;
  item   **itemptr;
  struct bookinfo_element *sublist;
} bookinfo_element;

static bookinfo_element bi_author_affiliation[] = {
  { US"jobtitle", ETYPE_STRING, &author_jobtitle, NULL, NULL },
  { US"orgname",  ETYPE_STRING, &author_orgname,  NULL, NULL },
  { NULL, 0, NULL, NULL, NULL }
};

static bookinfo_element bi_author[] = {
  { US"affiliation", ETYPE_RECURSE, NULL, NULL, bi_author_affiliation  },
  { US"firstname",   ETYPE_STRING,  &author_firstname, NULL, NULL },
  { US"honorific",   ETYPE_STRING,  &author_honorific, NULL, NULL },
  { US"lineage",     ETYPE_STRING,  &author_lineage,   NULL, NULL },
  { US"othername",   ETYPE_STRING,  &author_othername, NULL, NULL },
  { US"surname",     ETYPE_STRING,  &author_surname,   NULL, NULL },
  { NULL, 0, NULL, NULL, NULL }
};

static bookinfo_element bi_copyright[] = {
  { US"holder", ETYPE_STRING, &book_cpyholder, NULL, NULL },
  { US"year",   ETYPE_STRING, &book_cpyyear,   NULL, NULL },
  { NULL, 0, NULL, NULL, NULL }
};

static bookinfo_element bi_editor_affiliation[] = {
  { US"jobtitle", ETYPE_STRING, &editor_jobtitle, NULL, NULL },
  { US"orgname",  ETYPE_STRING, &editor_orgname,  NULL, NULL },
  { NULL, 0, NULL, NULL, NULL }
};

static bookinfo_element bi_editor[] = {
  { US"affiliation", ETYPE_RECURSE, NULL, NULL, bi_editor_affiliation},
  { US"firstname",   ETYPE_STRING,  &editor_firstname, NULL, NULL },
  { US"honorific",   ETYPE_STRING,  &editor_honorific, NULL, NULL },
  { US"lineage",     ETYPE_STRING,  &editor_lineage,   NULL, NULL },
  { US"othername",   ETYPE_STRING,  &editor_othername, NULL, NULL },
  { US"surname",     ETYPE_STRING,  &editor_surname,   NULL, NULL },
  { NULL, 0, NULL, NULL, NULL }
};

static bookinfo_element bi_othercredit_affiliation[] = {
  { US"jobtitle", ETYPE_STRING, &othercredit_jobtitle, NULL, NULL },
  { US"orgname",  ETYPE_STRING, &othercredit_orgname,  NULL, NULL },
  { NULL, 0, NULL, NULL, NULL }
};

static bookinfo_element bi_othercredit[] = {
  { US"affiliation", ETYPE_RECURSE, NULL, NULL, bi_othercredit_affiliation},
  { US"firstname",   ETYPE_STRING,  &othercredit_firstname, NULL, NULL },
  { US"honorific",   ETYPE_STRING,  &othercredit_honorific, NULL, NULL },
  { US"lineage",     ETYPE_STRING,  &othercredit_lineage,   NULL, NULL },
  { US"othername",   ETYPE_STRING,  &othercredit_othername, NULL, NULL },
  { US"surname",     ETYPE_STRING,  &othercredit_surname,   NULL, NULL },
  { NULL, 0, NULL, NULL, NULL }
};

static bookinfo_element bi_revision[] = {
  { US"authorinitials",  ETYPE_STRING,   &book_revauthorinitials, NULL, NULL },
  { US"date",            ETYPE_STRING,   &book_revdate,           NULL, NULL },
  { US"revdescription",  ETYPE_ITEMLIST, NULL, &revdescription_item_list, NULL },
  { US"revnumber",       ETYPE_STRING,   &book_revnumber,         NULL, NULL },
  { NULL, 0, NULL, NULL, NULL }
};

static bookinfo_element bi_revhistory[] = {
  { US"revision", ETYPE_RECURSE, NULL, NULL, bi_revision },
  { NULL, 0, NULL, NULL, NULL }
};

static bookinfo_element bi_top[] = {
  { US"address",        ETYPE_STRING,   &author_address,        NULL, NULL},
  { US"affiliation",    ETYPE_RECURSE,  NULL, NULL,  bi_author_affiliation },
  { US"author",         ETYPE_RECURSE,  NULL, NULL,              bi_author },
  { US"authorinitials", ETYPE_STRING,   &author_initials,       NULL, NULL },
  { US"copyright",      ETYPE_RECURSE,  NULL, NULL,           bi_copyright },
  { US"corpauthor",     ETYPE_STRING,   &author_corpauthor,     NULL, NULL },
  { US"date",           ETYPE_STRING,   &book_date,             NULL, NULL },
  { US"edition",        ETYPE_STRING,   &book_edition,          NULL, NULL },
  { US"editor",         ETYPE_RECURSE,  NULL, NULL,              bi_editor },
  { US"issuenum",       ETYPE_STRING,   &book_issuenum,         NULL, NULL },
  { US"legalnotice",    ETYPE_ITEMLIST, NULL, &legalnotice_item_list, NULL },
  { US"othercredit",    ETYPE_RECURSE,  NULL, NULL,         bi_othercredit },
  { US"pubdate",        ETYPE_STRING,   &book_pubdate,          NULL, NULL },
  { US"publishername",  ETYPE_STRING,   &book_publishername,    NULL, NULL },
  { US"releaseinfo",    ETYPE_STRING,   &book_releaseinfo,      NULL, NULL },
  { US"revhistory",     ETYPE_RECURSE,  NULL, NULL,          bi_revhistory },
  { US"subtitle",       ETYPE_STRING,   &book_subtitle,         NULL, NULL },
  { US"title",          ETYPE_STRING,   &book_title,            NULL, NULL },
  { US"titleabbrev",    ETYPE_STRING,   &book_titleabbrev,      NULL, NULL },
  { US"volumenum",      ETYPE_STRING,   &book_volumenum,        NULL, NULL },
  { NULL, 0, NULL, NULL, NULL }
};



/*************************************************
*      Get next item, skipping pins              *
*************************************************/

static item *
next_real_item(item *i)
{
while (i != NULL && (i->name[0] == '?' || i->name[0] == '#')) i = i->next;
return i;
}



/*************************************************
*      Extract a text string from an element     *
*************************************************/

/* Text is read into textblocks on input, because that's the way we want it for
typesetting purposes. However, for the <book> and <bookinfo> information, we
need to hoick it out again into conventional text strings that can then be
inserted in other places.

Argument:   an item that may contain textblocks
Returns:    pointer to a C string
*/

static uschar *
extract_string(item *i)
{
int length = 0;
uschar *yield, *s;

for (item *ii = i->next; ii != i->partner; ii = ii->next)
  {
  if (Ustrcmp(ii->name, "#PCDATA") != 0) continue;
  length += ii->p.txtblk->length;
  }

s = yield = misc_malloc(length + 1);

for (item *ii = i->next; ii != i->partner; ii = ii->next)
  {
  if (Ustrcmp(ii->name, "#PCDATA") != 0) continue;
  memcpy(s, ii->p.txtblk->string, ii->p.txtblk->length);
  s += ii->p.txtblk->length;
  }

while (s > yield && isspace(s[-1])) s--;
*s = 0;
return yield;
}



/*************************************************
*         Extract a sub-itemlist                 *
*************************************************/

/* This function is called for bookinfo elements such as legalnotice that
require the excision of their contained items. What is cut out it added to
chain whose base is passed as an argument.

Arguments:
  i          the containing element
  anchorptr  points to the chain anchor

Returns:     nothing
*/

static void
extract_itemlist(item *i, item **anchorptr)
{
item *excised;
if (i->partner == i || i->partner == i->next) return;  /* Nothing to excise */
excised = i->next;
excised->prev = NULL;
i->partner->prev->next = NULL;
i->next = i->partner;
i->partner->prev = i;
while (*anchorptr != NULL) anchorptr = &((*anchorptr)->next);
*anchorptr = excised;
}



/*************************************************
*          Extract bookinfo elements             *
*************************************************/

/* There are some lists of elements within <bookinfo> that we can handle. Some
of them are nested within other elements, so this function is recursive.

Arguments:
  i          the outer element, within which to search
  bi_list    the list of elements to seek

Returns:     nothing
*/

static void
extract_bookinfo_elements(item *i, bookinfo_element *bi_list)
{
for (item *ii = i->next; ii != i->partner; ii = ii->next)
  {
  if (ii->partner == ii) continue;
  for (bookinfo_element *bi = bi_list; bi->name != NULL; bi++)
    {
    if (Ustrcmp(ii->name, bi->name) == 0)
      {
      if (bi->type == ETYPE_STRING)
        *(bi->varptr) = extract_string(ii);
      else if (bi->type == ETYPE_ITEMLIST)
        extract_itemlist(ii, bi->itemptr);
      else
        extract_bookinfo_elements(ii, bi->sublist);
      ii = ii->partner;
      break;
      }
    }
  }
}



/*************************************************
*     Process <book>/<article> and <xxxinfo>     *
*************************************************/

/* Apart from internal items, processing instructions, headings, and comments,
<book> or <article> must be the first element in a DocBook file. Headings and
comments are discarded when the file is read. Titles etc are extracted into
book_xxx variables rather than handled differently for articles.

Argument:      the first item in the list to be processed
Returns:       TRUE to continue processing; FALSE to stop
               (currently always TRUE)
*/

BOOL
book_getdata(item *item_list)
{
item *next;

DEBUG(D_any) debug_printf("Processing <book> or <article>\n");

if ((next = next_real_item(item_list->next)) == NULL) return TRUE;

if (Ustrcmp(next->name, "book") == 0) document_type = DOC_BOOK;
  else if (Ustrcmp(next->name, "article") == 0)
    {
    document_type = DOC_ARTICLE;
    number_sections[0] = FALSE;
    }

if (document_type != DOC_UNSET)
  {
  if ((next = next_real_item(next->next)) == NULL) return TRUE;
  if (Ustrcmp(next->name, "title") == 0)
    {
    book_title = extract_string(next);
    if ((next = next_real_item(next->partner->next)) == NULL) return TRUE;
    if (Ustrcmp(next->name, "subtitle") == 0)
      {
      book_subtitle = extract_string(next);
      if ((next = next_real_item(next->partner->next)) == NULL) return TRUE;
      }
    if (Ustrcmp(next->name, "titleabbrev") == 0)
      {
      book_titleabbrev = extract_string(next);
      if ((next = next_real_item(next->partner->next)) == NULL) return TRUE;
      }
    }
  }

if (Ustrcmp(next->name, "bookinfo") == 0)
  {
  if (document_type != DOC_BOOK) error(96);
  extract_bookinfo_elements(next, bi_top);
  }

else if (Ustrcmp(next->name, "articleinfo") == 0)
  {
  uschar tfilename[128];
  int nest_stackptr = 0;
  item *nest_stack[NESTSTACKSIZE];
  if (document_type != DOC_ARTICLE) error(97);
  extract_bookinfo_elements(next, bi_top);
  (void)misc_find_share(US"arttemplate", tfilename, TRUE);
  read_addto = next->partner;
  internal_processing = TRUE;
  (void)read_file2(tfilename, nest_stack, &nest_stackptr);
  internal_processing = FALSE;
  }

return TRUE;
}



/*************************************************
*               Make title pages                 *
*************************************************/

/* Title pages are made if there is a book title.

Arguments:     none
Returns:       TRUE to continue processing; FALSE to stop
               (currently always TRUE)
*/

BOOL
book_title_make(void)
{
int rc;
int nest_stackptr = 0;
item *nest_stack[NESTSTACKSIZE];
uschar tfilename[128];

if (book_title == NULL)
  {
  DEBUG(D_any) debug_printf("===> No title pages (no book title)\n");
  return TRUE;
  }

DEBUG(D_any) debug_printf("===> Creating title pages\n");

(void)misc_find_share(US"titletemplate", tfilename, TRUE);
read_addto = title_item_list;
internal_processing = TRUE;
(void)read_file2(tfilename, nest_stack, &nest_stackptr);
internal_processing = FALSE;

DEBUG(D_title) debug_print_item_list(title_item_list, "for the title pages");

/* Now process the title pages */

read_what = US"processing title pages";
rc =  pin_cutcond(title_item_list) &&
      pin_process_inserts(title_item_list) &&
      entity_expand(title_item_list) &&
      url_check(title_item_list) &&
      font_assign(title_item_list, FONTS_TITLE) &&
      font_loadalltables() &&
      para_identify(title_item_list, FONTS_TITLE, NULL) &&
      table_identify(title_item_list, NULL) &&
      para_format(title_item_list) &&
      page_format(title_item_list, NULL, title_even_pages, FALSE,
        &title_page_count, US"title pages");

read_what = NULL;
return rc;
}

/* End of book.c */
