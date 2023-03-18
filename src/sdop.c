/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains the main program and initialization functions. */

#include "sdop.h"



/*************************************************
*                 Static variables               *
*************************************************/

static uschar *sdop_filename = NULL;
static uschar *out_filename = NULL;

static bit_table debug_options[] = {
  { US"all",             D_all },
  { US"entity",          D_entity },
  { US"fill",            D_fill },
  { US"font",            D_font },
  { US"fontload",        D_fontload },
  { US"hyphen",          D_hyphen },
  { US"index",           D_index },
  { US"indexfull",       D_indexfull },
  { US"internal",        D_internal },
  { US"ipara",           D_ipara },
  { US"itable",          D_itable },
  { US"number",          D_number },
  { US"object",          D_object },
  { US"page",            D_page },
  { US"para",            D_para },
  { US"param",           D_param },
  { US"read",            D_read },
  { US"ref",             D_ref },
  { US"title",           D_title },
  { US"toc",             D_toc },
  { US"write",           D_write }
};

static int debug_options_count = sizeof(debug_options)/sizeof(bit_table);


static templatestr templates[] = {
  { US"foottable",   &main_foot_item_list,    FALSE },
  { US"headtable",   &main_head_item_list,     TRUE },
  { US"foottable-p", &preface_foot_item_list, FALSE },
  { US"headtable-p", &preface_head_item_list,  TRUE },
  { US"foottable-t", &toc_foot_item_list,     FALSE },
  { US"headtable-t", &toc_head_item_list,      TRUE }
};

static int template_count = sizeof(templates)/sizeof(templatestr);



/*************************************************
*            Page list decoding                  *
*************************************************/

/* This is called for both -p and -pf. In the former case we allow "odd" and
"even" as well as page numbers.

Arguments:
  plist        the page list
  arg          -p or -pf

Returns:       FALSE after any error
*/

static BOOL
decode_pagelist(uschar *plist, uschar *arg)
{
pagelist *pl = NULL;
BOOL ismain = arg[2] == 0;

if ((ismain && pages_main != NULL) || (!ismain && pages_front != NULL))
  return error(81, arg);

while (*plist != 0)
  {
  pagelist *newpl;
  char *end;              /* Must be char *; casting doesn't suppress gcc */
  uschar buffer[64];                     /* "type-punned" warning message */
  uschar *c = Ustrchr(plist, ',');

  if (c == NULL)
    {
    Ustrcpy(buffer, plist);
    plist += Ustrlen(plist);
    }
  else
    {
    Ustrncpy(buffer, plist, c - plist);
    buffer[c - plist] = 0;
    plist = c + 1;
    }

  if (Ustrcmp(buffer, "odd") == 0)
    {
    if (!ismain) return error(82);
    pages_odd = TRUE;
    continue;
    }
  else if (Ustrcmp(buffer, "even") == 0)
    {
    if (!ismain) return error(82);
    pages_even = TRUE;
    continue;
    }

  newpl = misc_malloc(sizeof(pagelist));
  newpl->next = NULL;

  newpl->start = Ustrtoul(buffer, &end, 10);
  newpl->end = (*end == '-')? Ustrtoul(end + 1, &end, 10) : newpl->start;
  if (*end != 0) return error(83, arg, buffer);

  if (newpl->end < newpl->start) return error(84, arg);

  if (pl == NULL)
    {
    if (ismain) pages_main = newpl; else pages_front = newpl;
    }
  else
    {
    if (newpl->start <= pl->end) return error(84, arg);
    pl->next = newpl;
    }

  pl = newpl;
  }

return TRUE;
}



/*************************************************
*          Debug option decoding                 *
*************************************************/

/* This function decodes a string containing bit settings in the form of +name
and/or -name sequences, and sets/unsets bits in the debug selector bit string
accordingly.

Arguments:
  string         the argument string

Returns:         TRUE on success, FALSE on failure
*/

static BOOL
decode_debug(uschar *string)
{
for(;;)
  {
  BOOL adding;
  uschar *s;
  int len;
  bit_table *start, *end;

  while (isspace(*string)) string++;
  if (*string == 0) return TRUE;

  if (*string != '+' && *string != '-')
    {
    (void)fprintf(stderr, "sdop: malformed -d option: "
      "+ or - expected but found \"%s\"", string);
    return FALSE;
    }

  adding = *string++ == '+';
  s = string;
  while (isalnum(*string) || *string == '_') string++;
  len = string - s;

  start = debug_options;
  end = debug_options + debug_options_count;

  while (start < end)
    {
    bit_table *middle = start + (end - start)/2;
    int c = Ustrncmp(s, middle->name, len);
    if (c == 0)
      {
      if (middle->name[len] != 0) c = -1; else
        {
        unsigned int bit = middle->bit;
        if (adding) debug_selector |= bit; else debug_selector &= ~bit;
        break;  /* Out of loop to match selector name */
        }
      }
    if (c < 0) end = middle; else start = middle + 1;
    }  /* Loop to match selector name */

  if (start >= end)
    {
    (void)fprintf(stderr, "sdop: unknown debug selector setting: %c%.*s\n",
      adding? '+' : '-', len, s);
    return FALSE;
    }
  }           /* Loop for selector names */

return TRUE;  /* Should never get here, but keep compiler happy */
}



/*************************************************
*                  Usage                         *
*************************************************/

static void
usage(void)
{
(void)fprintf(stderr,
  "Usage: sdop [options] [input file]\n"
  "  -d<debug-options>         produce debug output (no space after -d)\n"
  "  -o <output-file>          specify output file\n"
  "  -p <pagelist>             output these main body pages\n"
  "  -pf <pagelist>            output these frontmatter pages\n");

(void)fprintf(stderr,
  "  -q                        suppress warnings for unsupported DocBook features\n"
  "  -qc                       suppress warnings for unsupported characters\n"
  "  -S <directories>          list of additional SDoP share directories\n"
  "  -w                        warn for unsupported DocBook features (default)\n"
  "  -wc                       warn for unsupported characters (default)\n"
  "  -v or --version           show version information\n"
  );

(void)fprintf(stderr,
"\nA page list for -p (but not -pf) may include \"odd\" or \"even\"; this"
"\napplies to all output pages. The -S option overrides SDOP_SHARE.\n");

(void)fprintf(stderr, "\nDebug options (+ to add, - to subtract):");
for (int i = 0; i < debug_options_count; i++)
  {
  if ((i & 7) == 0) (void)fprintf(stderr, "\n  ");
  (void)fprintf(stderr, " %s", debug_options[i].name);
  }
(void)fprintf(stderr, "\n");
}



/*************************************************
*          Command line argument decoding        *
*************************************************/

/* Arguments: as for main()
   Returns:   TRUE if OK
*/

static BOOL
sdop_decode_arg(int argc, char **argv)
{
int i;
for (i = 1; i < argc; i++)
  {
  uschar *arg = US argv[i];
  if (*arg != '-') break;

  if (Ustrcmp(arg, "-help") == 0 || Ustrcmp(arg, "--help") == 0)
    {
    usage();
    exit(0);
    }

  if (Ustrcmp(arg, "-p") == 0 || Ustrcmp(arg, "-pf") == 0)
    {
    uschar *plist = US argv[++i];
    if (plist == NULL) { usage(); return FALSE; }
    if (!decode_pagelist(plist, arg)) return FALSE;
    }

  else if (arg[1] == 'd')
    {
    debug_selector |= D_any;
    if (!decode_debug(arg+2)) return FALSE;
    }
  else if (Ustrcmp(arg, "-o") == 0)
    {
    out_filename = US argv[++i];
    if (out_filename == NULL) { usage(); return FALSE; }
    }
  else if (Ustrcmp(arg, "-qc") == 0)
    {
    warn_unsupported_chars = FALSE;
    warn_unsupported_chars_set = TRUE;
    }
  else if (Ustrcmp(arg, "-q") == 0)
    {
    warn_unsupported = FALSE;
    warn_unsupported_set = TRUE;
    }
  else if (Ustrcmp(arg, "-S") == 0)
    {
    if (argv[++i] == NULL) { usage(); return FALSE; }
    sdop_share = US argv[i];
    }
  else if (Ustrcmp(arg, "-wc") == 0)
    {
    warn_unsupported_chars = TRUE;
    warn_unsupported_chars_set = TRUE;
    }
  else if (Ustrcmp(arg, "-w") == 0)
    {
    warn_unsupported = TRUE;
    warn_unsupported_set = TRUE;
    }
  else if (Ustrcmp(arg, "-v") == 0 ||
           Ustrcmp(arg, "--version") == 0)
    {
    (void)fprintf(stdout, "SDoP version %s %s\n", SDOP_VERSION, SDOP_DATE);
    (void)fprintf(stdout, "JPEG support: %s\n",
      #if SUPPORT_JPEG
      "yes");
      #else
      "no");
      #endif
    (void)fprintf(stdout, "PNG support: %s\n",
      #if SUPPORT_PNG
      "yes");
      #else
      "no");
      #endif
    exit(0);
    }
  else
    {
    (void)fprintf(stderr, "sdop: unknown option \"%s\"\n", arg);
    usage();
    return FALSE;
    }
  }

/* If neither odd nor even pages were requested, select both. */

if (!pages_odd && !pages_even) pages_odd = pages_even = TRUE;

/* Require there to be either 0 or 1 command line argument left. */

if (argc > i + 1)
  {
  usage();
  return FALSE;
  }

/* This will set NULL if there is no file name. If there is a file name and no
output file is specified, default it to the input name with a .ps extension. */

sdop_filename = US argv[i];
if (sdop_filename != NULL && out_filename == NULL)
  {
  uschar *p;
  int len = Ustrlen(sdop_filename);
  out_filename = misc_malloc(len + 4);
  Ustrcpy(out_filename, sdop_filename);
  if ((p = Ustrrchr(out_filename, '.')) != NULL) len = p - out_filename;
  Ustrcpy(out_filename + len, ".ps");
  }

return TRUE;
}



/*************************************************
*         Initialize hyphenation dictionary      *
*************************************************/

static BOOL
sdop_init_hyphen(void)
{
uschar hfilename[256];
if (misc_find_share(US"HyphenData", hfilename, FALSE))
  {
  main_hyphenfile = Ufopen(hfilename, "rb");
  if (main_hyphenfile == NULL && errno != ENOENT)  /* Hard */
    (void)error(0, hfilename, "hyphenation dictionary", strerror(errno));
  }
Hyphen_Init();
return TRUE;
}



/*************************************************
*         Read head/foot template files          *
*************************************************/

/* This function reads the templates for heads and feet. The template is
typically a short table. We do some preliminary processing on the text.

Arguments:  none
Returns:    TRUE/FALSE
*/

static BOOL
sdop_init_templates(void)
{
uschar tfilename[128];
internal_processing = TRUE;
for (int i = 0; i < template_count; i++)
  {
  item *item_list = *(templates[i].anchor) = misc_dummy_item();
  (void)misc_find_share(templates[i].filename, tfilename, TRUE);
  page_linewidth = (templates[i].ishead)?
    page_head_linewidth : page_foot_linewidth;
  if (!read_file(tfilename, item_list) ||
      !entity_expand(item_list) ||
      !font_assign(item_list, FONTS_HEADFOOT) ||
      !para_identify(item_list, FONTS_HEADFOOT, NULL) ||
      !table_identify(item_list, NULL))
    return FALSE;
  }
internal_processing = FALSE;
page_linewidth = page_main_linewidth;
return TRUE;
}



/*************************************************
*      Output unknown element/attribute tree     *
*************************************************/

/* Outputs the tree of unknown elements and attributes, calling itself
recursively to get them in order.

Argument:    a tree node
Returns:     nothing
*/

static void
write_unknown_tree(tree_node *tn)
{
if (tn->left != NULL) write_unknown_tree(tn->left);
if (tn->name[0] == '+')
  {
  uschar *eptr = Ustrrchr(tn->name, ':');
  uschar *vptr = Ustrchr(tn->name, '=');
  if (vptr == NULL)
    fprintf(stderr, "%.*s in <%s>\n", (int)(eptr - tn->name - 1), tn->name + 1,
      eptr + 1);
  else
    fprintf(stderr, "%.*s=\"%.*s\" in <%s>\n", (int)(vptr - tn->name - 1),
      tn->name + 1, (int)(eptr - vptr - 1), vptr + 1, eptr + 1);
  }
else
  {
  fprintf(stderr, "<%s>\n", tn->name);
  }
if (tn->right != NULL) write_unknown_tree(tn->right);
}



/*************************************************
*    Set default values after parameter changes  *
*************************************************/

/* This function is called after pin_global() has made any changes to global
parameter values.

Arguments:  none
Returns:    TRUE
*/

static BOOL
compute_defaults(void)
{
page_length = page_full_length - page_head_length - page_foot_length;
return TRUE;
}



/*************************************************
*        Remove ignored elements                 *
*************************************************/

/* This function is called to remove elements like <sectioninfo> that are
completely ignored.

Arguments:  The item list
Returns:    TRUE
*/

static BOOL
remove_ignored(item *i)
{
while (i != NULL)
  {
  if (Ustrcmp(i->name, "objectinfo")  == 0 ||
      Ustrcmp(i->name, "sectioninfo") == 0)
    {
    i->prev->next = i->partner->next;
    i->partner->next->prev = i->prev;
    i = i->prev->next;
    }
  else i = i->next;
  }
return TRUE;
}



/*************************************************
*        Scan items and check revisionflag       *
*************************************************/

/* This function scans the input document and handles the revisionflag
attribute, which is a common attribute that is allowed on all elements. We only
support "changed" at present - the check for other values happens with the
general check for supported elements/attributes.

Argument:      the first item in the list to be processed
Returns:       TRUE to continue processing; FALSE to stop
               (currently always TRUE)
*/

static BOOL
revision_check(item *item_list)
{
DEBUG(D_any) debug_printf("Processing revisionflag attributes\n");

for (item *i = item_list; i != NULL; i = i->next)
  {
  paramstr *p;
  if (i->name[0] == '/' || i->name[0] == '#') continue;
  if ((p = misc_param_find(i, US"revisionflag")) == NULL) continue;

  if (Ustrcmp(p->value, "changed") == 0)
    {
    for (item *ii = i->next; ii != i->partner; ii = ii->next)
      {
      if (Ustrcmp(ii->name, "#PCDATA") == 0)
        ii->p.txtblk->pin_flags |= PIN_REVCH;
      }
    }
  }

return TRUE;
}



/*************************************************
*         Function called to tidy on exit        *
*************************************************/

/* This cleans up memory that wasn't freed while processing. */

static void
tidy_up(void)
{
while (main_memory_chain != NULL)
   {
   memchainstr *m = main_memory_chain;
   main_memory_chain = m->next;
   free(m);
   }
}



/*************************************************
*          Entry point and main program          *
*************************************************/

/* Decode the command line, set up dummy first items for the input file and
head/foot/toc templates, initialize some data, then run the various phases of
the program. */

int
main(int argc, char **argv)
{
item *format_from;
BOOL yield;
uschar *share = (uschar *)getenv("SDOP_SHARE");

if (share != NULL)
  {
  sdop_share = misc_malloc(Ustrlen(share) + 1);
  Ustrcpy(sdop_share, share);
  }

if (atexit(tidy_up) != 0) error(117);   /* Hard */

yield = sdop_decode_arg(argc, argv);

main_item_list  = misc_dummy_item();
title_item_list = misc_dummy_item();
toc_item_list   = misc_dummy_item();

memcpy(number_sections, number_sections_default,
  sizeof(number_sections_default));

/* The sort_omit vector must be in increasing order, terminated with a value
greater than any character code. */

index_sort_omit = misc_malloc(6*sizeof(unsigned int));
index_sort_omit[0] = BREAK_PERMIT;
index_sort_omit[1] = NO_BREAK_HERE;
index_sort_omit[2] = ZERO_SPACE;
index_sort_omit[3] = OPEN_DQUOTE;
index_sort_omit[4] = CLOSE_DQUOTE;
index_sort_omit[5] = 0xffffffff;

/* Set the default starting values for the multicolumn data before processing
the data before the first index. */

page_columns_init = 1;
page_colsep_init = DEFAULT_PAGE_COLSEP;

yield = yield &&
  sdop_init_hyphen() &&
  read_main_file(sdop_filename) &&
  read_includes(main_item_list, sdop_filename) &&
  remove_ignored(main_item_list) &&
  book_getdata(main_item_list) &&
  pin_global(main_item_list) &&
  sdop_init_templates() &&
  compute_defaults() &&
  pin_cutcond(main_item_list) &&
  number_titles(main_item_list) &&
  toc_save_raw_titles(main_item_list) &&
  pin_process_inserts(main_item_list) &&
  entity_expand(main_item_list) &&
  url_check(main_item_list) &&
  footnote_insert_keys(main_item_list) &&
  ref_resolve(main_item_list) &&
  font_assign(main_item_list, FONTS_MAIN) &&
  font_loadalltables() &&
  para_identify(main_item_list, FONTS_MAIN, NULL) &&
  table_identify(main_item_list, NULL) &&
  revision_check(main_item_list) &&
  para_format(main_item_list) &&
  preface_process() &&
  page_format(main_item_list, &format_from, main_even_pages, FALSE,
    &main_page_count, US"body");

/* This loop processes a sequence of indexes and following body text. If there
are no entries for an index, the <index> element will be removed from the
chain. */

while (yield)
  {
  if (format_from == NULL) break;                /* No (more) text or indexes */
  if (Ustrcmp(format_from->name, "index") == 0)  /* Process an index */
    {
    page_columns_save = page_columns;            /* Save current columns info */
    page_colsep_save = page_colsep;              /* from end of main text. */
    page_columns_init = 1;                       /* Start index with default */
    page_colsep_init = DEFAULT_PAGE_COLSEP;      /* so the title is right. */
    yield = index_make(main_item_list, format_from);
    if (yield)
      {
      if (format_from->prev->next == format_from)
        {
        yield = font_assign(format_from, FONTS_INDEX) &&
          font_loadalltables() &&
          para_identify(format_from, FONTS_INDEX, NULL) &&
          table_identify(format_from, NULL) &&
          para_format(format_from) &&
          page_format(format_from, &format_from, main_even_pages, TRUE,
            &main_page_count, US"index");
        }
      else
        {
        item *pp;
        format_from = format_from->partner->next;
        for (pp = format_from->next; pp != NULL; pp = pp->next)
          if (Ustrcmp(pp->name, "#PCDATA") == 0) break;
        if (pp == NULL) format_from = NULL;      /* Nothing after </index> */
        }
      }
    page_columns_init = page_columns_save;       /* Reset for post-index text */
    page_colsep_init = page_colsep_save;
    }
  else                                           /* Process post-index text */
    {
    yield = yield &&
      font_assign(format_from, FONTS_MAIN) &&
      font_loadalltables() &&
      para_identify(format_from, FONTS_MAIN, NULL) &&
      table_identify(format_from, NULL) &&
      para_format(format_from) &&
      page_format(format_from, &format_from, main_even_pages, FALSE,
        &main_page_count, US"body");
    }
  }

/* Reset the multicolumn initial values to the defaults before creating the
title pages and the TOC for a book. */

page_columns_init = 1;
page_colsep_init = DEFAULT_PAGE_COLSEP;

if (document_type != DOC_ARTICLE)
  {
  yield = yield &&
    book_title_make() &&
    toc_make(main_item_list, preface_item_list);
  }

/* If there has been an error of at least severity ec_failed, do not produce
the output. */

if (error_max >= ec_failed)
  {
  fprintf(stderr, "** No output has been generated.\n");
  yield = FALSE;
  }

/* If all is well, scan for cross references and set up their page numbers so
that pdfmark links can be generated. Then generate the output. */

yield = yield && ref_set_pages() && write_file(out_filename);

/* Check and give warnings for unrecognized processing instructions. If
something has gone wrong, don't do this check because we may not have looked at
some of them. */

if (yield)
  {
  for (item *i = main_item_list; i != NULL; i = i->next)
    {
    if (Ustrcmp(i->name, "?sdop") != 0) continue;
    for (paramstr *p = i->p.param; p != 0; p = p->next)
      {
      if (!p->seen)
        {
        read_linenumber = i->linenumber;
        (void)error(105, p->name);
        }
      }
    }
  }

/* It should be OK to do this warning always. */

if (warn_unsupported && unknown_element_tree != NULL)
  {
  fprintf(stderr, "sdop: Ignored unrecognized elements and attributes:\n");
  write_unknown_tree(unknown_element_tree);
  }

/* If there were errors that did not prevent the output being generated, give a
warning. */

if (error_max > 0 && error_max < ec_failed)
  {
  fprintf(stderr, "** Warning: generated output may be flawed.\n");
  if (error_max > ec_warning) yield = FALSE;
  }

DEBUG(D_any)
  {
  debug_printf("Memory HWM = ");
  if (memory_hwm >= 1000000)
    debug_printf("%d,%03d,%03d\n", memory_hwm/1000000,
      (memory_hwm%1000000)/1000, memory_hwm%1000);
  else if (memory_hwm >= 1000)
    debug_printf("%d,%03d\n", memory_hwm/1000, memory_hwm%1000);
  else
    debug_printf("%d\n", memory_hwm);
  }

return yield? EXIT_SUCCESS : EXIT_FAILURE;
}

/* End of sdop.c */
