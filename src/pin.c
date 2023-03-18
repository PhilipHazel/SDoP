/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains functions for handling "processing instructions". */

#include "sdop.h"

enum { PI_STRING, PI_TOC_LINE_STRING_LIST, PI_YESNO, PI_INTEGER, PI_FP,
       PI_DIMENSION, PI_SDIMENSION, PI_DIMOROBJ, PI_FONT, PI_FULLFONT,
       PI_FULLFONT4, PI_FULLFONT8, PI_ALIGN, PI_COLOUR, PI_INDENT };

typedef struct piparam {
  uschar     *name;         /* parameter name */
  BOOL       *orptr;        /* pointer to command line override switch */
  void       *pointer;      /* pointer to data variable */
  short int   type;         /* type of data variable */
  short int   obsolete;     /* non-zero if deprecated; relative index of new option */
} piparam;

/* Header/footer data for preface and body. These are checked when pages are
being printed so they can change value during printing. */

static piparam hfparam_list[] = {
  { US"foot_centre_recto", NULL, &(main_headfoot.foot_centre_recto), PI_STRING, 0 },
  { US"foot_centre_verso", NULL, &(main_headfoot.foot_centre_verso), PI_STRING, 0 },
  { US"foot_left_recto",   NULL, &(main_headfoot.foot_left_recto),   PI_STRING, 0 },
  { US"foot_left_verso",   NULL, &(main_headfoot.foot_left_verso),   PI_STRING, 0 },
  { US"foot_right_recto",  NULL, &(main_headfoot.foot_right_recto),  PI_STRING, 0 },
  { US"foot_right_verso",  NULL, &(main_headfoot.foot_right_verso),  PI_STRING, 0 },
  { US"head_centre_recto", NULL, &(main_headfoot.head_centre_recto), PI_STRING, 0 },
  { US"head_centre_verso", NULL, &(main_headfoot.head_centre_verso), PI_STRING, 0 },
  { US"head_left_recto",   NULL, &(main_headfoot.head_left_recto),   PI_STRING, 0 },
  { US"head_left_verso",   NULL, &(main_headfoot.head_left_verso),   PI_STRING, 0 },
  { US"head_right_recto",  NULL, &(main_headfoot.head_right_recto),  PI_STRING, 0 },
  { US"head_right_verso",  NULL, &(main_headfoot.head_right_verso),  PI_STRING, 0 },

  { US"preface_foot_centre_recto", NULL, &(preface_headfoot.foot_centre_recto), PI_STRING, 0 },
  { US"preface_foot_centre_verso", NULL, &(preface_headfoot.foot_centre_verso), PI_STRING, 0 },
  { US"preface_foot_left_recto",   NULL, &(preface_headfoot.foot_left_recto),   PI_STRING, 0 },
  { US"preface_foot_left_verso",   NULL, &(preface_headfoot.foot_left_verso),   PI_STRING, 0 },
  { US"preface_foot_right_recto",  NULL, &(preface_headfoot.foot_right_recto),  PI_STRING, 0 },
  { US"preface_foot_right_verso",  NULL, &(preface_headfoot.foot_right_verso),  PI_STRING, 0 },
  { US"preface_head_centre_recto", NULL, &(preface_headfoot.head_centre_recto), PI_STRING, 0 },
  { US"preface_head_centre_verso", NULL, &(preface_headfoot.head_centre_verso), PI_STRING, 0 },
  { US"preface_head_left_recto",   NULL, &(preface_headfoot.head_left_recto),   PI_STRING, 0 },
  { US"preface_head_left_verso",   NULL, &(preface_headfoot.head_left_verso),   PI_STRING, 0 },
  { US"preface_head_right_recto",  NULL, &(preface_headfoot.head_right_recto),  PI_STRING, 0 },
  { US"preface_head_right_verso",  NULL, &(preface_headfoot.head_right_verso),  PI_STRING, 0 },

  { US"chapter_skip_head", NULL, &chapter_skip_head, PI_YESNO, 0 }
};

static int hfparam_list_count = sizeof(hfparam_list)/sizeof(piparam);

/* Multicolumning data */

static piparam mcparam_list[] = {
  { US"page_columns",           NULL, &page_columns, PI_INTEGER,   0 },
  { US"page_column_separation", NULL, &page_colsep,  PI_DIMENSION, 0 }
};

static int mcparam_list_count = sizeof(mcparam_list)/sizeof(piparam);

/* Things that change while creating pages */

static piparam cpparam_list[] = {
  { US"index_headings",  NULL, &index_headings_enabled, PI_YESNO, 0 }
};

static int cpparam_list_count = sizeof(cpparam_list)/sizeof(piparam);

/* Itemized and ordered list parameters */

static piparam list_param_list[] = {
  { US"font_ilist_tag",      NULL, &ilist_tag_vfont,        PI_FULLFONT, 0 },
  { US"font_olist_tag",      NULL, &olist_tag_vfont,        PI_FULLFONT, 0 },
  { US"ilist_font",          NULL, &(ilist_tag_vfont.type), PI_FONT,     2 },
  { US"ilist_tag_rgb",       NULL, &ilist_tag_colour[0],    PI_COLOUR,   0 },
  { US"ilist_tag_style",     NULL, &(ilist_tag_vfont.type), PI_FONT,     0 },
  { US"olist_font",          NULL, &(olist_tag_vfont.type), PI_FONT,     4 },
  { US"olist_format",        NULL, &olist_format,           PI_STRING,   1 },
  { US"olist_tag_format",    NULL, &olist_format,           PI_STRING,   0 },
  { US"olist_tag_rgb",       NULL, &olist_tag_colour[0],    PI_COLOUR,   0 },
  { US"olist_tag_style",     NULL, &(olist_tag_vfont.type), PI_FONT,     0 },
  { US"orderedlist_format",  NULL, &olist_format,           PI_STRING,  -3 }
};

static int list_param_list_count = sizeof(list_param_list)/sizeof(piparam);

/* Figure, example, and table title format parameters */

static piparam figtabparam_list[] = {
  { US"example_number_format",  NULL, &example_number_format,  PI_STRING,  0 },
  { US"example_title_format",   NULL, &example_title_format,   PI_STRING,  0 },
  { US"figure_number_format",   NULL, &figure_number_format,   PI_STRING,  0 },
  { US"figure_title_format",    NULL, &figure_title_format,    PI_STRING,  0 },
  { US"table_number_format",    NULL, &table_number_format,    PI_STRING,  0 },
  { US"table_title_format",     NULL, &table_title_format,     PI_STRING,  0 }
};

static int figtabparam_list_count = sizeof(figtabparam_list)/sizeof(piparam);

/* Figure and example title layout paramters */

static piparam figexlayoutparam_list[] = {
  { US"example_title_justify",  NULL, &example_title_justify,  PI_ALIGN,    0 },
  { US"example_title_width",    NULL, &example_title_width,    PI_DIMOROBJ, 0 },
  { US"figure_title_justify",   NULL, &figure_title_justify,   PI_ALIGN,    0 },
  { US"figure_title_width",     NULL, &figure_title_width,     PI_DIMOROBJ, 0 }
};

static int figexlayoutparam_list_count = sizeof(figexlayoutparam_list)/sizeof(piparam);

/* Table title layout parameters */

static piparam tablelayoutparam_list[] = {
  { US"table_indent",           NULL, &table_overall_indent,   PI_DIMENSION, 0 },
  { US"table_title_justify",    NULL, &table_title_justify,    PI_ALIGN,     0 },
  { US"table_title_width",      NULL, &table_title_width,      PI_DIMOROBJ,  0 }
};

static int tablelayoutparam_list_count = sizeof(tablelayoutparam_list)/sizeof(piparam);

/* General layout parameters that apply throughout the document. */

static piparam layparm_list[] = {
  { US"blockquote_indent",        NULL, &blockquote_layparm,       PI_INDENT, 0 },
  { US"blockquote_ruled",         NULL, &blockquote_ruled,         PI_YESNO,  0 },
  { US"blockquote_title_justify", NULL, &blockquote_title_justify, PI_ALIGN,  0 },
  { US"formalpara_indent",        NULL, &formalpara_layparm,       PI_INDENT, 0 },
  { US"ilist_indent",             NULL, &ilistpara_layparm,        PI_INDENT, 0 },
  { US"literal_indent",           NULL, &literalpara_layparm,      PI_INDENT, 0 },
  { US"note_indent",              NULL, &note_layparm,             PI_INDENT, 0 },
  { US"note_ruled",               NULL, &note_ruled,               PI_YESNO,  0 },
  { US"note_title_justify",       NULL, &note_title_justify,       PI_ALIGN,  0 },
  { US"olist_indent",             NULL, &olistpara_layparm,        PI_INDENT, 0 },
  { US"para_indent",              NULL, &para_layparm,             PI_INDENT, 0 },
  { US"sidebar_indent",           NULL, &sidebar_layparm,          PI_INDENT, 0 },
  { US"sidebar_ruled",            NULL, &sidebar_ruled,            PI_YESNO,  0 },
  { US"sidebar_title_justify",    NULL, &sidebar_title_justify,    PI_ALIGN,  0 },
  { US"term_indent",              NULL, &term_layparm,             PI_INDENT, 2 },
  { US"vlist_indent",             NULL, &vlistpara_layparm,        PI_INDENT, 0 },
  { US"vlist_term_indent",        NULL, &term_layparm,             PI_INDENT, 0 },
  { US"vlist_title_indent",       NULL, &vlisttitle_layparm,       PI_INDENT, 0 }
};

static int layparm_list_count = sizeof(layparm_list)/sizeof(piparam);

/* Layout parameters that can change during paragraph creation. */

static piparam dynamic_layparm_list[] = {
  { US"extra_leading",            NULL, &extra_leading,                    PI_SDIMENSION,   0 },
};

static int dynamic_layparm_list_count = sizeof(dynamic_layparm_list)/sizeof(piparam);

/* Subscript and superscript font sizes can be changed when assigning fonts. */

static piparam subsupparam_list[] = {
  { US"subscript_small",          NULL, &subscript_small,                  PI_YESNO,        0 },
  { US"superscript_small",        NULL, &superscript_small,                PI_YESNO,        0 }
};

static int subsupparam_list_count = sizeof(subsupparam_list)/sizeof(piparam);

/* Subscript and superscript positions can change during their assignment. */

static piparam subsuplevel_list[] = {
  { US"subscript_down",           NULL, &subscript_down,                   PI_INTEGER,      0 },
  { US"superscript_up",           NULL, &superscript_up,                   PI_INTEGER,      0 }
};

static int subsuplevel_list_count = sizeof(subsuplevel_list)/sizeof(piparam);

/* Global overall data that is set once and for all at the start of processing
and cannot vary for different parts of the document. */

static piparam glparam_list[] = {
  { US"background_rgb",           NULL, &background_colour[0],             PI_COLOUR,      0 },
  { US"command_font",             NULL, &command_fs,                       PI_FONT,        1 },
  { US"command_style",            NULL, &command_fs,                       PI_FONT,        0 },
  { US"font_blockquote_title",    NULL, &blockquote_title_vfont,           PI_FULLFONT,    0 },
  { US"font_booktitle1",          NULL, &booktitle1_vfont,                 PI_FULLFONT,    0 },
  { US"font_booktitle2",          NULL, &booktitle2_vfont,                 PI_FULLFONT,    0 },
  { US"font_booktitle3",          NULL, &booktitle3_vfont,                 PI_FULLFONT,    0 },
  { US"font_booktitle4",          NULL, &booktitle4_vfont,                 PI_FULLFONT,    0 },
  { US"font_chapter",             NULL, &chapter_vfont,                    PI_FULLFONT,    0 },
  { US"font_chapter_subtitle",    NULL, &chapsubt_vfont,                   PI_FULLFONT,    0 },
  { US"font_figure_title",        NULL, &figure_title_vfont,               PI_FULLFONT,    0 },
  { US"font_footnote",            NULL, &footnote_vfonttable,              PI_FULLFONT8,   0 },
  { US"font_footnote_key",        NULL, &footnote_key_vfont,               PI_FULLFONT,    0 },
  { US"font_footnotemono",        NULL, &footnote_vfonttable,              PI_FULLFONT8,   0 },
  { US"font_formalpara_title",    NULL, &formalpara_title_vfont,           PI_FULLFONT,    0 },
  { US"font_headfoot",            NULL, &headfoot_vfonttable,              PI_FULLFONT8,   0 },
  { US"font_headfootmono",        NULL, &headfoot_vfonttable,              PI_FULLFONT4,   0 },
  { US"font_index",               NULL, &index_vfonttable,                 PI_FULLFONT8,   0 },
  { US"font_index_section",       NULL, &index_section_vfont,              PI_FULLFONT,    0 },
  { US"font_indexmono",           NULL, &index_vfonttable,                 PI_FULLFONT4,   0 },
  { US"font_main",                NULL, &main_vfonttable,                  PI_FULLFONT8,   0 },
  { US"font_mainmono",            NULL, &main_vfonttable,                  PI_FULLFONT4,   0 },
  { US"font_note_title",          NULL, &note_title_vfont,                 PI_FULLFONT,    0 },
  { US"font_section",             NULL, &section_vfont,                    PI_FULLFONT,    0 },
  { US"font_sidebar_title",       NULL, &sidebar_title_vfont,              PI_FULLFONT,    0 },
  { US"font_small_main",          NULL, &small_main_vfonttable,            PI_FULLFONT8,   0 },
  { US"font_small_mainmono",      NULL, &small_main_vfonttable,            PI_FULLFONT4,   0 },
  { US"font_subsection",          NULL, &subsection_vfont,                 PI_FULLFONT,    0 },
  { US"font_table_title",         NULL, &table_title_vfont,                PI_FULLFONT,    0 },
  { US"font_title",               NULL, &title_vfonttable,                 PI_FULLFONT8,   0 },
  { US"font_title_section",       NULL, &title_section_vfont,              PI_FULLFONT,    0 },
  { US"font_titlemono",           NULL, &title_vfonttable,                 PI_FULLFONT4,   0 },
  { US"font_toc",                 NULL, &toc_vfonttable,                   PI_FULLFONT8,   0 },
  { US"font_toc_fill",            NULL, &toc_fill_vfont,                   PI_FULLFONT,    0 },
  { US"font_tocmono",             NULL, &toc_vfonttable,                   PI_FULLFONT4,   0 },
  { US"foot_length",              NULL, &page_foot_length,                 PI_DIMENSION,   0 },
  { US"function_font",            NULL, &function_fs,                      PI_FONT,        1 },
  { US"function_style",           NULL, &function_fs,                      PI_FONT,        0 },
  { US"head_length",              NULL, &page_head_length,                 PI_DIMENSION,   0 },
  { US"index_sort_omit",          NULL, &index_sort_omit_string,           PI_STRING,      0 },
  { US"literal_indent_fudge",     NULL, &literal_indent_fudge,             PI_YESNO,       0 },
  { US"main_even_pages",          NULL, &main_even_pages,                  PI_YESNO,       0 },
  { US"margin_bottom",            NULL, &margin_bottom,                    PI_DIMENSION,   0 },
  { US"margin_left_recto",        NULL, &margin_left_recto,                PI_DIMENSION,   0 },
  { US"margin_left_verso",        NULL, &margin_left_verso,                PI_DIMENSION,   0 },
  { US"monospace_family",         NULL, &(type_families[2]),               PI_STRING,      0 },
  { US"option_font",              NULL, &option_fs,                        PI_FONT,        1 },
  { US"option_style",             NULL, &option_fs,                        PI_FONT,        0 },
  { US"page_foot_line_width",     NULL, &page_foot_linewidth,              PI_DIMENSION,   0 },
  { US"page_full_length",         NULL, &page_full_length,                 PI_DIMENSION,   0 },
  { US"page_head_line_width",     NULL, &page_head_linewidth,              PI_DIMENSION,   0 },
  { US"page_line_width",          NULL, &page_linewidth,                   PI_DIMENSION,   0 },
  { US"paper_size",               NULL, &paper_size,                       PI_STRING,      0 },
  { US"replaceable_font",         NULL, &replaceable_fs,                   PI_FONT,        1 },
  { US"replaceable_style",        NULL, &replaceable_fs,                   PI_FONT,        0 },
  { US"sanserif_family",          NULL, &(type_families[1]),               PI_STRING,      0 },
  { US"scale_typesize_base",      NULL, &scale_typesize_base,              PI_FP,          0 },
  { US"serif_family",             NULL, &(type_families[0]),               PI_STRING,      0 },
  { US"toc_even_pages",           NULL, &toc_even_pages,                   PI_YESNO,       0 },
  { US"toc_fill_leftspace",       NULL, &toc_fill_leftspace,               PI_DIMENSION,   0 },
  { US"toc_fill_rightspace",      NULL, &toc_fill_rightspace,              PI_DIMENSION,   0 },
  { US"toc_fill_string",          NULL, &toc_fill_string,                  PI_STRING,      0 },
  { US"toc_foot_centre_recto",    NULL, &(toc_headfoot.foot_centre_recto), PI_STRING,      0 },
  { US"toc_foot_centre_verso",    NULL, &(toc_headfoot.foot_centre_verso), PI_STRING,      0 },
  { US"toc_foot_left_recto",      NULL, &(toc_headfoot.foot_left_recto),   PI_STRING,      0 },
  { US"toc_foot_left_verso",      NULL, &(toc_headfoot.foot_left_verso),   PI_STRING,      0 },
  { US"toc_foot_right_recto",     NULL, &(toc_headfoot.foot_right_recto),  PI_STRING,      0 },
  { US"toc_foot_right_verso",     NULL, &(toc_headfoot.foot_right_verso),  PI_STRING,      0 },
  { US"toc_head_centre_recto",    NULL, &(toc_headfoot.head_centre_recto), PI_STRING,      0 },
  { US"toc_head_centre_verso",    NULL, &(toc_headfoot.head_centre_verso), PI_STRING,      0 },
  { US"toc_head_left_recto",      NULL, &(toc_headfoot.head_left_recto),   PI_STRING,      0 },
  { US"toc_head_left_verso",      NULL, &(toc_headfoot.head_left_verso),   PI_STRING,      0 },
  { US"toc_head_right_recto",     NULL, &(toc_headfoot.head_right_recto),  PI_STRING,      0 },
  { US"toc_head_right_verso",     NULL, &(toc_headfoot.head_right_verso),  PI_STRING,      0 },
  { US"toc_line_chapter_strings", NULL, toc_line_chapter_strings,          PI_TOC_LINE_STRING_LIST, 0 },
  { US"toc_line_sect1_strings",   NULL, toc_line_sect1_strings,            PI_TOC_LINE_STRING_LIST, 0 },
  { US"toc_line_sect2_strings",   NULL, toc_line_sect2_strings,            PI_TOC_LINE_STRING_LIST, 0 },
  { US"toc_title",                NULL, &toc_title,                        PI_STRING,      0 },
  { US"userinput_font",           NULL, &userinput_fs,                     PI_FONT,        1 },
  { US"userinput_style",          NULL, &userinput_fs,                     PI_FONT,        0 },
  { US"varname_font",             NULL, &varname_fs,                       PI_FONT,        1 },
  { US"varname_style",            NULL, &varname_fs,                       PI_FONT,        0 },
  { US"warn_unsupported",         &warn_unsupported_set,
                                        &warn_unsupported,                 PI_YESNO,       0 },
  { US"warn_unsupported_characters", &warn_unsupported_chars_set,
                                        &warn_unsupported_chars,           PI_YESNO,       0 },
  { US"xref_links",               NULL, &xref_links,                       PI_YESNO,       0 },
  { US"xref_rgb",                 NULL, &xref_colour[0],                   PI_COLOUR,      0 }
};

static int glparam_list_count = sizeof(glparam_list)/sizeof(piparam);


/* Data for values of type PI_FONT, which map to font flags. */

typedef struct fstype {
  uschar *name;            /* "italic", "bold", etc. */
  int    type;             /* corresponding FS_xxx value */
} fstype;

static fstype fstype_list[] = {
  { US"bold",            FS_BOLD },
  { US"bolditalic",      FS_BOLD + FS_ITALIC },
  { US"italic",          FS_ITALIC },
  { US"mono",            FS_MONO },
  { US"monobold",        FS_MONO + FS_BOLD },
  { US"monobolditalic",  FS_MONO + FS_BOLD + FS_ITALIC },
  { US"monoitalic",      FS_MONO + FS_ITALIC },
  { US"roman",           FS_ROMAN }
};

static int fstype_list_count = sizeof(fstype_list)/sizeof(fstype);


/* Data for font descriptions */

typedef struct fdata {
  uschar *name;
  int    length;
  int    value;
} fdata;

static fdata fdata_family[] = {
  { US"serif",       sizeof("serif") - 1,      FFAM_SERIF },
  { US"sanserif",    sizeof("sanserif") - 1,   FFAM_SANSERIF },
  { US"monospaced",  sizeof("monospaced") - 1, FFAM_MONO }
};

static int fdata_family_count = sizeof(fdata_family)/sizeof(fdata);

static fdata fdata_type[] = {
  { US"roman",       sizeof("roman") - 1,      FTYPE_ROMAN },
  { US"italic",      sizeof("italic") - 1,     FTYPE_ITALIC },
  { US"bolditalic",  sizeof("bolditalic") - 1, FTYPE_BOLDITALIC },
  { US"bold",        sizeof("bold") - 1,       FTYPE_BOLD }
};

static int fdata_type_count = sizeof(fdata_type)/sizeof(fdata);


/* A few flags can change during processing, and therefore be different in
different parts of the file. They are all represented as bits in a single flag
integer. */

typedef struct pftype {
  uschar   *name;
  uint32_t  flagvalue;
} pftype;

static pftype pftype_list[] = {
  { US"hyphenate", PIN_HYPH },
  { US"kern",      PIN_KERN },
};

static int pftype_list_count = sizeof(pftype_list)/sizeof(pftype);



/*************************************************
*          Parse a string describing a font      *
*************************************************/

/* A font string is of the form "<size>,<leading>,<family>,<type>". For example
"14,2,serif,bold". All items can be omitted. The value -1 is returned for those
that are not given. When this is used for global settings, font sizes are not
scaled, as the final scaling is applied to all globals fonts after the globals
have been processed. However local font size settings must be scaled. This is
handled by the "scale" argument.

Arguments:
  s             the string
  psize         where to put the size
  pleading      where to put the leading
  pfamily       where to put the family
  ptype         where to put the type
  scale         TRUE if font sizes are to be scaled

Returns:        TRUE if no problem
                FALSE if an error has been generated
*/

static BOOL
parse_font(uschar *s, int *psize, int *pleading, int *pfamily, int *ptype,
  BOOL scale)
{
int k;

*psize = *pleading = *pfamily = *ptype = -1;

for (k = 0; k < 2; k++)
  {
  while (isspace(*s)) s++;
  if (*s == 0) return TRUE;
  if (*s != ',')
    {
    if (k == 0)
      {
      *psize = misc_get_fp(s, &s);
      if (scale)
        *psize = MULDIV(*psize, scale_typesize_base, scale_typesize_base_orig);
      }
    else
      {
      *pleading = misc_get_fp(s, &s);
      if (scale)
        *pleading = MULDIV(*pleading, scale_typesize_base,
          scale_typesize_base_orig);
      }
    }
  if (*s != 0 && *s++ != ',') return error(91);
  }

while (isspace(*s)) s++;
if (*s == 0) return TRUE;

if (*s != ',')
  {
  for (k = 0; k < fdata_family_count; k++)
    {
    if (Ustrncmp(s, fdata_family[k].name, fdata_family[k].length) == 0)
      {
      s += fdata_family[k].length;
      if (*s != 0 && *s++ != ',') return error(91);
      *pfamily = fdata_family[k].value;
      break;
      }
    }
  if (k == fdata_family_count) return error(91);
  }
else s++;

while (isspace(*s)) s++;
if (*s == 0) return TRUE;

for (k = 0; k < fdata_type_count; k++)
  {
  if (Ustrncmp(s, fdata_type[k].name, fdata_type[k].length) == 0)
    {
    s += fdata_type[k].length;
    while (isspace(*s)) s++;
    if (*s != 0) return error(91);
    *ptype = fdata_type[k].value;
    break;
    }
  }
if (k == fdata_type_count) return error(91);

return TRUE;
}


/*************************************************
*     Check list of parameters and set values    *
*************************************************/

/* Given an <?sdop?> processesing element, scan a list of parameters, and for
any that are mentioned in the element, set their values. It is possible,
however, for some to be overriden on the command line, in which case internal
setting does not happen.

Arguments:
  i          the ?sdop element
  pilist     the list to scan
  count      the length of the list
  scale      TRUE if font sizes are to be scaled
  warnobs    TRUE if warnings for obsolete parameters are wanted

Returns:     nothing
*/

static void
check_pin_list(item *i, piparam *pilist, int count, BOOL scale, BOOL warnobs)
{
int j, k, x;
uschar *s;

for (j = 0; j < count; j++)
  {
  int typesize, typeleading, typefamily, typetype;
  paramstr *p = misc_param_find(i, pilist[j].name);

  /* Warn for deprecated processing elements */

  if (warnobs && p != NULL && pilist[j].obsolete != 0)
    (void)error(115, pilist[j].name, pilist[j+pilist[j].obsolete].name);

  if (p == NULL ||                   /* Parameter not set here, or */
      (pilist[j].orptr != NULL &&    /* there's a command line override and */
        *(pilist[j].orptr)))         /* it is set. */
    continue;

  switch(pilist[j].type)
    {
    case PI_COLOUR:
    (void)misc_get_colour(p->value, (int *)(pilist[j].pointer));
    break;

    case PI_FONT:
    for (k = 0; k < fstype_list_count; k++)
      {
      if (Ustrcmp(p->value, fstype_list[k].name) == 0)
        {
        *((int *)(pilist[j].pointer)) = fstype_list[k].type;
        DEBUG(D_param) debug_printf("%s=%s\n", p->name, p->value);
        break;
        }
      }
    if (k >= fstype_list_count) error(31, p->value, p->name);
    break;

    case PI_FULLFONT:
    if (parse_font(p->value, &typesize, &typeleading, &typefamily,
        &typetype, scale))
      {
      vfontstr *v = (vfontstr *)(pilist[j].pointer);
      if (typesize > 0) v->size = typesize;
      if (typeleading >= 0) v->leading = typeleading;
      if (typefamily >= 0) v->family = typefamily;
      if (typetype >= 0) v->type = typetype;
      }
    break;

    case PI_FULLFONT4:
    case PI_FULLFONT8:
    if (parse_font(p->value, &typesize, &typeleading, &typefamily,
        &typetype, scale))
      {
      vfontstr **vv;
      int fontcount = (pilist[j].type == PI_FULLFONT4)? 4 : 8;
      if (typetype >= 0) error(90);
      for (vv = (vfontstr **)(pilist[j].pointer); fontcount-- > 0; vv++)
        {
        vfontstr *v = *vv;
        if (typesize > 0) v->size = typesize;
        if (typeleading >= 0) v->leading = typeleading;
        if (typefamily >= 0 && fontcount < 4) v->family = typefamily;
        }
      }
    break;

    case PI_DIMOROBJ:
    if (Ustrcmp(p->value, "object") == 0)
      {
      *((int *)(pilist[j].pointer)) = -1;
      break;
      }
    /* Fall through */
    /* vvvvvvvvvvvv */
    case PI_DIMENSION:
    x = misc_get_dimension(p->value);
    if (x < 0) error(51, "dimension", p->value); else
      {
      *((int *)(pilist[j].pointer)) = x;
      DEBUG(D_param) debug_printf("%s=%d\n", pilist[j].name, x);
      }
    break;

    case PI_SDIMENSION:
      {
      int sign = 1;
      uschar *t = p->value;
      if (*t == '-') { sign = -1; t++; }
        else if (*t == '+') t++;
      x = misc_get_dimension(t);
      if (x < 0) error(51, "dimension", p->value); else
        {
        *((int *)(pilist[j].pointer)) = x*sign;
        DEBUG(D_param) debug_printf("%s=%d\n", pilist[j].name, x*sign);
        }
      }
    break;

    case PI_INDENT:
     {
     int d[3];
     if (misc_get_dimensions(3, p->value, &d[0], TRUE))
       {
       layoutparam *lp = (layoutparam *)(pilist[j].pointer);
       lp->indent1 = d[0];
       lp->indent  = d[1];
       lp->endent  = d[2];
       }
     }
    break;

    case PI_INTEGER:
    x = misc_get_number(p->value);
    if (x < 0) error(51, "number", p->value); else
      {
      *((int *)(pilist[j].pointer)) = x;
      DEBUG(D_param) debug_printf("%s=%d\n", pilist[j].name, x);
      }
    break;

    case PI_FP:
    x = misc_get_fp(p->value, &s);
    if (x <= 0 || *s != 0) error(51, "point size", pilist[j].name); else
      {
      *((int *)(pilist[j].pointer)) = x;
      DEBUG(D_param) debug_printf("%s=%d\n", pilist[j].name, x);
      }
    break;

    case PI_STRING:
    *((uschar **)(pilist[j].pointer)) = p->value;
    DEBUG(D_param) debug_printf("%s=\"%s\"\n", pilist[j].name, p->value);
    break;

    case PI_TOC_LINE_STRING_LIST:
      {
      uschar **lp = (uschar **)(pilist[j].pointer);
      s = p->value;
      for (k = 0; k < TOC_LINE_STRING_COUNT; k++)
        {
        uschar *t;
        for (t = s; *t != 0 && *t != ','; t++)
          { if (*t == '&' && t[1] == ',') t++; }
        lp[k] = t = (uschar *)misc_malloc(t - s + 1);
        while (*s != 0 && *s != ',')
          {
          if (*s == '&' && s[1] == ',') s++;
          *t++ = *s++;
          }
        *t = 0;
        DEBUG(D_param)
          debug_printf("%s[%d]=\"%s\"\n", pilist[j].name, k, lp[k]);
        if (*s == ',') s++;
        }
      if (*s != 0) error(52, p->value, TOC_LINE_STRING_COUNT);
      }
    break;

    case PI_YESNO:
    if (Ustrcmp(p->value, US"yes") == 0)
      {
      *((BOOL *)(pilist[j].pointer)) = TRUE;
      DEBUG(D_param) debug_printf("%s=true\n", pilist[j].name);
      }
    else if (Ustrcmp(p->value, US"no") == 0)
      {
      *((BOOL *)(pilist[j].pointer)) = FALSE;
      DEBUG(D_param) debug_printf("%s=false\n", pilist[j].name);
      }
    else
      error(31, p->value, p->name);
    break;

    case PI_ALIGN:
    if (Ustrcmp(p->value, "left") == 0) *((int *)(pilist[j].pointer)) = J_LEFT;
    else if (Ustrcmp(p->value, "right") == 0) *((int *)(pilist[j].pointer)) = J_RIGHT;
    else if (Ustrcmp(p->value, "centre") == 0) *((int *)(pilist[j].pointer)) = J_CENTRE;
    else if (Ustrcmp(p->value, "center") == 0) *((int *)(pilist[j].pointer)) = J_CENTRE;
    else if (Ustrcmp(p->value, "both") == 0) *((int *)(pilist[j].pointer)) = J_BOTH;
    else error(87, p->value);
    break;
    }
  }
}



/*************************************************
*        Initialize text processing flags        *
*************************************************/

/* This function is called before processing the item list for paragraph
creation.

Arguments:  none
Returns:    default flags
*/

unsigned int
pin_init_flags(void)
{
return PIN_HYPH | PIN_KERN | PIN_HARDOF | PIN_SOFTOF;
}



/*************************************************
*         Change text processing flags           *
*************************************************/

/* This function is called for each ?sdop item when the list is being processed
for paragraph creation. One option has multiple values; the remainder are just
yes/no, and can be handled by a table.

Arguments:
  i         the ?sdop item
  f         the current flags

Returns:    the modified flags
*/

uint32_t
pin_change_flags(item *i, uint32_t f)
{
int j;
paramstr *p;

if ((p = misc_param_find(i, US"table_warn_overflow")) != NULL)
  {
  if (Ustrcmp(p->value, "always") == 0)
    f |= PIN_HARDOF;
  else if (Ustrcmp(p->value, "never") == 0)
    f &= ~ (PIN_HARDOF | PIN_SOFTOF);
  else if (Ustrcmp(p->value, "overprint") == 0)
    f = (f & ~PIN_HARDOF) | PIN_SOFTOF;
  else (void)error(31, p->value, "table_warn_overflow");
  }

else for (j = 0; j < pftype_list_count; j++)
  {
  if ((p = misc_param_find(i, pftype_list[j].name)) != NULL)
    {
    if (Ustrcmp(p->value, US"yes") == 0) f |= pftype_list[j].flagvalue;
    else if (Ustrcmp(p->value, US"no") == 0) f &= ~pftype_list[j].flagvalue;
    else (void)error(31, p->value, pftype_list[j].name);
    }
  }

return f;
}


/*************************************************
*                 Copy indents                   *
*************************************************/

/* Copy the indent settings from one layout parameter block to another. Called
from pin_change_layparm() below.

Arguments:
  lpt         the destination
  lpf         the source

Returns:      nothing
*/

static void
copy_indents(layoutparam *lpt, layoutparam *lpf)
{
lpt->indent1 = lpf->indent1;
lpt->indent = lpf->indent;
lpt->endent = lpf->endent;
}



/*************************************************
*           Change indent information            *
*************************************************/

/* This function is called during global PIN processing.

Argument:   the ?sdop item
Returns:    nothing
*/

void
pin_change_layparm(item *i)
{
check_pin_list(i, layparm_list, layparm_list_count, FALSE, TRUE);

/* Some indents need copying into alternate blocks (which are typically used so
they can have different spacing settings). */

copy_indents(&vlistpara1_layparm, &vlistpara_layparm);
copy_indents(&termfirst_layparm,  &term_layparm);
copy_indents(&termmid_layparm,    &term_layparm);
copy_indents(&termlast_layparm,   &term_layparm);
}


/*************************************************
*             Dynamic font changing              *
*************************************************/

/* This function is called during font assignment. At present, it affects only
the sizes of subscript and superscript fonts.

Argument:   the ?sdop item
Returns:    nothing
*/

void
pin_change_font_assign(item *i)
{
check_pin_list(i, subsupparam_list, subsupparam_list_count, FALSE, TRUE);
}



/*************************************************
*       Change multicolumning information        *
*************************************************/

/* This function is called for each ?sdop item when a list is being processed
for paragraph and page creation, and during printing.

Argument:   the ?sdop item
Returns:    nothing
*/

void
pin_change_columns(item *i)
{
check_pin_list(i, mcparam_list, mcparam_list_count, FALSE, TRUE);
}



/*************************************************
*       Change dynamic layparm information       *
*************************************************/

/* This function is called during paragraph creation.

Argument:   the ?sdop item
Returns:    nothing
*/

void
pin_dynamic_layparm(item *i)
{
check_pin_list(i, dynamic_layparm_list, dynamic_layparm_list_count, FALSE, TRUE);
}




/*************************************************
*   Change dynamic sub/superscript information   *
*************************************************/

/* This function is called during paragraph creation.

Argument:   the ?sdop item
Returns:    nothing
*/

void
pin_dynamic_subsuper(item *i)
{
check_pin_list(i, subsuplevel_list, subsuplevel_list_count, FALSE, TRUE);
if (subscript_down > 127)
  {
  subscript_down = 127;
  (void)error(106);
  }
if (superscript_up > 127)
  {
  superscript_up = 127;
  (void)error(106);
  }
}




/*************************************************
*      Change options during page creating       *
*************************************************/

/* This function is called for each ?sdop item when a list is being processed
for page creation.

Argument:   the ?sdop item
Returns:    nothing
*/

void
pin_paging_changes(item *i)
{
check_pin_list(i, cpparam_list, cpparam_list_count, FALSE, TRUE);
}



/*************************************************
*       Change figure/table title options        *
*************************************************/

/* This function is called for each ?sdop item when numbering things, including
figures. If the figure, example, or table number format changes, we recalculate
the number of percents it contains.

Argument:   the ?sdop item
Returns:    nothing
*/

void
pin_figtab_format_changes(item *i)
{
uschar *oldfnf = figure_number_format;
uschar *oldtnf = table_number_format;
check_pin_list(i, figtabparam_list, figtabparam_list_count, FALSE, TRUE);
if (figure_number_format != oldfnf)
  {
  uschar *p;
  figure_nformat_pcount = 0;
  for (p = figure_number_format; (p = Ustrchr(p, '%')) != NULL; p++)
    figure_nformat_pcount++;
  }
if (table_number_format != oldtnf)
  {
  uschar *p;
  table_nformat_pcount = 0;
  for (p = table_number_format; (p = Ustrchr(p, '%')) != NULL; p++)
    table_nformat_pcount++;
  }
if (example_number_format != oldtnf)
  {
  uschar *p;
  example_nformat_pcount = 0;
  for (p = example_number_format; (p = Ustrchr(p, '%')) != NULL; p++)
    example_nformat_pcount++;
  }
}


/* This function is called during paragraph creation. */

void
pin_figex_layout_changes(item *i)
{
check_pin_list(i, figexlayoutparam_list, figexlayoutparam_list_count, FALSE, TRUE);
}


/* This function is called during table creation. */

void
pin_table_layout_changes(item *i)
{
check_pin_list(i, tablelayoutparam_list, tablelayoutparam_list_count, FALSE, TRUE);
}





/*************************************************
*           Change head/foot information         *
*************************************************/

/* This function is called for each ?sdop item in a page that is about to
be printed. We check for changes to the header/footer content.

Argument:   the ?sdop item
Returns:    nothing
*/

void
pin_headfoot(item *i)
{
check_pin_list(i, hfparam_list, hfparam_list_count, FALSE, TRUE);
}



/*************************************************
*          Set default list parameters           *
*************************************************/

/* There is more than one scan that adjusts these parameters, so we have
a function to initialize them.

Arguments:   none
Returns:     nothing
*/

void
pin_default_list_params(void)
{
int i;
ilist_tag_vfont = maintext_vfont;
olist_tag_vfont = maintext_vfont;
olist_format = US"(%s)";
for (i = 0; i < 3; i++)
  {
  ilist_tag_colour[i] = 0;
  olist_tag_colour[i] = 0;
  }
}



/*************************************************
*    Change itemized/ordered list parameters     *
*************************************************/

/* This function is called for each ?sdop item in a page during font assignment
and also as it is being printed. It processes changes to itemized and ordered
list parameters. If the size or leading of the ilist or olist tag fonts is
changed, we must apply the overall scaling by passing TRUE as the second-last
argument of check_pin_list().  Also, if there is a change to the ordered list
numbering format, the new string is passed through entity_find().

Arguments:  i        the ?sdop item
            warnobs  TRUE to warn for obsolete parameters

Returns:    nothing
*/

void
pin_change_list_params(item *i, BOOL warnobs)
{
BOOL found;
int length;
uschar *p, *pp, *np, *value;
uschar *old_olist_format = olist_format;

check_pin_list(i, list_param_list, list_param_list_count, TRUE, warnobs);

/* Handle changes to the olist format. */

if (olist_format == old_olist_format) return;

length = 1;
found = FALSE;

for (p = olist_format; *p != 0;)
  {
  length++;
  if (*p++ != '&') continue;
  found = TRUE;
  p = entity_find(p, &value, FALSE, US"");
  length += Ustrlen(value) - 1;
  }

if (!found) return;   /* No '&' characters found */

np = pp = misc_malloc(length);
for (p = olist_format; *p != 0;)
  {
  if (*p != '&')
    {
    *pp++ = *p++;
    continue;
    }
  p = entity_find(p + 1, &value, TRUE, US"");
  Ustrcpy(pp, value);
  pp += Ustrlen(value);
  }

*pp = 0;
olist_format = np;
}


/*************************************************
*   Unsigned integer compare for qsort below     *
*************************************************/

static int
uicomp(const void *a, const void *b)
{
unsigned int ia = *(unsigned int *)(a);
unsigned int ib = *(unsigned int *)(b);
if (ia < ib) return -1;
if (ia > ib) return +1;
return 0;
}



/*************************************************
*     Scan for global processing flags           *
*************************************************/

/* Some global flags set options for the way certain processing happens, once
and for all. This function is called immediately after the items have been read
in.

Argument:   the first processing item
Returns:    TRUE
*/

BOOL
pin_global(item *item_list)
{
item *i;

DEBUG(D_any) debug_printf("Setting global parameters\n");

for (i = item_list; i != NULL; i = i->next)
  {
  if (Ustrcmp(i->name, US"?sdop") != 0) continue;
  read_linenumber = i->linenumber;
  check_pin_list(i, glparam_list, glparam_list_count, FALSE, TRUE);
  (void)misc_yesno_vector(i, US"toc_chapter_blanks", toc_chapter_blanks, 2);
  pin_change_layparm(i);
  }

/* If the cross reference colour was set, compute the packed version. */

if (xref_colour[0] >= 0)
  xref_packed_colour = (xref_colour[0] << 20) |
                       (xref_colour[1] << 10) | xref_colour[2];

/* If the base typesize value was changed, scale all the vfonts accordingly. */

if (scale_typesize_base != scale_typesize_base_orig)
  {
  vfontstr *v, **vv, ***vvv;
  for (vvv = &(vfonttables[0]); *vvv != NULL; vvv++)
    {
    for (vv = *vvv; *vv != NULL; vv++)
      {
      v = *vv;
      v->size = MULDIV(v->size, scale_typesize_base, scale_typesize_base_orig);
      v->leading = MULDIV(v->leading, scale_typesize_base, scale_typesize_base_orig);
      }
    }
  }

/* Enforce a minimum page length */

if (page_full_length < 108000)
  {
  (void)error(108, misc_formatfixed(page_full_length));
  page_full_length = 108000;
  }

/* Check for a valid paper size */

if (paper_size != NULL)
  {
  if (sscanf(CS paper_size, "%lfx%lf", &paper_size_width, &paper_size_height)
      != 2)
    {
    error(92);
    paper_size = NULL;
    }
  }

/* If head/foot linewidth hasn't been set, use the main width. Save a copy
for re-instating after head/foot. */

if (page_foot_linewidth <= 0) page_foot_linewidth = page_linewidth;
if (page_head_linewidth <= 0) page_head_linewidth = page_linewidth;
page_main_linewidth = page_linewidth;

/* If the index_sort_omit string was set, unpick it into a vector of character
values, terminated by 0xffffffff. */

if (index_sort_omit_string != NULL)
  {
  int count = 1;   /* Allow for the terminating 0xffffffff */
  int c;
  uschar *s = index_sort_omit_string;
  unsigned int *t;

  while (*s != 0)
    {
    GETCHARINC(c, s);
    count++;
    if (c == '&')
      {
      while (*s != ';') s++;
      s++;
      }
    }

  s = index_sort_omit_string;
  t = index_sort_omit = misc_malloc(count * sizeof(unsigned int));

  while (*s != 0)
    {
    GETCHARINC(c, s);
    if (c == '&')
      {
      uschar *buff;
      s = entity_find(s, &buff, FALSE, US"in \"index_sort_omit\" setting");
      GETCHAR(c, buff);
      }
    *t++ = c;
    }

  *t = 0xffffffff;
  qsort(index_sort_omit, count, sizeof(unsigned int), uicomp);
  }

read_linenumber = 0;
return TRUE;
}



/*************************************************
*         Cut out conditional items              *
*************************************************/

/* This function scans an item list and processes <?sdop if(n)def="xxx"?> items
by cutting out unwanted sections.

Argument:   the first processing item
Returns:    TRUE
*/

BOOL
pin_cutcond(item *item_list)
{
item *i, *ii;

DEBUG(D_any) debug_printf("Processing conditional sections\n");

for (i = item_list; i != NULL; i = i->next)
  {
  BOOL cond;
  uschar *cname, *evalue;
  paramstr *p;
  int nest;

  if (Ustrcmp(i->name, US"?sdop") != 0) continue;

  p = misc_param_find(i, US"ifdef");
  if (p != NULL) cond = TRUE; else
    {
    p = misc_param_find(i, US"ifndef");
    if (p != NULL) cond = FALSE;
    }
  if (p == NULL) continue;   /* Neither ifdef nor ifndef */

  /* Find the value of named entity that provides the condition */

  cname = p->name;
  entity_find_byname(p->value, &evalue, TRUE, US"");

  /* Find the matching endif; we need to do this always so that it is marked
  as used, and also so that any skipped nested ifdefs are also so marked. */

  nest = 1;
  for (ii = i->next; ii != NULL; ii = ii->next)
    {
    if (Ustrcmp(ii->name, US"?sdop") != 0) continue;
    p = misc_param_find(ii, US"endif");
    if (p != NULL)
      {
      if (--nest <= 0) break;
      continue;
      }
    p = misc_param_find(ii, US"ifdef");
    if (p != NULL) nest++; else
      {
      p = misc_param_find(ii, US"ifndef");
      if (p != NULL) nest++;
      }
    }

  /* If the condition is OK, we can just continue. */

  if ((*evalue == 0) != cond) continue;

  /* Cut out everything between here and <?sdop endif?>, allowing for nested
  conditionals. i -> first to remove; ii -> last to remove or NULL. */

  if (ii == NULL)
    {
    read_linenumber = i->linenumber;
    error(68, cname);
    }
  else
    {
    i = i->prev;
    i->next = ii->next;
    if (ii->next != NULL) ii->next->prev = i;
    }
  }

return TRUE;
}


/*************************************************
*             Process insertion items            *
*************************************************/

/* This function scans a list, looking for processing instructions of the form
<?sdop insert="xxxx" ?>. Such items cause a copy of an appropriate saved
sublist to be inserted. This is used for <legalnotice> and <revdescription>.

Argument:   the list to scan
Returns:    TRUE
*/

BOOL
pin_process_inserts(item *i)
{
for (; i != NULL; i = i->next)
  {
  paramstr *p;
  item *insert_list = NULL;

  if (Ustrcmp(i->name, "?sdop") != 0 ||
      (p = misc_param_find(i, US"insert")) == NULL)
    continue;

  if (Ustrcmp(p->value, "legalnotice") == 0)
    insert_list = legalnotice_item_list;
  else if (Ustrcmp(p->value, "revdescription") == 0)
    insert_list = revdescription_item_list;
  else error(98, p->value);

  if (insert_list != NULL)
    {
    item *addto = i;
    item *follow = i->next;

    item *partner_stack[50];
    int pstackptr = 0;

    for (; insert_list != NULL; insert_list = insert_list->next)
      {
      item *newi = misc_malloc(sizeof(item));
      *newi = *insert_list;
      newi->prev = addto;
      addto->next = newi;
      addto = newi;

      if (insert_list->partner == insert_list)
        newi->partner = newi;
      else if (Ustrcmp(insert_list->name, "/") == 0)
        {
        newi->partner = partner_stack[--pstackptr];
        newi->partner->partner = newi;
        }
      else
        partner_stack[pstackptr++] = newi;
      }

    addto->next = follow;
    if (follow != NULL) follow->prev = addto;
    }
  }

return TRUE;
}


/* End of pin.c */
