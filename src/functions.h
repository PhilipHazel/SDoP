/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This header defines all the global functions. */

#ifndef INCLUDED_FUNCTIONS_H
#define INCLUDED_FUNCTIONS_H

#if SUPPORT_JPEG
extern int           read_JPEG_file(FILE *, BOOL, void *);
extern void          give_jpeg_image_data(int, int, int);
extern void          put_scanline_someplace(uschar *, int, void *);
#endif

#if SUPPORT_PNG
extern int           read_PNG_file(FILE *, uschar **, BOOL);
extern void          give_png_image_data(int, int, int);
extern void          write_png_data(FILE *);
#endif

extern void          Hyphen_Init(void);
extern int           Hyphen_Prepare(uschar *);
extern int           Hyphen_DePlural(uschar *, uschar *);
extern int           Hyphen_Next(uschar *, int);

extern BOOL          book_getdata(item *);
extern BOOL          book_title_make(void);

extern void          debug_printf(const char *, ...) PRINTF_FUNCTION ;
extern void          debug_printfixed(int, const char *);
extern void          debug_print_item_list(item *, const char *);
extern void          debug_print_line_text(outputline *);
extern void          debug_print_para(item *, item *, const char *);
extern void          debug_print_string(uschar *, int, const char *);

extern BOOL          entity_expand(item *);
extern uschar       *entity_find(uschar *, uschar **, BOOL, uschar *);
extern void          entity_find_byname(uschar *, uschar **, BOOL, uschar *);
extern BOOL          error(int, ...);

extern BOOL          font_assign(item *, int);
extern int           font_charwidth(int, vfontstr *, int *);
extern int           font_get_bullet(item *);
extern int           font_kernwidth(int, int, vfontstr *);
extern BOOL          font_loadalltables(void);
extern int           font_stringwidth(uschar *, vfontstr *);
extern vfontstr     *font_used(vfontstr *, uschar *);

extern void          footnote_insert_reference(item *, unsigned int);
extern BOOL          footnote_insert_keys(item *);
extern void          footnote_remove_newline(item *);

extern BOOL          index_make(item *, item *);

extern int           misc_alpha(uschar *, int);
extern item         *misc_dummy_item(void);
extern lengthstring *misc_find_rawtitle(item *, uschar *);
extern BOOL          misc_find_share(uschar *, uschar *, BOOL);
extern lengthstring *misc_find_title(item *, uschar *);
extern char         *misc_formatfixed(int);    /* char * is deliberate */
extern void          misc_free(void *, int);
extern BOOL          misc_get_colour(uschar *, int *);
extern int           misc_get_fp(uschar *, uschar **);
extern int           misc_get_dimension(uschar *);
extern BOOL          misc_get_dimensions(int, uschar *, int *, BOOL);
extern int           misc_get_number(uschar *);
extern item         *misc_insert_element_pair(uschar *, item *);
extern void          misc_insert_item(item *, item *);
extern BOOL          misc_istext_elname(uschar *);
extern void         *misc_malloc(int);
extern int           misc_ord2utf8(int, uschar *);
extern paramstr     *misc_param_find(item *, uschar *);
extern int           misc_roman(uschar *, int);
extern int           misc_scale_number(int, uschar *);
extern BOOL          misc_yesno_vector(item *, uschar *, BOOL *, int);

extern BOOL          number_titles(item *);

extern int           object_find_size(item *, int *);
extern int           object_write_image(item *, int, FILE *, int *, int *);

extern BOOL          page_format(item *, item **, BOOL, BOOL, int *, uschar *);
extern BOOL          para_format(item *);
extern BOOL          para_identify(item *, int, item *);
extern void          pin_change_columns(item *);
extern uint32_t      pin_change_flags(item *, uint32_t);
extern void          pin_change_font_assign(item *);
extern void          pin_change_layparm(item *);
extern void          pin_change_list_params(item *, BOOL);
extern BOOL          pin_cutcond(item *);
extern void          pin_default_list_params(void);
extern void          pin_dynamic_layparm(item *);
extern void          pin_dynamic_subsuper(item *);
extern void          pin_figtab_format_changes(item *);
extern void          pin_figex_layout_changes(item *);
extern BOOL          pin_global(item *);
extern void          pin_headfoot(item *);
extern unsigned int  pin_init_flags(void);
extern void          pin_paging_changes(item *);
extern BOOL          pin_process_inserts(item *);
extern void          pin_table_layout_changes(item *);
extern BOOL          preface_process(void);

extern uschar       *read_element(uschar *, item **, int *);
extern BOOL          read_file(uschar *, item *);
extern int           read_file2(uschar *, item **, int *);
extern BOOL          read_includes(item *, uschar *);
extern BOOL          read_main_file(uschar *);
extern void          read_string(uschar *, item **, int *, int);
extern BOOL          read_write(uschar *);

extern BOOL          ref_resolve(item *);
extern BOOL          ref_set_pages(void);

extern BOOL          sys_exists(uschar *);

extern BOOL          table_identify(item *, item *);
extern int           table_row_depth(tdatastr *td, item *);
extern BOOL          toc_make(item *, item *);
extern BOOL          toc_save_raw_titles(item *);
extern int           tree_insertnode(tree_node **, tree_node *);
extern tree_node    *tree_search(tree_node *, uschar *);

extern BOOL          url_check(item *);

extern BOOL          write_file(uschar *);
#endif

/* End of functions.h */
