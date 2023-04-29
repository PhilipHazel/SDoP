/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: April 2023 */

/* Main SDoP header file, included by all modules. */

#ifndef INCLUDED_SDOP_H
#define INCLUDED_SDOP_H

#define SDOP_VERSION "1.10"
#define SDOP_DATE    "09-April-2023"

/* Standard C headers */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* These are some parameters that specify sizes of things in the code. They
must appear before including the local headers. */

#define DBNAMESIZE           32
#define DBPARAMNAMESIZE      32

#define LINEBUFSIZE        1024
#define NESTSTACKSIZE       100

#define INDEXMAX             10

/* These values do not necessarily have to appear before including the local
headers, but they might as well be together with those above. */

#define MAXPARALINES       1024
#define MAXTABLEROWS       1024

#define MAXHEADFOOTPARA      32
#define MAXSECTDEPTH         10
#define MAXLISTNEST          10

#define TOC_LINE_STRING_COUNT 6

#define DEFAULT_PAGE_COLSEP  16000

/* More header files for sdop */

#include "mytypes.h"
#include "structs.h"
#include "globals.h"
#include "functions.h"
#include "ucp.h"

/* Error codes */

#define ec_noerror   0
#define ec_warning   1
#define ec_serious   2
#define ec_failed    3
#define ec_disaster  4

/* Document types */

#define DOC_UNSET         0
#define DOC_BOOK          1
#define DOC_ARTICLE       2

/* The following value must not be less than 256. Characters whose Unicode
values are less than this limit are treated specially in two ways:

(1) Each font has a table of widths of this size, so that the widths of the
most common characters can be found quickly. Characters whose code points are
greater than this limit have their widths stored in a tree structure instead.

(2) Cloned PostScript fonts are created, with different encodings, so that
characters up to this value can be directly printed without too much
translation. NOTE: At present, we assume that the value is less than 512,
because only two fonts are used. It seems unlikely that we will ever need to
extend this, but you never know. */

#define LOWCHARLIMIT   384

/* This defines the character that is substituted for unavailable characters.
It should be one that is always available in the base font, with code less that
256, because it is used unmodified in the PostScript output. */

#define UNKNOWN_CHAR   0x00a4      /* Currency symbol */

/* This defines the "width" that is placed in the low-character width table for
unknown characters. It marks the character as unknown. We cannot use a negative
number, because there may be fonts containing characters that have negative
widths. */

#define WIDTH_UNKNOWN  0x7fffffff

/* Other specific Unicode characters that are needed */

#define BREAK_PERMIT   0x0082      /* break permitted here */
#define NO_BREAK_HERE  0x0083      /* break not permitted here */
#define HARD_SPACE     0x00a0      /* not-split space */
#define SOFT_HYPHEN    0x00ad      /* hyphen or omitted */
#define ZERO_SPACE     0x200b      /* zero-width space */
#define OPEN_SQUOTE    0x2018      /* opening single quote */
#define CLOSE_SQUOTE   0x2019      /* closing single quote */
#define OPEN_DQUOTE    0x201c      /* opening double quote */
#define CLOSE_DQUOTE   0x201d      /* closing double quote */
#define CHAR_FI        0xfb01      /* fi ligature */
#define CHAR_ENDASH    0x2013      /* en dash */
#define CHAR_EMDASH    0x2014      /* em dash */

#define BULLET_BULLET  0x00b7
#define BULLET_DASH    0x2212
#define BULLET_OCIRCLE 'o'
#define BULLET_NONE    INT_MAX

/* We also need some Unicode characters as strings for use when building
indexes, after the entities in the text have already been processed. */

#define S_HARD_SPACE   "\xc2\xa0"
#define S_HARD_SPACE2  "\xc2\xa0\xc2\xa0"
#define S_HARD_SPACE4  "\xc2\xa0\xc2\xa0\xc2\xa0\xc2\xa0"
#define S_EN_DASH      "\xe2\x80\x93"

/* A string that is used when a chapter or section has no title. */

#define UNTITLED       US"Untitled"

/* A value that is used when dealing with preface page numbers. */

#define PREFACE_DUMMY_PAGE (-1000000)

/* Debugging control. D_all does not include D_internal, so as to avoid
outputting details of head/foot/toc template files every time. */

#define DEBUG(x)       if ((debug_selector & (x)) != 0)

#define D_any          0x00000001
#define D_entity       0x00000002
#define D_fill         0x00000004
#define D_font         0x00000008
#define D_fontload     0x00000010
#define D_hyphen       0x00000020
#define D_index        0x00000040
#define D_indexfull    0x00000080
#define D_internal     0x00000100
#define D_ipara        0x00000200
#define D_itable       0x00000400
#define D_number       0x00000800
#define D_object       0x00001000
#define D_page         0x00002000
#define D_para         0x00004000
#define D_param        0x00008000
#define D_read         0x00010000
#define D_ref          0x00020000
#define D_svg          0x00040000
#define D_title        0x00080000
#define D_toc          0x00100000
#define D_write        0x00200000

#define D_all         (0xffffffff & ~D_internal)

/* Flags in items */

#define IF_FONTSET     0x00000001   /* Item changes font */
#define IF_PARACONTA   0x00000002   /* First part of split para */
#define IF_PARACONTB   0x00000004   /* Second part of split para */
#define IF_NUMBERED    0x00000008   /* Number inserted at start (of title) */
#define IF_NOHEADFOOT  0x00000010   /* Suppress head foot (in page item) */
#define IF_ISENTRY     0x00000020   /* Paragraph is a table entry */
#define IF_ROWSEP      0x00000040   /* This row has a separator */
#define IF_FIGTITLE    0x00000080   /* This para is a figure title */
#define IF_TABTITLE    0x00000100   /* This para is a table title */
#define IF_RULEABOVE   0x00000200   /* This para has a rule above */
#define IF_RULEBELOW   0x00000400   /* This para has a rule below */
#define IF_DUMMYDATA   0x00000800   /* The is dummy #PCDATA item */

/* Flags in table structures */

#define TDF_BOTFRAME   0x00000001
#define TDF_TOPFRAME   0x00000002
#define TDF_SIDEFRAME  0x00000004

#define TDF_HASBODY    0x00000008
#define TDF_HASFOOT    0x00000010
#define TDF_HASHEAD    0x00000020

#define TDF_CONTA      0x00000040
#define TDF_CONTB      0x00000080

#define TDF_TOC        0x00000100   /* The special TOC table */

#define TDF_DEFAULT    (TDF_BOTFRAME|TDF_TOPFRAME|TDF_SIDEFRAME)

/* Processing instruction flags in textblocks. The top seven bits are used for
the percentage to go up/down for subscripts or superscripts. */

#define PIN_KERN         0x00000001u
#define PIN_HYPH         0x00000002u
#define PIN_SOFTOF       0x00000004u   /* Table: warn for soft overflow */
#define PIN_HARDOF       0x00000008u   /* Table: warn for hard overflow */
#define PIN_REVCH        0x00000010u   /* revisionflag="changed" */
#define PIN_FNKEYREF     0x00000020u   /* Foonote key reference */
#define PIN_FNKEYDEF     0x00000040u   /* Footnote key definition */
#define PIN_FNREFREF     0x00000080u   /* Reference from <footnoteref> */
#define PIN_SUPERSCRIPT  0x00000100u
#define PIN_SUBSCRIPT    0x00000200u
#define PIN_XREF         0x00000400u   /* This is crossreference text */
#define PIN_IREF         0x00000800u   /* This is index reference text */
#define PIN_SSPERCENT    0xFE000000u   /* Seven-bit number */

#define SSPERCENT_SHIFT    25
#define SSPERCENT_DEFAULT  33
#define SSPERCENT_MASK    127

/* These are not dynamically alterable */

#define PIN_FIXED  (PIN_FNKEYREF|PIN_FNKEYDEF|PIN_FNREFREF|\
                    PIN_SUPERSCRIPT|PIN_SUBSCRIPT|PIN_XREF|\
                    PIN_IREF)

/* Flags in output line blocks */

#define OLF_HYPHENATED 0x00000001
#define OLF_ADD_HYPHEN 0x00000002
#define OLF_RULE       0x00000004

/* List numerations */

enum { LN_ARABIC, LN_alpha, LN_roman, LN_ALPHA, LN_ROMAN };

/* These definitions must be kept in step with the vectors in datatables.c. */

enum { FFAM_SERIF, FFAM_SANSERIF, FFAM_MONO, FFAM_SPECIAL, FFAM_EXOTIC };
enum { FTYPE_ROMAN, FTYPE_ITALIC, FTYPE_BOLD, FTYPE_BOLDITALIC, FTYPE_SPECIAL };

enum { J_UNSET, J_LEFT, J_CENTRE, J_RIGHT, J_BOTH, J_CHAR };

#define FONTS_MAIN      0
#define FONTS_SMALL     8   /* These values are offsets into the vfontstr */
#define FONTS_TOC      16   /* tables and are used as the 2nd argument to */
#define FONTS_INDEX    24   /* font_assign() and para_identify(). */
#define FONTS_TITLE    32
#define FONTS_FOOTNOTE 40
#define FONTS_HEADFOOT 48

/* Values in the special fonts characters table */

#define SF_SYMB  0
#define SF_DBAT  1

/* Values for current font state */

#define FS_ROMAN   0
#define FS_ITALIC  1
#define FS_BOLD    2
#define FS_MONO    4

/* Values for type of character */

#define CHTYPE_UNKNOWN  0     /* Unknown character */
#define CHTYPE_STD      1     /* Character is in standard font */
#define CHTYPE_AUX      2     /* Character is in auxiliary font */

/* Values for image formats */

#define IFORM_EPS   1
#define IFORM_JPG   2
#define IFORM_PNG   3
#define IFORM_SVG   4

/* Values for main and footnote layout parameter selection. The last one must
be LP_FOOTNOTE. These are used to index into lptable. */

#define LP_MAIN        0

enum { LP_FORMALPARA,   LP_PARA,         LP_BLOCKQUOTE,   LP_SIDEBAR,
       LP_NOTE,         LP_ILISTPARA,    LP_OLISTPARA,    LP_VLISTPARA,
       LP_VLISTPARA1,   LP_TERM,         LP_TERMFIRST,    LP_TERMMID,
       LP_TERMLAST,     LP_LITERALPARA,  LP_TABLE,        LP_FRAMED_TABLE,
       LP_TABLETITLE,   LP_FIGURETITLE,  LP_EXAMPLETITLE, LP_FOOTNOTE };

/* Macro for multiplying two fixed point numbers and rescaling. */

#define MUL(a,b) \
  ((int)((((double)((int)(a)))*((double)((int)(b))))/1000.0))

/* Macro for multiplying two fixed point numbers and dividing by a third */

#define MULDIV(a,b,c) \
  ((int)((((double)((int)(a)))*((double)((int)(b))))/((double)((int)(c)))))

/* Macro for testing for "section" or "sectN" */

#define ISSECT(n) \
  ( \
  Ustrncmp(n, "sect", 4) == 0 && \
    ( \
    Ustrcmp(n+4, "ion") == 0 || \
      (n[5] == 0 && isdigit(n[4])) \
    ) \
  )

/* ------------ UTF-8 and UCD macros ------------ */

/* Get the next UTF-8 character, not advancing the pointer. */

#define GETCHAR(c, ptr) \
  c = *ptr; \
  if ((c & 0xc0) == 0xc0) \
    { \
    int gcii; \
    int gcaa = utf8_table4[c & 0x3f];  /* Number of additional bytes */ \
    int gcss = 6*gcaa; \
    c = (c & utf8_table3[gcaa]) << gcss; \
    for (gcii = 1; gcii <= gcaa; gcii++) \
      { \
      gcss -= 6; \
      c |= (ptr[gcii] & 0x3f) << gcss; \
      } \
    }

/* Get the next UTF-8 character, advancing the pointer. */

#define GETCHARINC(c, ptr) \
  c = *ptr++; \
  if ((c & 0xc0) == 0xc0) \
    { \
    int gcaa = utf8_table4[c & 0x3f];  /* Number of additional bytes */ \
    int gcss = 6*gcaa; \
    c = (c & utf8_table3[gcaa]) << gcss; \
    while (gcaa-- > 0) \
      { \
      gcss -= 6; \
      c |= (*ptr++ & 0x3f) << gcss; \
      } \
    }

/* Get the next UTF-8 character, not advancing the pointer, incrementing length
if there are extra bytes. */

#define GETCHARLEN(c, ptr, len) \
  c = *ptr; \
  if ((c & 0xc0) == 0xc0) \
    { \
    int gcii; \
    int gcaa = utf8_table4[c & 0x3f];  /* Number of additional bytes */ \
    int gcss = 6*gcaa; \
    c = (c & utf8_table3[gcaa]) << gcss; \
    for (gcii = 1; gcii <= gcaa; gcii++) \
      { \
      gcss -= 6; \
      c |= (ptr[gcii] & 0x3f) << gcss; \
      } \
    len += gcaa; \
    }

/* If the pointer is not at the start of a character, move it back until
it is. */

#define BACKCHAR(ptr) while((*ptr & 0xc0) == 0x80) ptr--;

/* UCD access macros - copied from PCRE2. We are only interested in the
character category, for detecting letters when hyphenating. */

#define UCD_BLOCK_SIZE 128
#define GET_UCD(ch) (ucd_records + \
        ucd_stage2[ucd_stage1[(int)(ch) / UCD_BLOCK_SIZE] * \
        UCD_BLOCK_SIZE + (int)ch % UCD_BLOCK_SIZE])

#define UCD_CHARTYPE(ch)  GET_UCD(ch)->chartype
#define UCD_CATEGORY(ch)  ucp_gentype[UCD_CHARTYPE(ch)]

#endif   /* INCLUDED_SDOP_H */

/* End of sdop.h */
