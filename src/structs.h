/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

#ifndef INCLUDED_STRUCTS_H
#define INCLUDED_STRUCTS_H

/* This module contains definitions of structures that are used throughout the
program. */

struct item;                   /* For forward reference */

/* Structure used at the start of malloc'ed blocks so that they can all be
chained for freeing on exit. */

typedef struct memchainstr {
  struct memchainstr *next;
  struct memchainstr *prev;
} memchainstr;

/* Structure for table of supported elements */

typedef struct elliststr {
  uschar *name;
  uschar **attrs;
} elliststr;

/* Structure for each node in a tree, of which there are various kinds */

typedef struct tree_node {
  struct tree_node *left;      /* pointer to left child */
  struct tree_node *right;     /* pointer to right child */
  union
    {
    void  *ptr;                /* pointer to data */
    int val[2];                /* or integer data (2 values) */
    } data;
  uschar  balance;             /* balancing factor */
  uschar  name[1];             /* node name - variable length */
} tree_node;

/* Items in a kerning table */

typedef struct kerntablestr {
  uint32_t pair;
  int32_t kwidth;
} kerntablestr;

/* Entries in the character name to Unicode code point table */

typedef struct an2uencod {
  uschar *name;                /* Adobe character name */
  int code;                    /* Unicode code point */
  int poffset;                 /* Offset for printing certain chars */
} an2uencod;

/* Entries in the Unicode to special font char table */

typedef struct u2sencod {
  int ucode;                   /* Unicode code point */
  int which;                   /* Which of the special fonts */
  int scode;                   /* Code point in the special font */
} u2sencod;

/* For each actual font */

typedef struct afontstr {
  struct afontstr *next;
  int *widths;                 /* width table for low-valued characters */
  tree_node *widths_tree;      /* tree for other characters */
  kerntablestr *kerns;         /* kern table */
  int kerncount;               /* size of same */
  int psnumber;                /* PostScript base font number */
  BOOL stdencoding;            /* Set from the AFM file */
  BOOL fixedpitch;             /* Set from the AFM file */
  BOOL hasfi;                  /* Set from the AFM file */
  uschar name[1];              /* "Times-Roman" or whatever */
} afontstr;

/* For each virtual (logical) font */

typedef struct vfontstr {
  struct vfontstr *next;       /* chain of those actually used */
  int family;                  /* serif/sanserif/monospaced etc. */
  int type;                    /* roman/italic/bold etc */
  int size;
  int leading;
  /*** Fields above are fixed values; fields below are filled in ***/
  int pnumber;                 /* PostScript font number */
  afontstr *afont;             /* the actual font */
  struct vfontstr *sfont[2];   /* pointers to related "special" vfonts */
} vfontstr;

/* For the table of font name suffixes */

typedef struct fontsuffixstr {
  const uschar *familyname;
  const uschar *suffixes[4];
} fontsuffixstr;

/* Table of elements that just do a font change */

typedef struct fontelstr {
  const uschar *name;
  int *fs;
} fontelstr;

/* This is the structure for parameters that hang off items that are elements */

typedef struct paramstr {
  struct paramstr *next;
  uschar name[DBPARAMNAMESIZE];
  uschar seen;
  uschar value[1];
} paramstr;

/* This structure contains a string of characters all in the same font. It is
used for raw input data (hanging off a #PCDATA item), and for formatted lines
(hanging off an outputline structure). Input data is in single textblocks. The
chain facility is used to chain the input blocks of a paragraph together, and
also for chaining the textblocks that make up a formatted output line. */

typedef struct textblock {
  struct textblock *next;      /* Chain for paragraphs and formatted lines */
  struct textblock *lastin;    /* Last input textblock in an output one */
  vfontstr *vfont;             /* Point to virtual font */
  uint32_t pin_flags;          /* Processing instruction flags */
  int colour;                  /* Three 10-bit fields (values 0-1000) */
  int length;                  /* Length of string */
  uschar string[1];            /* The string data */
} textblock;

/* Structure for chaining footnotes to an output line */

typedef struct footnotestr {
  struct footnotestr *next;
  struct item *footnote;
} footnotestr;

/* Block for heading up a formatted output line */

typedef struct outputline {
  struct outputline *next;
  int width;                   /* Width of all chars in the line */
  int stretch;                 /* Amount of space to use for justification */
  int swidth;                  /* Total width of all spaces in the line */
  int scount;                  /* Number of spaces in the line */
  int indent;                  /* Indent for printing */
  int depth;                   /* Depth to move down */
  int flags;                   /* Various flags */
  textblock *txtblk;           /* The first output textblock */
  footnotestr *fnstr;          /* Chain of footnotes */
} outputline;

/* Layout parameters for a paragraph */

typedef struct layoutparam {
  int beforemax;
  int beforemin;
  int aftermax;
  int aftermin;
  int indent1;
  int indent;
  int endent;
  int justify;
  BOOL fill;
} layoutparam;

/* The structure for holding data for a "paragraph", which is a block of
text of a unified kind. */

typedef struct paragraph {
  textblock *intxtblk;         /* The input text blocks */
  outputline *out;             /* The first output line of the output */
  layoutparam *layparm;        /* Layout parameters */
  int maxwidth;                /* Maximum line width */
  int extra_leading;           /* Extra leading for lines in this paragraph */
  int justify;                 /* Overriding justify */
  int charval;                 /* Character for "char" justification */
  int charoff;                 /* Percentage offset for "char" justification */
} paragraph;

/* The structure for holding information about a column in a table */

typedef struct tcolstr {
  int width;
  int align;
  int charval;
  int charoff;
  BOOL sep;
} tcolstr;

/* The structure for holding information about a table */

typedef struct tdatastr {
  int flags;
  int twidth;                  /* The total width */
  int indent;                  /* Overall indent */
  int toprowsize;              /* Point size of top row */
  int colcount;
  layoutparam *layparm;        /* Parameters for this table */
  tcolstr coldata[1];          /* Must be last because it's extended */
} tdatastr;

/* The structure for holding information about a page */

typedef struct pdatastr {
  int available;               /* Available space */
  int used;                    /* Used */
  int stretchable;             /* Potential stretchable space */
} pdatastr;

/* Structure for a string with a length */

typedef struct lengthstring {
  int length;
  uschar value[1];
} lengthstring;

/* Structure for index item */

typedef struct indexstr {
  int     ixnumber;            /* Identifies which index */
  int     pagenumber;          /* INT_MAX for a "see also" item */
  int     endpage;             /* End of range */
  uschar  sorttext[1];         /* Text for primary sorting */
} indexstr;

/* The input file is read into a chain of items. Most of them are DocBook
markup items, but the input text is held in #PCDATA items, formatted paragraphs
are held in #PCPARA items, table information is held in #TDATA items, page data
in #PDATA items, raw chapter and section title strings in #RAWTITLE items, and
there may be others. */

typedef struct item {
  struct item *next;
  struct item *prev;
  struct item *partner;
  int linenumber;
  int flags;
  uschar name[DBNAMESIZE];
  union {
    paramstr *param;
    textblock *txtblk;
    paragraph *prgrph;
    tdatastr *tdata;
    pdatastr *pdata;
    lengthstring *lngthstrng;
    indexstr *ndxstr;
    uschar *string;
  } p;
} item;

/* Structure for bit tables for debugging */

typedef struct bit_table {
  uschar *name;
  unsigned int bit;
} bit_table;

/* Structure for the built-in list of named entities */

typedef struct entity_block {
  uschar *name;
  uschar *value;
} entity_block;

/* Structure for the build-in list of named "variable" entities */

typedef struct ventity_block {
  uschar *name;
  uschar **value;
} ventity_block;

/* Structure for information about head/foot/etc templates */

typedef struct templatestr {
  uschar *filename;            /* File name */
  item  **anchor;              /* Anchor the item list */
  BOOL    ishead;              /* TRUE for head; FALSE for foot */
} templatestr;

/* Structure for a set of pointers to head/foot data */

typedef struct hfstr {
  uschar *foot_centre_recto;
  uschar *foot_centre_verso;
  uschar *foot_left_recto;
  uschar *foot_left_verso;
  uschar *foot_right_recto;
  uschar *foot_right_verso;
  uschar *head_centre_recto;
  uschar *head_centre_verso;
  uschar *head_left_recto;
  uschar *head_left_verso;
  uschar *head_right_recto;
  uschar *head_right_verso;
} hfstr;

/* Structure for a chain of data for creating pdfmark strings at output time.
The chain is created during TOC processing. */

typedef struct pdfmarkstr {
  struct pdfmarkstr *next;
  int    level;                /* 0 => chapter, 1 => section, etc. */
  int    page;                 /* Page number */
  BOOL   ispreface;
  uschar text[1];
} pdfmarkstr;

/* Structure for included file for stack to prevent recursion */

typedef struct incfile {
  struct incfile *next;
  item *end;
  uschar name[1];
} incfile;

/* Structure for lists of pages to be output */

typedef struct pagelist {
  struct pagelist *next;
  unsigned int start;
  unsigned int end;
} pagelist;

/* Structure for a named colour */

typedef struct colourstr {
  uschar name[20];
  int R;
  int G;
  int B;
} colourstr;

/* Unicode character database (UCD) record format. This is copied from PCRE2,
but is used only for detecting letters when hyphenating. All the other
information (such as script) is not relevant. */

typedef struct {
  uint8_t script;     /* ucp_Arabic, etc. */
  uint8_t chartype;   /* ucp_Cc, etc. (general categories) */
  uint8_t gbprop;     /* ucp_gbControl, etc. (grapheme break property) */
  uint8_t caseset;    /* offset to multichar other cases or zero */
  int32_t other_case; /* offset to other case, or zero if none */
  uint16_t scriptx_bidiclass; /* script extension (11 bit) and bidi class (5 bit) values */
  uint16_t bprops;    /* binary properties offset */
} ucd_record;

#endif

/* End of structs.h */
