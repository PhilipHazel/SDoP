/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: April 2023 */

/* Header file for all the global variables */

#ifndef INCLUDED_GLOBALS_H
#define INCLUDED_GLOBALS_H

/*************************************************
*           General global variables             *
*************************************************/

extern afontstr     *afont_last;
extern afontstr     *afont_list;
extern int           an2ucount;
extern an2uencod     an2ulist[];

extern uschar       *author_address;
extern uschar       *author_corpauthor;
extern uschar       *author_firstname;
extern uschar       *author_honorific;
extern uschar       *author_initials;
extern uschar       *author_jobtitle;
extern uschar       *author_lineage;
extern uschar       *author_orgname;
extern uschar       *author_othername;
extern uschar       *author_surname;

extern int           background_colour[3];
extern vfontstr     *boldfonts[];
extern uschar       *book_cpyholder;
extern uschar       *book_cpyyear;
extern uschar       *book_date;
extern uschar       *book_edition;
extern uschar       *book_issuenum;
extern uschar       *book_pubdate;
extern uschar       *book_publishername;
extern uschar       *book_releaseinfo;
extern uschar       *book_revauthorinitials;
extern uschar       *book_revdate;
extern uschar       *book_revnumber;
extern uschar       *book_subtitle;
extern uschar       *book_title;
extern uschar       *book_titleabbrev;
extern uschar       *book_volumenum;

extern int           bullets_default[];

extern unsigned int  debug_selector;
extern BOOL          debug_need_nl;
extern int           document_type;

extern uschar       *editor_firstname;
extern uschar       *editor_honorific;
extern uschar       *editor_initials;
extern uschar       *editor_jobtitle;
extern uschar       *editor_lineage;
extern uschar       *editor_orgname;
extern uschar       *editor_othername;
extern uschar       *editor_surname;
extern entity_block  entity_list[];
extern int           entity_list_count;
extern tree_node    *entity_tree;
extern int           error_max;
extern int           example_nformat_pcount;
extern int           extra_leading;

extern uschar       *family_names[];
extern int           figure_nformat_pcount;

extern fontelstr     fontels[];
extern fontsuffixstr fontsuffixes[];

extern tree_node    *id_tree;
extern int           index_count;
extern uschar       *index_names[INDEXMAX];
extern unsigned int *index_sort_omit;
extern uschar       *index_sort_omit_string;
extern BOOL          inheadorfoot;
extern BOOL          internal_processing;
extern vfontstr     *italfonts[];

extern item         *legalnotice_item_list;
extern layoutparam  *lptable[];

extern item         *main_foot_item_list;
extern item         *main_head_item_list;
extern FILE         *main_hyphenfile;
extern item         *main_item_list;
extern memchainstr  *main_memory_chain;
extern int           main_page_count;
extern int           memory_hwm;
extern int           memory_used;
extern vfontstr     *monofonts[];

extern BOOL          number_sections_default[MAXSECTDEPTH];

extern uschar       *othercredit_firstname;
extern uschar       *othercredit_honorific;
extern uschar       *othercredit_initials;
extern uschar       *othercredit_jobtitle;
extern uschar       *othercredit_lineage;
extern uschar       *othercredit_orgname;
extern uschar       *othercredit_othername;
extern uschar       *othercredit_surname;

extern int           page_columns;
extern int           page_columns_init;
extern int           page_columns_save;
extern int           page_colsep;
extern int           page_colsep_init;
extern int           page_colsep_save;
extern int           page_length;
extern BOOL          pages_even;
extern pagelist     *pages_front;
extern pagelist     *pages_main;
extern BOOL          pages_odd;
extern BOOL          preface_even_pages;
extern item         *preface_foot_item_list;
extern item         *preface_head_item_list;
extern item         *preface_item_list;
extern int           preface_page_count;

extern item         *read_addto;
extern BOOL          read_done;
extern uschar       *read_filename;
extern int           read_linenumber;
extern uschar       *read_what;
extern item         *revdescription_item_list;
extern vfontstr     *romanfonts[];

extern int           scale_typesize_base;
extern int           scale_typesize_base_orig;
extern uschar       *sdop_share;
extern uschar       *sfontname[];
extern BOOL          subscript_small;
extern int           subscript_down;
extern BOOL          superscript_small;
extern int           superscript_up;
extern elliststr     supported_elements[];
extern int           supported_elements_count;

extern int           table_nformat_pcount;
extern uschar       *text_elements[];
extern item         *title_item_list;
extern int           title_page_count;

extern item         *toc_foot_item_list;
extern item         *toc_head_item_list;
extern item         *toc_item_list;
extern int           toc_page_count;
extern pdfmarkstr   *toc_pdfmarks;
extern uschar       *toc_title;
extern uschar       *type_names[];

extern int           u2scount;
extern u2sencod      u2slist[];
extern tree_node    *unknown_char_tree;
extern tree_node    *unknown_element_tree;
extern const int     utf8_table1[];
extern const int     utf8_table2[];
extern const int     utf8_table3[];
extern const uschar  utf8_table4[];

extern ventity_block ventity_list[];
extern int           ventity_list_count;

extern vfontstr     *vfont_last;
extern vfontstr     *vfont_list;

extern vfontstr     *main_vfonttable[];
extern vfontstr     *footnote_vfonttable[];
extern vfontstr     *headfoot_vfonttable[];
extern vfontstr     *index_vfonttable[];
extern vfontstr     *small_main_vfonttable[];
extern vfontstr     *title_vfonttable[];
extern vfontstr     *toc_vfonttable[];
extern vfontstr     **vfonttables[];

extern const ucd_record  ucd_records[];
extern const uint16_t    ucd_stage1[];
extern const uint16_t    ucd_stage2[];
extern const int         ucp_gentype[];

extern BOOL          warn_unsupported;
extern BOOL          warn_unsupported_set;

extern BOOL          warn_unsupported_chars;
extern BOOL          warn_unsupported_chars_set;


/*************************************************
*       Parameters that control the output       *
*************************************************/

/* The idea is that these are the values that can be changed by configuration
items. */

extern uschar       *type_families[];

extern vfontstr      booktitle1_vfont;
extern vfontstr      booktitle2_vfont;
extern vfontstr      booktitle3_vfont;
extern vfontstr      booktitle4_vfont;

extern vfontstr      chapter_vfont;
extern vfontstr      chapsubt_vfont;
extern vfontstr      section_vfont;
extern vfontstr      subsection_vfont;

extern vfontstr      blockquote_title_vfont;
extern vfontstr      example_title_vfont;
extern vfontstr      figure_title_vfont;
extern vfontstr      formalpara_title_vfont;
extern vfontstr      note_title_vfont;
extern vfontstr      sidebar_title_vfont;

extern vfontstr      ilist_tag_vfont;
extern int           ilist_tag_colour[3];
extern vfontstr      olist_tag_vfont;
extern int           olist_tag_colour[3];

extern vfontstr      boldtext_vfont;
extern vfontstr      boittext_vfont;
extern vfontstr      italtext_vfont;
extern vfontstr      maintext_vfont;

extern vfontstr      monoboit_vfont;
extern vfontstr      monobold_vfont;
extern vfontstr      monoital_vfont;
extern vfontstr      monotext_vfont;

extern vfontstr      small_boldtext_vfont;
extern vfontstr      small_boittext_vfont;
extern vfontstr      small_italtext_vfont;
extern vfontstr      small_maintext_vfont;

extern vfontstr      small_monoboit_vfont;
extern vfontstr      small_monobold_vfont;
extern vfontstr      small_monoital_vfont;
extern vfontstr      small_monotext_vfont;

extern vfontstr      footnote_boldtext_vfont;
extern vfontstr      footnote_boittext_vfont;
extern vfontstr      footnote_italtext_vfont;
extern vfontstr      footnote_maintext_vfont;

extern vfontstr      footnote_monoboit_vfont;
extern vfontstr      footnote_monobold_vfont;
extern vfontstr      footnote_monoital_vfont;
extern vfontstr      footnote_monotext_vfont;

extern vfontstr      footnote_key_vfont;

extern vfontstr      headfoot_boldtext_vfont;
extern vfontstr      headfoot_boittext_vfont;
extern vfontstr      headfoot_italtext_vfont;
extern vfontstr      headfoot_maintext_vfont;

extern vfontstr      headfoot_monoboit_vfont;
extern vfontstr      headfoot_monobold_vfont;
extern vfontstr      headfoot_monoital_vfont;
extern vfontstr      headfoot_monotext_vfont;

extern vfontstr      table_title_vfont;
extern vfontstr      title_section_vfont;

extern vfontstr      title_boldtext_vfont;
extern vfontstr      title_boittext_vfont;
extern vfontstr      title_italtext_vfont;
extern vfontstr      title_maintext_vfont;

extern vfontstr      title_monoboit_vfont;
extern vfontstr      title_monobold_vfont;
extern vfontstr      title_monoital_vfont;
extern vfontstr      title_monotext_vfont;

extern vfontstr      toc_fill_vfont;
extern vfontstr     *toc_fill_vfont_ptr;

extern vfontstr      toc_boldtext_vfont;
extern vfontstr      toc_boittext_vfont;
extern vfontstr      toc_italtext_vfont;
extern vfontstr      toc_maintext_vfont;

extern vfontstr      toc_monobold_vfont;
extern vfontstr      toc_monoboit_vfont;
extern vfontstr      toc_monoital_vfont;
extern vfontstr      toc_monotext_vfont;

extern vfontstr      index_section_vfont;

extern vfontstr      index_boldtext_vfont;
extern vfontstr      index_boittext_vfont;
extern vfontstr      index_italtext_vfont;
extern vfontstr      index_maintext_vfont;

extern vfontstr      index_monoboit_vfont;
extern vfontstr      index_monobold_vfont;
extern vfontstr      index_monoital_vfont;
extern vfontstr      index_monotext_vfont;

extern int           index_page_columns;
extern int           index_page_colsep;

extern int           list_numeration_default;
extern uschar       *olist_format;

extern uschar       *example_number_format;
extern uschar       *example_title_format;
extern int           example_title_justify;
extern int           example_title_width;

extern uschar       *figure_number_format;
extern uschar       *figure_title_format;
extern int           figure_title_justify;
extern int           figure_title_width;

extern uschar       *table_number_format;
extern uschar       *table_title_format;
extern int           table_title_justify;
extern int           table_title_width;

extern BOOL          blockquote_ruled;
extern int           blockquote_title_justify;

extern BOOL          note_ruled;
extern int           note_title_justify;

extern BOOL          sidebar_ruled;
extern int           sidebar_title_justify;

extern int           command_fs;
extern int           filename_fs;
extern int           function_fs;
extern int           option_fs;
extern int           replaceable_fs;
extern int           userinput_fs;
extern int           varname_fs;

extern BOOL          chapter_skip_head;
extern layoutparam   chapter_layparm;
extern layoutparam   chapter2_layparm;
extern layoutparam   chapsubt_layparm;
extern layoutparam   section_layparm;
extern layoutparam   subsection_layparm;

extern layoutparam   example_title_layparm;
extern layoutparam   figure_title_layparm;
extern layoutparam   formalpara_layparm;
extern layoutparam   ilistpara_layparm;
extern layoutparam   olistpara_layparm;
extern layoutparam   vlistpara_layparm;
extern layoutparam   vlistpara1_layparm;
extern layoutparam   vlisttitle_layparm;
extern layoutparam   table_layparm;
extern layoutparam   term_layparm;
extern layoutparam   termfirst_layparm;
extern layoutparam   termmid_layparm;
extern layoutparam   termlast_layparm;
extern layoutparam   literalpara_layparm;
extern layoutparam   para_layparm;
extern layoutparam   ixpara_layparm;
extern layoutparam   table_layparm;
extern layoutparam   table_title_layparm;
extern layoutparam   framed_table_layparm;
extern layoutparam   blockquote_layparm;
extern layoutparam   note_layparm;
extern layoutparam   sidebar_layparm;

extern layoutparam   footnote_formalpara_layparm;
extern layoutparam   footnote_ilistpara_layparm;
extern layoutparam   footnote_olistpara_layparm;
extern layoutparam   footnote_vlistpara_layparm;
extern layoutparam   footnote_vlistpara1_layparm;
extern layoutparam   footnote_term_layparm;
extern layoutparam   footnote_termfirst_layparm;
extern layoutparam   footnote_termmid_layparm;
extern layoutparam   footnote_termlast_layparm;
extern layoutparam   footnote_literalpara_layparm;
extern layoutparam   footnote_para_layparm;
extern layoutparam   footnote_ixpara_layparm;
extern layoutparam   footnote_table_layparm;
extern layoutparam   footnote_framed_table_layparm;
extern layoutparam   footnote_blockquote_layparm;
extern layoutparam   footnote_note_layparm;
extern layoutparam   footnote_sidebar_layparm;

extern int           footnote_indent;
extern int           footnote_line_thickness;
extern int           footnote_line_length;
extern int           footnote_overhead;

extern int           global_align_default;
extern int           global_align_char_default;
extern int           global_align_charoff_default;
extern BOOL          global_colsep_default;
extern BOOL          global_rowsep_default;
extern int           global_tableflags_default;

extern BOOL          index_headings_enabled;
extern BOOL          literal_indent_fudge;

extern BOOL          main_even_pages;
extern hfstr         main_headfoot;
extern hfstr         main_headfoot_default;
extern int           margin_bottom;
extern int           margin_left;
extern int           margin_left_recto;
extern int           margin_left_verso;

extern BOOL          number_sections[MAXSECTDEPTH];

extern int           page_baroffset;
extern int           page_barwidth;
extern int           page_foot_length;
extern int           page_foot_linewidth;
extern int           page_full_length;
extern int           page_head_length;
extern int           page_head_linewidth;
extern int           page_linewidth;
extern int           page_main_linewidth;
extern uschar       *paper_size;
extern double        paper_size_height;
extern double        paper_size_width;
extern hfstr         preface_headfoot;

extern BOOL          svg_listignored;

extern int           table_bot_frame_space;
extern int           table_frame_thickness;
extern int           table_left_col_space;
extern int           table_overall_indent;
extern int           table_right_col_space;
extern int           table_row_sep_space;
extern int           table_top_frame_space;

extern BOOL          title_even_pages;

extern BOOL          toc_chapter_blanks[];
extern BOOL          toc_even_pages;
extern uschar       *toc_fill_string;
extern int           toc_fill_leftspace;
extern int           toc_fill_rightspace;
extern hfstr         toc_headfoot;
extern BOOL          toc_sections[MAXSECTDEPTH];
extern BOOL          toc_printed_sections[MAXSECTDEPTH];
extern BOOL          toc_sections_default[MAXSECTDEPTH];
extern BOOL          toc_sections_preface_default[MAXSECTDEPTH];

extern uschar       *toc_line_chapter_strings[];
extern uschar       *toc_line_sect1_strings[];
extern uschar       *toc_line_sect2_strings[];

extern uschar       **toc_line_strings[];
extern int           toc_line_string_count;

extern int           xref_colour[3];
extern BOOL          xref_links;
extern int           xref_packed_colour;
#endif

/* End of globals.h */
