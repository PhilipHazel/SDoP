/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains code for creating a table of contents. */

#include "sdop.h"

static uschar *titels[] = { US"titleabbrev",
                            US"subtitle",
                            US"title",
                            NULL };

static uschar *ritels[] = { US"#RAWTITLEABBREV",
                            US"#RAWSUBTITLE",
                            US"#RAWTITLE" };

static int  toc_depth;
static int  toc_page;
static pdfmarkstr **toc_pdftail;

static BOOL firstchapter;
static BOOL firstsection;
static BOOL in_index;
static BOOL in_section;
static BOOL index_headings_pdf_toc;



/*************************************************
*          Save raw titles for TOC               *
*************************************************/

/* This function is called before entity processing. It scans the input
document for preface, chapter, section, appendix, and index titles, and saves a
copy of their texts in raw form so that it can be reprocessed later when
building the TOC and when setting up heads and feet (that's why it notices
"article" even though no TOC is printed for articles).

Argument:      the first item in the list
Returns:       TRUE to continue processing; FALSE to stop
               (currently always TRUE)
*/

BOOL
toc_save_raw_titles(item *item_list)
{
for (item *i = item_list; i != NULL; i = i->next)
  {
  if (!ISSECT(i->name) &&
      Ustrcmp(i->name, "chapter")  != 0 &&
      Ustrcmp(i->name, "preface")  != 0 &&
      Ustrcmp(i->name, "appendix") != 0 &&
      Ustrcmp(i->name, "index")    != 0 &&
      Ustrcmp(i->name, "article")  != 0)
    continue;

  /* Scan for titleabbrev, subtitle, title, in that order. As we insert after
  the main item, that means the raw elements will end up in the opposite order.
  If there is no title, put in a default. */

  for (int k = 0; titels[k] != NULL; k++)
    {
    item *new;
    lengthstring *ls = misc_find_title(i, titels[k]);
    if (ls == NULL)
      {
      if (titels[k+1] != NULL) continue;  /* Ignore unless title */
      ls = misc_malloc(sizeof(lengthstring) + Ustrlen(UNTITLED));
      ls->length = Ustrlen(UNTITLED);
      Ustrcpy(ls->value, UNTITLED);
      }
    new = misc_malloc(sizeof(item));
    new->partner = new;
    new->linenumber = i->linenumber;
    new->flags = 0;
    Ustrcpy(new->name, ritels[k]);
    new->p.lngthstrng = ls;

    /* If the item is its own partner, we have a pathological case, and really
    all we want to do is not crash. However, the easiest thing to do is to
    create a partner and insert the title so that subsequent processing will
    all work OK. Because there may be no following item, insert the partner by
    hand. */

    if (i->partner == i)
      {
      item *ii = misc_malloc(sizeof(item));
      Ustrcpy(ii->name, US"/");
      ii->linenumber = i->linenumber;
      ii->flags = 0;
      ii->partner = i;
      i->partner = ii;
      ii->next = i->next;
      ii->prev = i;
      if (i->next != NULL) i->next->prev = ii;
      i->next = ii;
      }

    /* Insert the saved title */

    misc_insert_item(new, i->next);
    }
  }
return TRUE;
}



/*************************************************
*          Handle item for TOC                   *
*************************************************/

/* This is called separately for preface and main body items. It generates TOC
lines and associated pdfmark data. Some static variables that persist for the
whole TOC (over the preface and body) are used.

Arguments:
  i               the current item
  nest_stack      stack for item nesting
  anest_stackptr  pointer to the stack pointer
  ispreface       TRUE when dealing with preface items

Returns:          nothing
*/

static void
toc_handle_item(item *i, item **nest_stack, int *anest_stackptr, BOOL ispreface)
{
BOOL is_section = FALSE;
BOOL is_index = FALSE;
pdfmarkstr *pdf;

/* Adjust section selection and numbering. For TOC sections, "no" is assumed
for any that follow a "no". Setting toc_sections also sets toc_printed_
sections. */

if (Ustrcmp(i->name, "?sdop") == 0)
  {
  paramstr *p = misc_param_find(i, US"index_headings_pdf_toc");
  if (p != NULL) index_headings_pdf_toc = Ustrcmp(p->value, "yes") == 0;
  (void)misc_yesno_vector(i, US"numbertitles", number_sections, MAXSECTDEPTH);
  if (misc_yesno_vector(i, US"toc_sections", toc_sections, MAXSECTDEPTH))
    memcpy(toc_printed_sections, toc_sections, sizeof(toc_sections));
  (void)misc_yesno_vector(i, US"toc_printed_sections", toc_printed_sections,
    MAXSECTDEPTH);
  return;
  }

/* Keep the page number up to date. */

else if (Ustrcmp(i->name, "#PDATA") == 0) toc_page++;

/* For each preface, chapter, section, appendix, and index generate a TOC line
if wanted. However, within an index, don't do sections. */

else if (Ustrcmp(i->name, "preface")  == 0 ||
         Ustrcmp(i->name, "chapter")  == 0 ||
         Ustrcmp(i->name, "appendix") == 0 ||
         (is_section = ISSECT(i->name)) ||
         (is_index   = Ustrcmp(i->name, "index") == 0))
  {
  uschar ss[256];
  uschar tt[256];
  uschar **strings;
  paramstr *p = misc_param_find(i, US"#number");
  uschar *s, *t;

  if (is_section) in_section = TRUE;
    else if (is_index) in_index = TRUE;

  /* The active title will have had its entities expanded, so instead we
  use the raw title string that has been saved for precisely this purpose. */

  s = (Ustrcmp(i->next->name, "#RAWTITLE") == 0)?
    i->next->p.lngthstrng->value : UNTITLED;

  /* Set up for a chapter/preface/appendix/index. For all but the first, insert
  a blank line before it, if so configured. */

  if (!is_section)
    {
    if (!toc_sections[0]) return;
    if (!firstchapter && toc_chapter_blanks[0] && toc_printed_sections[0])
      read_string(US"<row><entry>&nbsp;</entry></row>", nest_stack,
        anest_stackptr, 0);
    firstchapter = FALSE;
    firstsection = TRUE;
    }

  /* Set up for sections, subsections, etc. Within an index, where there is
  only ever one level of section, skip unless index_headings_pdf_toc is TRUE.
  This is independent of toc_sections, but index sections are present only if
  index_headings_enabled is TRUE (the default). Sections within an index are
  never included in the printed TOC, but a pdfmark is created for them if
  index_headings_pdf_toc is TRUE. */

  else
    {
    toc_depth++;

    if (in_index)
      {
      if (index_headings_pdf_toc)
        {
        lengthstring *ls = misc_find_title(i, US"title");
        pdf = misc_malloc(sizeof(pdfmarkstr) + Ustrlen(ls->value));
        pdf->next = NULL;
        *toc_pdftail = pdf;
        toc_pdftail = &(pdf->next);
        pdf->page = toc_page;
        pdf->level = toc_depth;
        pdf->ispreface = ispreface;
        Ustrcpy(pdf->text, ls->value);
        }
      return;
      }

    /* For a normal section, increase the depth. Skip if we are too deep for
    the TOC. If this is the first section in a chapter, insert a blank line if
    so configured. */

    if (!toc_sections[toc_depth]) return;
    if (firstsection && toc_chapter_blanks[1] && toc_printed_sections[1])
      {
      read_string(US"<row><entry>&nbsp;</entry></row>", nest_stack,
        anest_stackptr, 0);
      }
    firstsection = FALSE;
    }

  /* Handle cases that go into the printed TOC (everything except sections in
  indexes). Find the configured strings for building the line. */

  strings = toc_line_strings[(toc_depth >= toc_line_string_count)?
    toc_line_string_count - 1 : toc_depth];

  /* Create the title text. Omit numbering for the index or if no number is
  available. */

  if (number_sections[toc_depth] && !in_index && p != NULL)
    (void)sprintf(CS ss, "%s%s%s%s%s%s", strings[0], strings[1],
      p->value, strings[2], s, strings[3]);
  else
    (void)sprintf(CS ss, "%s%s%s", strings[0], s, strings[3]);

  /* Now create the row for the TOC line, provided that this section is not cut
  out by toc_printed_sections. */

  if (toc_printed_sections[toc_depth])
    {
    read_string(US"<row><entry>", nest_stack, anest_stackptr, 0);
    read_string(ss, nest_stack, anest_stackptr, 0);
    read_string(US"</entry><entry>", nest_stack, anest_stackptr, 0);
    (void)sprintf(CS tt, "%s%d%s", strings[4], toc_page, strings[5]);
    read_string(tt, nest_stack, anest_stackptr, 0);
    read_string(US"</entry></row>", nest_stack, anest_stackptr, 0);
    }

  /* Remember the title text (currently in ss) and page number for the
  creation of a pdfmark item. We need to expand entities and remove XML
  elements from the text before doing this. */

  t = tt;
  for (s = ss; *s != 0; s++)
    {
    if (*s == '&')
      {
      uschar *value;
      s = entity_find(s + 1, &value, TRUE, US" in TOC lines") - 1;
      t += sprintf(CS t, "%s", value);
      }
    else if (*s == '<')
      {
      while (*(++s) != 0 && *s != '>');
      }
    else *t++ = *s;
    }
  *t = 0;

  pdf = misc_malloc(sizeof(pdfmarkstr) + Ustrlen(tt));
  pdf->next = NULL;
  *toc_pdftail = pdf;
  toc_pdftail = &(pdf->next);
  pdf->page = toc_page;
  pdf->level = toc_depth;
  pdf->ispreface = ispreface;
  Ustrcpy(pdf->text, tt);
  }

/* Handle the ends of sections and indexes. */

else if (Ustrcmp(i->name, "/") == 0)
  {
  if (ISSECT(i->partner->name))
    {
    toc_depth--;
    in_section = FALSE;
    }
  else if (Ustrcmp(i->partner->name, "index") == 0)
    {
    in_index = FALSE;
    }
  }
}



/*************************************************
*              Create the TOC if required        *
*************************************************/

/* This function scans the input document, and creates the table of contents,
controlled by the toc_sections[] vector.

Argument:
  body_item_list   the first item of the main body
  pref_item_list   the first item of the preface, or NULL

Returns:           TRUE to continue processing; FALSE to stop
*/

BOOL
toc_make(item *body_item_list, item *pref_item_list)
{
BOOL rc;
int nest_stackptr = 0;
item *nest_stack[NESTSTACKSIZE];
uschar tfilename[128];

DEBUG(D_any) debug_printf("===> Creating TOC\n");

/* First, read in the template for the start of the data. This interface to the
reading code leaves a list of unclosed elements on the stack. Set internal
processing so that we don't get debugging output. */

(void)misc_find_share(US"toctemplate", tfilename, TRUE);
read_addto = toc_item_list;
internal_processing = TRUE;
(void)read_file2(tfilename, nest_stack, &nest_stackptr);
internal_processing = FALSE;

/* Now create the rows of the table from the data in the preface and body item
lists. */

read_what = US"creating TOC lines";

firstchapter = TRUE;
firstsection = TRUE;
in_index = FALSE;
in_section = FALSE;
index_headings_pdf_toc = FALSE;

toc_depth = 0;
toc_pdftail = &toc_pdfmarks;

/* First scan the preface, if there is one. It will be created with large
negative page numbers, which leaves space for the correct roman numbers to be
inserted once we know the length of the TOC. */

if (pref_item_list != NULL)
  {
  toc_page = PREFACE_DUMMY_PAGE;
  memcpy(toc_sections, toc_sections_preface_default,
    sizeof(toc_sections_preface_default));
  memcpy(toc_printed_sections, toc_sections_preface_default,
    sizeof(toc_sections_preface_default));
  number_sections[0] = FALSE;
  for (item *i = pref_item_list; i != NULL; i = i->next)
    toc_handle_item(i, nest_stack, &nest_stackptr, TRUE);
  }

/* Now scan the body, first re-setting the default values for selection and
numbering sections, and the page number. */

memcpy(toc_sections, toc_sections_default, sizeof(toc_sections_default));
memcpy(toc_printed_sections, toc_sections_default,
  sizeof(toc_sections_default));
memcpy(number_sections, number_sections_default,
  sizeof(number_sections_default));
toc_page = 0;

for (item *i = body_item_list; i != NULL; i = i->next)
  toc_handle_item(i, nest_stack, &nest_stackptr, FALSE);

/* If it turns out that we have not actually generated any TOC data, abandon
the initial input that we have, and do no more. */

if (toc_pdfmarks == NULL)
  {
  DEBUG(D_any) debug_printf("TOC not written because no entries found\n");
  toc_item_list->next = NULL;    /* Leave the initial dummy */
  read_what = NULL;
  return TRUE;
  }

/* Now automatically close all the open elements. */

while (nest_stackptr > 0)
  {
  item *partner = nest_stack[--nest_stackptr];
  item *new = misc_malloc(sizeof(item));
  new->prev = read_addto;
  new->next = read_addto->next;
  new->linenumber = 0;
  new->flags = 0;
  Ustrcpy(new->name, "/");
  new->p.param = NULL;
  read_addto->next = new;
  read_addto = new;

  new->partner = partner;
  partner->partner = new;
  }

DEBUG(D_toc) debug_print_item_list(toc_item_list, "for the TOC");

/* If there is a fill string, note that its font is used. We may get back a
pointer to an identical vfont that has already been set up. */

if (toc_fill_string[0] != 0)
  toc_fill_vfont_ptr = font_used(&toc_fill_vfont, NULL);

/* Now process the TOC pages */

read_what = US"processing TOC lines";

rc =  entity_expand(toc_item_list) &&
      font_assign(toc_item_list, FONTS_TOC) &&
      font_loadalltables() &&
      para_identify(toc_item_list, FONTS_TOC, NULL) &&
      table_identify(toc_item_list, NULL) &&
      para_format(toc_item_list) &&
      page_format(toc_item_list, NULL, toc_even_pages, FALSE, &toc_page_count,
        US"TOC");

/* If there was a preface, scan the TOC items for negative page numbers, and
replace them with the correct roman page numbers. We know that each such number
must be in a textblock on its own in an "output line" (really a table cell) on
its own. We also know that the cell was right-justified, so we have to adjust
the indent appropriately. */

if (rc && pref_item_list != NULL)
  {
  for (item *i = toc_item_list; i != NULL; i = i->next)
    {
    outputline *ol;

    /* For each "paragraph", scan the output lines. For a line-number cell in
    the table, there will be just one textblock. */

    if (Ustrcmp(i->name, "#PCPARA") != 0) continue;

    for (ol = i->p.prgrph->out; ol != NULL; ol = ol->next)
      {
      long int n;
      uschar *s, *t;
      textblock *tb = ol->txtblk;

      /* Check that that is just a single textblock, and get its string. */

      if (tb == NULL || tb->next != NULL) continue;
      s = tb->string;

      /* If the string consists entirely of digits, assume we have reached a
      non-preface entry, and terminate the loop. The assignment to n in this
      statement is purely to stop gcc grumbling about ignoring the return value
      in environments when FORTIFY_SOURCE is set, and where casting to (void)
      does not help. If the string does not consist entirely of digits, look
      for a string that is a negative number in a reasonable range, and convert
      it. */

      if (isdigit(*s) && (n = Ustrtol(s, (char *)(&t), 10), *t == 0)) break;
      if (*s != '-') continue;
      n = Ustrtol(s, (char *)(&t), 10) - PREFACE_DUMMY_PAGE;
      if (*t != 0 || n <= 0 || n > 1000) continue;  /* Paranoia */

      /* Replace the number with the correct roman numeral, adjust the string
      length, and also the width and indent of the line. */

      misc_roman(s, n + toc_page_count + title_page_count);
      tb->length = Ustrlen(s);
      ol->indent += ol->width;
      ol->width = font_stringwidth(s, tb->vfont);
      ol->indent -= ol->width;
      }
    }
  }

read_what = NULL;
return rc;
}

/* End of toc.c */
