/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* Allocate storage and initialize global variables, apart from those that are
tables of some kind. Those are in datatables.c. */

#include "sdop.h"


/*************************************************
*           General global variables             *
*************************************************/

afontstr     *afont_last                 = NULL;
afontstr     *afont_list                 = NULL;

uschar       *author_address             = NULL;
uschar       *author_corpauthor          = NULL;
uschar       *author_firstname           = NULL;
uschar       *author_honorific           = NULL;
uschar       *author_initials            = NULL;
uschar       *author_jobtitle            = NULL;
uschar       *author_lineage             = NULL;
uschar       *author_orgname             = NULL;
uschar       *author_othername           = NULL;
uschar       *author_surname             = NULL;

int           background_colour[3]       = { 0, 0, 0 };
uschar       *book_cpyholder             = NULL;
uschar       *book_cpyyear               = NULL;
uschar       *book_date                  = NULL;
uschar       *book_edition               = NULL;
uschar       *book_issuenum              = NULL;
uschar       *book_pubdate               = NULL;
uschar       *book_publishername         = NULL;
uschar       *book_releaseinfo           = NULL;
uschar       *book_revauthorinitials     = NULL;
uschar       *book_revdate               = NULL;
uschar       *book_revnumber             = NULL;
uschar       *book_subtitle              = NULL;
uschar       *book_title                 = NULL;
uschar       *book_titleabbrev           = NULL;
uschar       *book_volumenum             = NULL;

unsigned int  debug_selector             = 0;
BOOL          debug_need_nl              = FALSE;
int           document_type              = DOC_UNSET;

uschar       *editor_firstname           = NULL;
uschar       *editor_honorific           = NULL;
uschar       *editor_initials            = NULL;
uschar       *editor_jobtitle            = NULL;
uschar       *editor_lineage             = NULL;
uschar       *editor_orgname             = NULL;
uschar       *editor_othername           = NULL;
uschar       *editor_surname             = NULL;
tree_node    *entity_tree                = NULL;
int           error_max                  = 0;
int           example_nformat_pcount     = 2;
int           extra_leading              = 0;

int           figure_nformat_pcount      = 2;

tree_node    *id_tree                    = NULL;
int           index_count                = 0;
uschar       *index_names[INDEXMAX];
unsigned int *index_sort_omit            = NULL;
uschar       *index_sort_omit_string     = NULL;
BOOL          inheadorfoot               = FALSE;
BOOL          internal_processing        = FALSE;

item         *legalnotice_item_list      = NULL;

item         *main_foot_item_list        = NULL;
item         *main_head_item_list        = NULL;
FILE         *main_hyphenfile            = NULL;
item         *main_item_list             = NULL;
memchainstr  *main_memory_chain          = NULL;
int           main_page_count            = 0;
int           memory_hwm                 = 0;
int           memory_used                = 0;

uschar       *othercredit_firstname      = NULL;
uschar       *othercredit_honorific      = NULL;
uschar       *othercredit_initials       = NULL;
uschar       *othercredit_jobtitle       = NULL;
uschar       *othercredit_lineage        = NULL;
uschar       *othercredit_orgname        = NULL;
uschar       *othercredit_othername      = NULL;
uschar       *othercredit_surname        = NULL;

int           page_columns               = 0;
int           page_columns_init          = 0;
int           page_columns_save          = 0;
int           page_colsep                = 0;
int           page_colsep_init           = 0;
int           page_colsep_save           = 0;
int           page_length                = 0;
BOOL          pages_even                 = FALSE;
pagelist     *pages_front                = NULL;
pagelist     *pages_main                 = NULL;
BOOL          pages_odd                  = FALSE;
BOOL          preface_even_pages         = TRUE;
item         *preface_foot_item_list     = NULL;
item         *preface_head_item_list     = NULL;
item         *preface_item_list          = NULL;
int           preface_page_count         = 0;

item         *read_addto                 = NULL;
BOOL          read_done                  = FALSE;
uschar       *read_filename              = NULL;
int           read_linenumber            = 0;
uschar       *read_what                  = NULL;
item         *revdescription_item_list   = NULL;

int           scale_typesize_base        = 11000;
int           scale_typesize_base_orig   = 11000;
uschar       *sdop_share                 = NULL;
BOOL          subscript_small            = TRUE;
int           subscript_down             = 33;
BOOL          superscript_small          = TRUE;
int           superscript_up             = 33;

int           table_nformat_pcount       = 2;
item         *title_item_list            = NULL;
int           title_page_count           = 0;

item         *toc_foot_item_list         = NULL;
item         *toc_head_item_list         = NULL;
item         *toc_item_list              = NULL;
int           toc_page_count             = 0;
pdfmarkstr   *toc_pdfmarks               = NULL;
uschar       *toc_title                  = US"Contents";

tree_node    *unknown_char_tree          = NULL;
tree_node    *unknown_element_tree       = NULL;

vfontstr     *vfont_last                 = NULL;
vfontstr     *vfont_list                 = NULL;

BOOL          warn_unsupported           = TRUE;
BOOL          warn_unsupported_set       = FALSE;

BOOL          warn_unsupported_chars     = TRUE;
BOOL          warn_unsupported_chars_set = FALSE;

int           xref_colour[3]             = { -1, -1, -1 };
BOOL          xref_links                 = TRUE;
int           xref_packed_colour         = -1;



/*************************************************
*       Parameters that control the output       *
*************************************************/

uschar       *type_families[] = { US"Times", US"Helvetica", US"Courier" };

vfontstr      booktitle1_vfont = { NULL, FFAM_SANSERIF, FTYPE_BOLD,     24000, 2000, 0, NULL, { NULL, NULL } };
vfontstr      booktitle2_vfont = { NULL, FFAM_SANSERIF, FTYPE_BOLD,     18000, 1500, 0, NULL, { NULL, NULL } };
vfontstr      booktitle3_vfont = { NULL, FFAM_SANSERIF, FTYPE_BOLD,     16000, 1250, 0, NULL, { NULL, NULL } };
vfontstr      booktitle4_vfont = { NULL, FFAM_SANSERIF, FTYPE_BOLD,     13000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      chapter_vfont    = { NULL, FFAM_SANSERIF, FTYPE_BOLD,     16000,    0, 0, NULL, { NULL, NULL } };
vfontstr      chapsubt_vfont   = { NULL, FFAM_SANSERIF, FTYPE_BOLD,     12000,    0, 0, NULL, { NULL, NULL } };
vfontstr      section_vfont    = { NULL, FFAM_SANSERIF, FTYPE_BOLD,     13000,    0, 0, NULL, { NULL, NULL } };
vfontstr      subsection_vfont = { NULL, FFAM_SANSERIF, FTYPE_BOLD,     11000,    0, 0, NULL, { NULL, NULL } };

vfontstr      blockquote_title_vfont = { NULL, FFAM_SERIF,  FTYPE_BOLD,     11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      example_title_vfont    = { NULL, FFAM_SERIF,  FTYPE_ITALIC,   11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      figure_title_vfont     = { NULL, FFAM_SERIF,  FTYPE_ITALIC,   11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      formalpara_title_vfont = { NULL, FFAM_SERIF,  FTYPE_BOLD,     11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      note_title_vfont       = { NULL, FFAM_SERIF,  FTYPE_ITALIC,   11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      sidebar_title_vfont    = { NULL, FFAM_SERIF,  FTYPE_BOLD,     11000, 1000, 0, NULL, { NULL, NULL } };

/* The list tag vfonts are reset from maintext_vfont at the start of the
appropriate scans because thay may change during a document. */

vfontstr      ilist_tag_vfont;
int           ilist_tag_colour[3];
vfontstr      olist_tag_vfont;
int           olist_tag_colour[3];

vfontstr      boittext_vfont = { NULL, FFAM_SERIF,    FTYPE_BOLDITALIC, 11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      boldtext_vfont = { NULL, FFAM_SERIF,    FTYPE_BOLD,       11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      italtext_vfont = { NULL, FFAM_SERIF,    FTYPE_ITALIC,     11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      maintext_vfont = { NULL, FFAM_SERIF,    FTYPE_ROMAN,      11000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      monoboit_vfont = { NULL, FFAM_MONO,     FTYPE_BOLDITALIC, 11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      monobold_vfont = { NULL, FFAM_MONO,     FTYPE_BOLD,       11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      monoital_vfont = { NULL, FFAM_MONO,     FTYPE_ITALIC,     11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      monotext_vfont = { NULL, FFAM_MONO,     FTYPE_ROMAN,      11000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      small_boittext_vfont = { NULL, FFAM_SERIF,    FTYPE_BOLDITALIC, 9000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      small_boldtext_vfont = { NULL, FFAM_SERIF,    FTYPE_BOLD,       9000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      small_italtext_vfont = { NULL, FFAM_SERIF,    FTYPE_ITALIC,     9000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      small_maintext_vfont = { NULL, FFAM_SERIF,    FTYPE_ROMAN,      9000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      small_monoboit_vfont = { NULL, FFAM_MONO,     FTYPE_BOLDITALIC, 9000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      small_monobold_vfont = { NULL, FFAM_MONO,     FTYPE_BOLD,       9000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      small_monoital_vfont = { NULL, FFAM_MONO,     FTYPE_ITALIC,     9000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      small_monotext_vfont = { NULL, FFAM_MONO,     FTYPE_ROMAN,      9000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      footnote_boittext_vfont = { NULL, FFAM_SERIF, FTYPE_BOLDITALIC, 9000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      footnote_boldtext_vfont = { NULL, FFAM_SERIF, FTYPE_BOLD,       9000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      footnote_italtext_vfont = { NULL, FFAM_SERIF, FTYPE_ITALIC,     9000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      footnote_maintext_vfont = { NULL, FFAM_SERIF, FTYPE_ROMAN,      9000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      footnote_monoboit_vfont = { NULL, FFAM_MONO,  FTYPE_BOLDITALIC, 9000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      footnote_monobold_vfont = { NULL, FFAM_MONO,  FTYPE_BOLD,       9000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      footnote_monoital_vfont = { NULL, FFAM_MONO,  FTYPE_ITALIC,     9000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      footnote_monotext_vfont = { NULL, FFAM_MONO,  FTYPE_ROMAN,      9000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      footnote_key_vfont      = { NULL, FFAM_SERIF, FTYPE_ROMAN,      7000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      headfoot_boittext_vfont = { NULL, FFAM_SERIF,    FTYPE_BOLDITALIC, 11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      headfoot_boldtext_vfont = { NULL, FFAM_SERIF,    FTYPE_BOLD,       11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      headfoot_italtext_vfont = { NULL, FFAM_SERIF,    FTYPE_ITALIC,     11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      headfoot_maintext_vfont = { NULL, FFAM_SERIF,    FTYPE_ROMAN,      11000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      headfoot_monoboit_vfont = { NULL, FFAM_MONO,     FTYPE_BOLDITALIC, 11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      headfoot_monobold_vfont = { NULL, FFAM_MONO,     FTYPE_BOLD,       11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      headfoot_monoital_vfont = { NULL, FFAM_MONO,     FTYPE_ITALIC,     11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      headfoot_monotext_vfont = { NULL, FFAM_MONO,     FTYPE_ROMAN,      11000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      table_title_vfont    = { NULL, FFAM_SERIF,    FTYPE_ITALIC, 11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      title_section_vfont  = { NULL, FFAM_SANSERIF, FTYPE_BOLD,   11500,    0, 0, NULL, { NULL, NULL } };

vfontstr      title_boittext_vfont = { NULL, FFAM_SERIF, FTYPE_BOLDITALIC, 11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      title_boldtext_vfont = { NULL, FFAM_SERIF, FTYPE_BOLD,       11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      title_italtext_vfont = { NULL, FFAM_SERIF, FTYPE_ITALIC,     11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      title_maintext_vfont = { NULL, FFAM_SERIF, FTYPE_ROMAN,      11000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      title_monoboit_vfont = { NULL, FFAM_MONO,  FTYPE_BOLDITALIC, 11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      title_monobold_vfont = { NULL, FFAM_MONO,  FTYPE_BOLD,       11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      title_monoital_vfont = { NULL, FFAM_MONO,  FTYPE_ITALIC,     11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      title_monotext_vfont = { NULL, FFAM_MONO,  FTYPE_ROMAN,      11000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      toc_fill_vfont = { NULL, FFAM_SERIF,    FTYPE_ROMAN,      11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr     *toc_fill_vfont_ptr = &toc_fill_vfont;

vfontstr      toc_boittext_vfont = { NULL, FFAM_SANSERIF, FTYPE_BOLDITALIC, 11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      toc_boldtext_vfont = { NULL, FFAM_SANSERIF, FTYPE_BOLD,       11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      toc_italtext_vfont = { NULL, FFAM_SANSERIF, FTYPE_ITALIC,     11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      toc_maintext_vfont = { NULL, FFAM_SANSERIF, FTYPE_ROMAN,      11000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      toc_monoboit_vfont = { NULL, FFAM_MONO, FTYPE_BOLDITALIC, 11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      toc_monobold_vfont = { NULL, FFAM_MONO, FTYPE_BOLD,       11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      toc_monoital_vfont = { NULL, FFAM_MONO, FTYPE_ITALIC,     11000, 1000, 0, NULL, { NULL, NULL } };
vfontstr      toc_monotext_vfont = { NULL, FFAM_MONO, FTYPE_ROMAN,      11000, 1000, 0, NULL, { NULL, NULL } };

vfontstr      index_section_vfont  = { NULL, FFAM_SANSERIF, FTYPE_BOLD,   11500,    0, 0, NULL, { NULL, NULL } };

vfontstr      index_boittext_vfont = { NULL, FFAM_SERIF, FTYPE_BOLDITALIC, 9600, 1000, 0, NULL, { NULL, NULL } };
vfontstr      index_boldtext_vfont = { NULL, FFAM_SERIF, FTYPE_BOLD,       9600, 1000, 0, NULL, { NULL, NULL } };
vfontstr      index_italtext_vfont = { NULL, FFAM_SERIF, FTYPE_ITALIC,     9600, 1000, 0, NULL, { NULL, NULL } };
vfontstr      index_maintext_vfont = { NULL, FFAM_SERIF, FTYPE_ROMAN,      9600, 1000, 0, NULL, { NULL, NULL } };

vfontstr      index_monoboit_vfont = { NULL, FFAM_MONO,  FTYPE_BOLDITALIC, 9600, 1000, 0, NULL, { NULL, NULL } };
vfontstr      index_monobold_vfont = { NULL, FFAM_MONO,  FTYPE_BOLD,       9600, 1000, 0, NULL, { NULL, NULL } };
vfontstr      index_monoital_vfont = { NULL, FFAM_MONO,  FTYPE_ITALIC,     9600, 1000, 0, NULL, { NULL, NULL } };
vfontstr      index_monotext_vfont = { NULL, FFAM_MONO,  FTYPE_ROMAN,      9600, 1000, 0, NULL, { NULL, NULL } };

/* Tables for doing things to the above fonts. First of all, there's a set of
tables, each of which contains 8 pointers to a set of fonts. The four mono ones
must come first, as they can be independently modified. The NULL at the end is
for a general scan that involves tables of varying lengths. */

vfontstr *main_vfonttable[] = {
  &monoboit_vfont,
  &monobold_vfont,
  &monoital_vfont,
  &monotext_vfont,
  &boittext_vfont,
  &boldtext_vfont,
  &italtext_vfont,
  &maintext_vfont,
  NULL };

vfontstr *small_main_vfonttable[] = {
  &small_monoboit_vfont,
  &small_monobold_vfont,
  &small_monoital_vfont,
  &small_monotext_vfont,
  &small_boittext_vfont,
  &small_boldtext_vfont,
  &small_italtext_vfont,
  &small_maintext_vfont,
  NULL };

vfontstr *footnote_vfonttable[] = {
  &footnote_monoboit_vfont,
  &footnote_monobold_vfont,
  &footnote_monoital_vfont,
  &footnote_monotext_vfont,
  &footnote_boittext_vfont,
  &footnote_boldtext_vfont,
  &footnote_italtext_vfont,
  &footnote_maintext_vfont,
  NULL };

vfontstr *title_vfonttable[] = {
  &title_monoboit_vfont,
  &title_monobold_vfont,
  &title_monoital_vfont,
  &title_monotext_vfont,
  &title_boittext_vfont,
  &title_boldtext_vfont,
  &title_italtext_vfont,
  &title_maintext_vfont,
  NULL };

vfontstr *index_vfonttable[] = {
  &index_monoboit_vfont,
  &index_monobold_vfont,
  &index_monoital_vfont,
  &index_monotext_vfont,
  &index_boittext_vfont,
  &index_boldtext_vfont,
  &index_italtext_vfont,
  &index_maintext_vfont,
  NULL };

vfontstr *headfoot_vfonttable[] = {
  &headfoot_monoboit_vfont,
  &headfoot_monobold_vfont,
  &headfoot_monoital_vfont,
  &headfoot_monotext_vfont,
  &headfoot_boittext_vfont,
  &headfoot_boldtext_vfont,
  &headfoot_italtext_vfont,
  &headfoot_maintext_vfont,
  NULL };

/* The mono fonts here are never used, but we create them in order to keep
the tables in the same format. */

vfontstr *toc_vfonttable[] = {
  &toc_monoboit_vfont,
  &toc_monobold_vfont,
  &toc_monoital_vfont,
  &toc_monotext_vfont,
  &toc_boittext_vfont,
  &toc_boldtext_vfont,
  &toc_italtext_vfont,
  &toc_maintext_vfont,
  NULL };

/* This final table is a list of all the individual fonts that are not part of
a set-of-eight. It is used when scaling all the fonts at once. */

vfontstr *individual_vfonttable[] = {
  &booktitle1_vfont,
  &booktitle2_vfont,
  &booktitle3_vfont,
  &booktitle4_vfont,
  &chapter_vfont,
  &figure_title_vfont,
  &footnote_key_vfont,
  &formalpara_title_vfont,
  &index_section_vfont,
  &section_vfont,
  &subsection_vfont,
  &table_title_vfont,
  &title_section_vfont,
  &toc_fill_vfont,
  NULL };

/* This table is a list of all the above tables, for use when scaling all the
fonts at once. */

vfontstr **vfonttables[] = {
  &main_vfonttable[0],
  &small_main_vfonttable[0],
  &footnote_vfonttable[0],
  &title_vfonttable[0],
  &index_vfonttable[0],
  &toc_vfonttable[0],
  &headfoot_vfonttable[0],
  &individual_vfonttable[0],
  NULL };


int           index_page_columns = 2;
int           index_page_colsep = 20000;

int           list_numeration_default = LN_ARABIC;

uschar       *example_number_format = US"%s-%s";
uschar       *example_title_format  = US"Example %s: ";
int           example_title_justify = J_UNSET;
int           example_title_width   = -1;

uschar       *figure_number_format = US"%s-%s";
uschar       *figure_title_format  = US"Figure %s: ";
int           figure_title_justify = J_UNSET;
int           figure_title_width = -1;

uschar       *table_number_format = US"%s-%s";
uschar       *table_title_format  = US"Table %s: ";
int           table_title_justify = J_UNSET;
int           table_title_width = -1;

BOOL          blockquote_ruled = FALSE;
int           blockquote_title_justify = J_CENTRE;

BOOL          note_ruled = TRUE;
int           note_title_justify = J_CENTRE;

BOOL          sidebar_ruled = FALSE;
int           sidebar_title_justify = J_CENTRE;

int           command_fs     = FS_ITALIC;
int           filename_fs    = FS_ITALIC;
int           function_fs    = FS_ITALIC;
int           option_fs      = FS_BOLD;
int           replaceable_fs = FS_ITALIC;
int           userinput_fs   = FS_MONO;
int           varname_fs     = FS_ITALIC;

uschar       *olist_format;

BOOL          chapter_skip_head = FALSE;

/* The "before" values for a chapter do not apply to normal chapters (or
appendixes etc.) because they always start at the top of a page. However, if
there are multiple colophons, they run on, and that's when the extra space
comes in. The chapter2 parameters are used when a subtitle follows a chapter
title. */

/*                                     before before  after  after    in1     in     en  justify    fill */
/*                                       max    min    max    min                                        */
layoutparam   chapter_layparm       = { 30000, 30000, 15000, 15000,     0,     0,     0, J_CENTRE, FALSE };
layoutparam   chapter2_layparm      = { 30000, 30000,  8000,  8000,     0,     0,     0, J_CENTRE, FALSE };
layoutparam   chapsubt_layparm      = {     0,     0, 15000, 15000,     0,     0,     0, J_CENTRE, FALSE };
layoutparam   section_layparm       = { 20000, 16000,     0,     0,     0,     0,     0, J_LEFT,   FALSE };
layoutparam   subsection_layparm    = { 18000, 16000,     0,     0,     0,     0,     0, J_LEFT,   FALSE };
layoutparam   example_title_layparm = { 11000,  6000, 11000, 11000,     0,     0,     0, J_LEFT,   TRUE };
layoutparam   figure_title_layparm  = { 11000,  6000, 11000, 11000,     0,     0,     0, J_CENTRE, TRUE };
layoutparam   formalpara_layparm    = { 11000,  6000,     0,     0,     0,     0,     0, J_BOTH,   TRUE };
layoutparam   para_layparm          = { 11000,  6000, 11000,  6000,     0,     0,     0, J_BOTH,   TRUE };
layoutparam   blockquote_layparm    = { 11000,  6000, 11000,  6000, 24000, 24000, 24000, J_BOTH,   TRUE };
layoutparam   note_layparm          = { 11000,  6000, 11000,  6000, 24000, 24000, 24000, J_BOTH,   TRUE };
layoutparam   ilistpara_layparm     = { 11000,  6000, 11000,  6000, 12000, 12000,     0, J_BOTH,   TRUE };
layoutparam   olistpara_layparm     = { 11000,  6000, 11000,  6000, 24000, 24000,     0, J_BOTH,   TRUE };
layoutparam   vlistpara_layparm     = { 11000,  6000, 11000,  6000, 14000, 14000,     0, J_BOTH,   TRUE };
layoutparam   vlistpara1_layparm    = {     0,     0, 11000,  6000, 14000, 14000,     0, J_BOTH,   TRUE };
layoutparam   vlisttitle_layparm    = { 11000,  6000, 11000,  6000,     0,     0,     0, J_BOTH,   TRUE };
layoutparam   sidebar_layparm       = { 11000,  6000, 11000,  6000, 24000, 24000, 24000, J_BOTH,   TRUE };
layoutparam   term_layparm          = { 11000,  6000,     0,     0,     0,     0,     0, J_BOTH,   TRUE };
layoutparam   termfirst_layparm     = { 11000,  6000,     0,     0, 12000, 12000,     0, J_BOTH,   TRUE };
layoutparam   termmid_layparm       = {     0,     0,     0,     0, 12000, 12000,     0, J_BOTH,   TRUE };
layoutparam   termlast_layparm      = {     0,     0,     0,     0, 12000, 12000,     0, J_BOTH,   TRUE };
layoutparam   literalpara_layparm   = { 11000,  6000, 11000,  6000, 12000, 12000,     0, J_LEFT,   FALSE };
layoutparam   table_layparm         = { 11000,  6000, 11000,  6000,     0,     0,     0,     NA,   NA };
layoutparam   framed_table_layparm  = { 20000, 16000, 13000, 11000,     0,     0,     0,     NA,   NA };
layoutparam   table_title_layparm   = { 11000,  6000,     0,     0,     0,     0,     0, J_LEFT,   TRUE };

layoutparam   ixsection_layparm     = { 21000, 14000,     0,     0,     0,     0,     0, J_LEFT,   FALSE };
layoutparam   ixpara_layparm        = {  1000,     0,     0,     0,     0, 12000,     0, J_LEFT,   TRUE };

/*                                             before before  after  after    in1     in     en  justify    fill */
/*                                               max    min    max    min                                        */
layoutparam   footnote_formalpara_layparm   = {  5000,  5000,     0,     0,     0,     0,     0, J_BOTH,   TRUE };
layoutparam   footnote_para_layparm         = {  5000,  5000,  5000,  5000,     0,     0,     0, J_BOTH,   TRUE };
layoutparam   footnote_blockquote_layparm   = {  5000,  5000,  5000,  5000, 21000, 21000, 21000, J_BOTH,   TRUE };
layoutparam   footnote_note_layparm         = {  5000,  5000,  5000,  5000, 21000, 21000, 21000, J_BOTH,   TRUE };
layoutparam   footnote_ilistpara_layparm    = {  5000,  5000,  5000,  5000, 10500, 10500,     0, J_BOTH,   TRUE };
layoutparam   footnote_olistpara_layparm    = {  5000,  5000,  5000,  5000, 21000, 21000,     0, J_BOTH,   TRUE };
layoutparam   footnote_vlistpara_layparm    = {  5000,  5000,  5000,  5000, 12000, 12000,     0, J_BOTH,   TRUE };
layoutparam   footnote_vlistpara1_layparm   = {     0,     0,  5000,  5000, 12000, 12000,     0, J_BOTH,   TRUE };
layoutparam   footnote_term_layparm         = {  5000,  5000,     0,     0, 10500, 10500,     0, J_BOTH,   TRUE };
layoutparam   footnote_termfirst_layparm    = {  5000,  5000,     0,     0, 10500, 10500,     0, J_BOTH,   TRUE };
layoutparam   footnote_termmid_layparm      = {     0,     0,     0,     0, 10500, 10500,     0, J_BOTH,   TRUE };
layoutparam   footnote_termlast_layparm     = {     0,     0,     0,     0, 10500, 10500,     0, J_BOTH,   TRUE };
layoutparam   footnote_literalpara_layparm  = {  5000,  5000,  5000,  5000, 10500, 10500,     0, J_LEFT,   FALSE };
layoutparam   footnote_sidebar_layparm      = {  5000,  5000,  5000,  5000, 21000, 21000, 21000, J_BOTH,   TRUE };
layoutparam   footnote_table_layparm        = {  5000,  5000,  5000,  5000,     0,     0,     0,     NA,   NA };
layoutparam   footnote_framed_table_layparm = { 17500, 14000, 11000,  6000,     0,     0,     0,     NA,   NA };

int           footnote_indent              = 10000;
int           footnote_line_thickness      = 500;
int           footnote_line_length         = 72000;
BOOL          footnote_overhead            = 10000;

int           global_align_default         = J_LEFT;
int           global_align_char_default    = '.';
int           global_align_charoff_default = 50;
BOOL          global_colsep_default        = TRUE;
BOOL          global_rowsep_default        = TRUE;
int           global_tableflags_default    = TDF_DEFAULT;

BOOL          index_headings_enabled       = TRUE;
BOOL          literal_indent_fudge         = TRUE;

BOOL          main_even_pages = FALSE;
hfstr         main_headfoot;
hfstr         main_headfoot_default = {
                                US"&arabicpage;",   /* foot_centre_recto */
                                US"&arabicpage;",   /* foot_centre_verso */
                                US"",               /* foot_left_recto */
                                US"",               /* foot_left_verso */
                                US"",               /* foot_right_recto */
                                US"",               /* foot_right_verso */
                                US"",               /* head_centre_recto */
                                US"",               /* head_centre_verso */
                                US"",               /* head_left_recto */
                                US"",               /* head_left_verso */
                                US"",               /* head_right_recto */
                                US"" };             /* head_right_verso */
int           margin_bottom         = 60000;
int           margin_left           = 0;
int           margin_left_recto     = 72000;
int           margin_left_verso     = 72000;

BOOL          number_sections[MAXSECTDEPTH];  /* Default in datatables. */

int           page_baroffset        = 10000;
int           page_barwidth         = 800;
int           page_foot_length      = 24000;
int           page_foot_linewidth   = 0;
int           page_full_length      = 720000;
int           page_head_length      = 0;
int           page_head_linewidth   = 0;
int           page_linewidth        = 450000;
int           page_main_linewidth   = 0;

uschar       *paper_size            = NULL;
double        paper_size_height     = 0.0;
double        paper_size_width      = 0.0;

hfstr         preface_headfoot = { US"&romanpage;",    /* foot_centre_recto */
                                   US"&romanpage;",    /* foot_centre_verso */
                                   US"",               /* foot_left_recto */
                                   US"",               /* foot_left_verso */
                                   US"",               /* foot_right_recto */
                                   US"",               /* foot_right_verso */
                                   US"",               /* head_centre_recto */
                                   US"",               /* head_centre_verso */
                                   US"",               /* head_left_recto */
                                   US"",               /* head_left_verso */
                                   US"",               /* head_right_recto */
                                   US"" };             /* head_right_verso */

int           table_bot_frame_space = 4000;
int           table_frame_thickness =  500;
int           table_left_col_space  = 5000;
int           table_overall_indent  = 0;
int           table_right_col_space = 5000;
int           table_row_sep_space   = 5000;
int           table_top_frame_space = 0;

BOOL          title_even_pages  = TRUE;

BOOL          toc_chapter_blanks[2]              = { TRUE, TRUE };
BOOL          toc_even_pages                     = TRUE;
uschar       *toc_fill_string     = US".";
int           toc_fill_leftspace  = 2000;
int           toc_fill_rightspace = 4000;
hfstr         toc_headfoot = { US"&romanpage;",    /* foot_centre_recto */
                               US"&romanpage;",    /* foot_centre_verso */
                               US"",               /* foot_left_recto */
                               US"",               /* foot_left_verso */
                               US"",               /* foot_right_recto */
                               US"",               /* foot_right_verso */
                               US"",               /* head_centre_recto */
                               US"",               /* head_centre_verso */
                               US"",               /* head_left_recto */
                               US"",               /* head_left_verso */
                               US"",               /* head_right_recto */
                               US"" };             /* head_right_verso */

BOOL          toc_sections[MAXSECTDEPTH];
BOOL          toc_printed_sections[MAXSECTDEPTH];
BOOL          toc_sections_default[MAXSECTDEPTH] = { TRUE, TRUE, TRUE };
BOOL          toc_sections_preface_default[MAXSECTDEPTH] = { TRUE };

/* If there are more deeply nested sections that are in the TOC, the later
ones use the last entry in the toc_line_strings vector. I failed to find how
to set this all up with a single initialization - got tangled in C incomplete
types - and so I set up subvectors instead. Even so, I still needed the casts
in the final vector to avoid compiler warnings about incompatible types. */

uschar  *toc_line_chapter_strings[TOC_LINE_STRING_COUNT] = {
         US"<emphasis role='bold'>",    /* Start of line */
         US"",                          /* Before number */
         US".&nbsp;&nbsp;",             /* After number */
         US"</emphasis>",               /* After title */
         US"<emphasis role='bold'>",    /* Before page number */
         US"</emphasis>" };             /* After page number */

uschar  *toc_line_sect1_strings[TOC_LINE_STRING_COUNT] = {
         US"&nbsp;&nbsp;&nbsp;&nbsp;",  /* Start of line */
         US"",                          /* Before number */
         US"&nbsp;&nbsp;",              /* After number */
         US"",                          /* After title */
         US"",                          /* Before page number */
         US"" };                        /* After page number */

uschar  *toc_line_sect2_strings[TOC_LINE_STRING_COUNT] = {
         US"&nbsp;&nbsp;&nbsp;&nbsp;"
           "&nbsp;&nbsp;&nbsp;&nbsp;",  /* Start of line */
         US"",                          /* Before number */
         US"&nbsp;&nbsp;",              /* After number */
         US"",                          /* After title */
         US"",                          /* Before page number */
         US"" };                        /* After page number */

uschar **toc_line_strings[] = {
         (uschar **)&toc_line_chapter_strings,
         (uschar **)&toc_line_sect1_strings,
         (uschar **)&toc_line_sect2_strings };

int      toc_line_string_count = sizeof(toc_line_strings)/
           sizeof(uschar **);

/* End of globals.c */
