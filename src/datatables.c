/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2005, last modified March 2023 */

/* This module contains fixed data in the form of various tables. */


#include "sdop.h"



/*************************************************
*   Tables of supported elements and attributes  *
*************************************************/

/* The "revisionflag" attribute is supported on all elements. The "toc"
attribute for tables is not DocBook; it is use used internally by SDoP for the
table that forms the TOC. Some attributes are recognized, but not used in the
formatting. The nocheck_attrs list is used for elements that are completely
ignored. */

uschar *nocheck_attrs[]       = { US"*",          NULL };

uschar *article_attrs[]       = { US"class",      US"id",       NULL };
uschar *chapter_attrs[]       = { US"id",         NULL };
uschar *colspec_attrs[]       = { US"align",      US"char",     US"charoff",
                                  US"colwidth",   US"colsep",   NULL };
uschar *emphasis_attrs[]      = { US"role",       NULL };
uschar *entry_attrs[]         = { US"align",      US"char",     US"charoff",
                                  NULL };
uschar *example_attrs[]       = { US"id",         NULL };
uschar *figure_attrs[]        = { US"id",         NULL };
uschar *footnote_attrs[]      = { US"id",         NULL };
uschar *footnoteref_attrs[]   = { US"linkend",    NULL };
uschar *imagedata_attrs[]     = { US"align",      US"depth",    US"fileref",
                                  US"format",     US"scale",    US"scalefit",
                                  US"width",      NULL };
uschar *imageobject_attrs[]   = { US"role",       NULL };
uschar *index_attrs[]         = { US"role",       NULL };
uschar *indexterm_attrs[]     = { US"class",      US"id",       US"role",
                                  US"startref",   NULL };
uschar *itemizedlist_attrs[]  = { US"mark",       NULL };
uschar *literallayout_attrs[] = { US"class",      NULL };
uschar *orderedlist_attrs[]   = { US"numeration", NULL };
uschar *row_attrs[]           = { US"rowsep",     NULL };
uschar *table_attrs[]         = { US"colsep",     US"frame",    US"id",
                                  US"rowsep",     US"toc",      NULL };
uschar *tgroup_attrs[]        = { US"char",       US"charoff",  US"cols",
                                  US"colsep",     US"rowsep",   NULL };
uschar *ulink_attrs[]         = { US"url",        NULL };
uschar *xref_attrs[]          = { US"linkend",    NULL };


elliststr supported_elements[] = {
  { US"abbrev",            NULL },
  { US"abstract",          NULL },
  { US"acronym",           NULL },
  { US"address",           NULL },
  { US"affiliation",       NULL },
  { US"appendix",          chapter_attrs },
  { US"article",           article_attrs },
  { US"articleinfo",       NULL },
  { US"attribution",       NULL },
  { US"audiodata",         nocheck_attrs },
  { US"audioobject",       nocheck_attrs },
  { US"author",            NULL },
  { US"authorblurb",       nocheck_attrs },
  { US"authorinitials",    NULL },
  { US"blockquote",        NULL },
  { US"book",              NULL },
  { US"bookinfo",          NULL },
  { US"caption",           NULL },
  { US"chapter",           chapter_attrs },
  { US"citetitle",         emphasis_attrs },  /* sic */
  { US"colophon",          NULL },
  { US"colspec",           colspec_attrs },
  { US"command",           NULL },
  { US"computeroutput",    NULL },
  { US"copyright",         NULL },
  { US"corpauthor",        NULL },
  { US"date",              NULL },
  { US"edition",           NULL },
  { US"editor",            NULL },
  { US"email",             emphasis_attrs },  /* sic */
  { US"emphasis",          emphasis_attrs },
  { US"entry",             entry_attrs },
  { US"epigraph",          NULL },
  { US"example",           example_attrs },
  { US"figure",            figure_attrs },
  { US"filename",          NULL },
  { US"firstname",         NULL },
  { US"footnote",          footnote_attrs },
  { US"footnoteref",       footnoteref_attrs },
  { US"formalpara",        NULL },
  { US"function",          NULL },
  { US"holder",            NULL },
  { US"honorific",         NULL },
  { US"imagedata",         imagedata_attrs },
  { US"imageobject",       imageobject_attrs },
  { US"index",             index_attrs },
  { US"indexterm",         indexterm_attrs },
  { US"informalfigure",    figure_attrs },
  { US"informaltable",     table_attrs },
  { US"inlinemediaobject", NULL },
  { US"issuenum",          NULL },
  { US"itemizedlist",      itemizedlist_attrs },
  { US"jobtitle",          NULL },
  { US"keyword",           NULL },
  { US"keywordset",        NULL },
  { US"legalnotice",       NULL },
  { US"lineage",           NULL },
  { US"lineannotation",    NULL },
  { US"link",              nocheck_attrs },
  { US"listitem",          NULL },
  { US"literal",           NULL },
  { US"literallayout",     literallayout_attrs },
  { US"mediaobject",       NULL },
  { US"note",              NULL },
  { US"objectinfo",        NULL },
  { US"option",            NULL },
  { US"orderedlist",       orderedlist_attrs },
  { US"orgname",           NULL },
  { US"othercredit",       NULL },
  { US"othername",         NULL },
  { US"para",              NULL },
  { US"phrase",            NULL },
  { US"preface",           NULL },
  { US"primary",           NULL },
  { US"programlisting",    NULL },
  { US"pubdate",           NULL },
  { US"publishername",     NULL },
  { US"quote",             NULL },
  { US"releaseinfo",       NULL },
  { US"replaceable",       NULL },
  { US"revdescription",    NULL },
  { US"revhistory",        NULL },
  { US"revision",          NULL },
  { US"revnumber",         NULL },
  { US"revremark",         nocheck_attrs },
  { US"row",               row_attrs },
  { US"screen",            NULL },
  { US"secondary",         NULL },
  { US"sect1",             chapter_attrs },
  { US"sect2",             chapter_attrs },
  { US"sect3",             chapter_attrs },
  { US"sect4",             chapter_attrs },
  { US"sect5",             chapter_attrs },
  { US"section",           chapter_attrs },
  { US"sectioninfo",       NULL },
  { US"see",               NULL },
  { US"seealso",           NULL },
  { US"sidebar",           NULL },
  { US"simpara",           NULL },
  { US"subject",           nocheck_attrs },
  { US"subjectset",        nocheck_attrs },
  { US"subjectterm",       nocheck_attrs },
  { US"subscript",         NULL },
  { US"subtitle",          NULL },
  { US"superscript",       NULL },
  { US"surname",           NULL },
  { US"systemitem",        nocheck_attrs },
  { US"table",             table_attrs },
  { US"tbody",             NULL },
  { US"term",              NULL },
  { US"tertiary",          NULL },
  { US"textobject",        NULL },
  { US"tfoot",             NULL },
  { US"tgroup",            tgroup_attrs },
  { US"thead",             NULL },
  { US"title",             NULL },
  { US"titleabbrev",       NULL },
  { US"trademark",         nocheck_attrs },
  { US"ulink",             ulink_attrs },
  { US"userinput",         NULL },
  { US"variablelist",      NULL },
  { US"varlistentry",      NULL },
  { US"varname",           NULL },
  { US"videodata",         nocheck_attrs },
  { US"videoobject",       nocheck_attrs },
  { US"volumenum",         NULL },
  { US"xref",              xref_attrs },
  { US"year",              NULL }
};

int supported_elements_count = sizeof(supported_elements)/sizeof(elliststr);


/*************************************************
*     Table of elements that turn into text      *
*************************************************/

uschar *text_elements[] = { US"footnote", US"footnoteref", US"quote",
                            US"ulink",    US"xref",        NULL };


/*************************************************
*            Tables of named entities            *
*************************************************/

/* This table must be in collating sequence of entity name, because it is
searched by binary chop. If a replacement starts with "&#x" this in turn is
interpreted as a numerical entity. Other replacements are not re-scanned. In
particular, the special entities for use in heads and feet are not replaced by
this table since their replacements are themselves - they are reprocessed later
for each head and foot.

The entities that are commented out are some that are listed in the DocBook
specification, but which do not correspond to characters in the PostScript
fonts. Some of those that are not commented out (e.g. Cdot) are recognized by
SDoP because their codes are less than 017F, but as they don't have glyphs in
the PostScript fonts, they are output as the currency symbol. */

entity_block entity_list[] = {
  { US"AElig",             US"&#x00c6" },
  { US"Aacute",            US"&#x00c1" },
  { US"Abreve",            US"&#x0102" },
  { US"Acirc",             US"&#x00c2" },
  { US"Agrave",            US"&#x00c0" },
  { US"Amacr",             US"&#x0100" },
  { US"Aogon",             US"&#x0104" },
  { US"Aring",             US"&#x00c5" },
  { US"Atilde",            US"&#x00c3" },
  { US"Auml",              US"&#x00c4" },
  { US"Cacute",            US"&#x0106" },
  { US"Ccaron",            US"&#x010c" },
  { US"Ccedil",            US"&#x00c7" },
  { US"Ccirc",             US"&#x0108" },
  { US"Cdot",              US"&#x010a" },
  { US"Dagger",            US"&#x2021" },
  { US"Dcaron",            US"&#x010e" },
  { US"Dstrok",            US"&#x0110" },
  { US"ENG",               US"&#x014a" },
  { US"ETH",               US"&#x00d0" },
  { US"Eacute",            US"&#x00c9" },
  { US"Ecaron",            US"&#x011a" },
  { US"Ecirc",             US"&#x00ca" },
  { US"Edot",              US"&#x0116" },
  { US"Egrave",            US"&#x00c8" },
  { US"Emacr",             US"&#x0112" },
  { US"Eogon",             US"&#x0118" },
  { US"Euml",              US"&#x00cb" },
  { US"Euro",              US"&#x20ac" },
  { US"Gbreve",            US"&#x011e" },
  { US"Gcedil",            US"&#x0122" },
  { US"Gcirc",             US"&#x011c" },
  { US"Gdot",              US"&#x0120" },
  { US"Hcirc",             US"&#x0124" },
  { US"Hstrok",            US"&#x0126" },
  { US"IJlig",             US"&#x0132" },
  { US"Iacute",            US"&#x00cd" },
  { US"Icirc",             US"&#x00ce" },
  { US"Idot",              US"&#x0130" },
  { US"Igrave",            US"&#x00cc" },
  { US"Imacr",             US"&#x012a" },
  { US"Iogon",             US"&#x012e" },
  { US"Itilde",            US"&#x0128" },
  { US"Iuml",              US"&#x00cf" },
  { US"Jcirc",             US"&#x0134" },
  { US"Kcedil",            US"&#x0136" },
  { US"Lacute",            US"&#x0139" },
  { US"Lcaron",            US"&#x013d" },
  { US"Lcedil",            US"&#x013b" },
  { US"Lmidot",            US"&#x013f" },
  { US"Lstrok",            US"&#x0141" },
  { US"Nacute",            US"&#x0143" },
  { US"Ncaron",            US"&#x0147" },
  { US"Ncedil",            US"&#x0145" },
  { US"Ntilde",            US"&#x00d1" },
  { US"OElig",             US"&#x0152" },
  { US"Oacute",            US"&#x00d3" },
  { US"Ocirc",             US"&#x00d4" },
  { US"Odblac",            US"&#x0150" },
  { US"Ograve",            US"&#x00d2" },
  { US"Omacr",             US"&#x014c" },
  { US"Oslash",            US"&#x00d8" },
  { US"Otilde",            US"&#x00d5" },
  { US"Ouml",              US"&#x00d6" },
  { US"Racute",            US"&#x0154" },
  { US"Rcaron",            US"&#x0158" },
  { US"Rcedil",            US"&#x0156" },
  { US"Sacute",            US"&#x015a" },
  { US"Scaron",            US"&#x0160" },
  { US"Scedil",            US"&#x015e" },
  { US"Scirc",             US"&#x015c" },
  { US"THORN",             US"&#x00de" },
  { US"Tcaron",            US"&#x0164" },
  { US"Tcedil",            US"&#x0162" },
  { US"Tstrok",            US"&#x0166" },
  { US"Uacute",            US"&#x00da" },
  { US"Ubreve",            US"&#x016c" },
  { US"Ucirc",             US"&#x00db" },
  { US"Udblac",            US"&#x0170" },
  { US"Ugrave",            US"&#x00d9" },
  { US"Umacr",             US"&#x016a" },
  { US"Uogon",             US"&#x0172" },
  { US"Uring",             US"&#x016e" },
  { US"Utilde",            US"&#x0168" },
  { US"Uuml",              US"&#x00dc" },
  { US"Wcirc",             US"&#x0174" },
  { US"Yacute",            US"&#x00dd" },
  { US"Ycirc",             US"&#x0176" },
  { US"Yuml",              US"&#x0178" },
  { US"Zacute",            US"&#x0179" },
  { US"Zcaron",            US"&#x017d" },
  { US"Zdot",              US"&#x017b" },
  { US"aacute",            US"&#x00e1" },
  { US"abreve",            US"&#x0103" },
  { US"acirc",             US"&#x00e2" },
  { US"aelig",             US"&#x00e6" },
  { US"agrave",            US"&#x00e0" },
  { US"amacr",             US"&#x0101" },
  { US"amp",               US"&" },
  { US"aogon",             US"&#x0105" },
  { US"apos",              US"'" },
  { US"aring",             US"&#x00e5" },
  { US"atilde",            US"&#x00e3" },
  { US"auml",              US"&#x00e4" },
/*  { US"blank",             US"&#x2423" }, */
/*  { US"blk12",             US"&#x2592" }, */
/*  { US"blk14",             US"&#x2591" }, */
/*  { US"blk34",             US"&#x2593" }, */
/*  { US"block",             US"&#x2588" }, */
  { US"brvbar",            US"&#x00a6" },
/*  { US"bull",              US"&#x2022" }, */
  { US"cacute",            US"&#x0107" },
/*  { US"caret",             US"&#x2041" }, */
  { US"ccaron",            US"&#x010d" },
  { US"ccedil",            US"&#x00e7" },
  { US"ccirc",             US"&#x0109" },
  { US"cdot",              US"&#x010b" },
  { US"cent",              US"&#x00a2" },
  { US"check",             US"&#x2713" },
/*  { US"cir",               US"&#x25cb" }, */
  { US"clubs",             US"&#x2663" },
  { US"copy",              US"&#x00a9" },
/*  { US"copysr",            US"&#x2117" }, */
  { US"cross",             US"&#x2717" },
  { US"curren",            US"&#x00a4" },
  { US"dagger",            US"&#x2020" },
  { US"darr",              US"&#x2193" },
/*  { US"dash",              US"&#x2010" }, */
  { US"dcaron",            US"&#x010f" },
  { US"deg",               US"&#x00b0" },
  { US"diams",             US"&#x2666" },
  { US"divide",            US"&#x00f7" },
/*  { US"dlcrop",            US"&#x230d" }, */
/*  { US"drcrop",            US"&#x230c" }, */
  { US"dstrok",            US"&#x0111" },
/*  { US"dtri",              US"&#x25bf" }, */
/*  { US"dtrif",             US"&#x25be" }, */
  { US"eacute",            US"&#x00e9" },
  { US"ecaron",            US"&#x011b" },
  { US"ecirc",             US"&#x00ea" },
  { US"edot",              US"&#x0117" },
  { US"egrave",            US"&#x00e8" },
  { US"emacr",             US"&#x0113" },
  { US"eng",               US"&#x014b" },
  { US"eogon",             US"&#x0119" },
  { US"eth",               US"&#x00f0" },
  { US"euml",              US"&#x00eb" },
/*  { US"female",            US"&#x2640" }, */
/*  { US"ffilig",            US"&#xfb03" }, */
/*  { US"fflig",             US"&#xfb00" }, */
/*  { US"ffllig",            US"&#xfb04" }, */
  { US"filig",             US"&#xfb01" },
/*  { US"flat",              US"&#x266d" }, */
  { US"fllig",             US"&#xfb02" },
  { US"footcenter",        US"&footcentre;" },
  { US"footcentre",        US"&footcentre;" },
  { US"footleft",          US"&footleft;" },
  { US"footright",         US"&footright;" },
  { US"frac12",            US"&#x00bd" },
/*  { US"frac13",            US"&#x2153" }, */
  { US"frac14",            US"&#x00bc" },
/*  { US"frac15",            US"&#x2155" }, */
/*  { US"frac16",            US"&#x2159" }, */
/*  { US"frac18",            US"&#x215b" }, */
/*  { US"frac23",            US"&#x2154" }, */
/*  { US"frac25",            US"&#x2156" }, */
  { US"frac34",            US"&#x00be" },
/*  { US"frac35",            US"&#x2157" }, */
/*  { US"frac38",            US"&#x215c" }, */
/*  { US"frac45",            US"&#x2158" }, */
/*  { US"frac56",            US"&#x215a" }, */
/*  { US"frac58",            US"&#x215d" }, */
/*  { US"frac78",            US"&#x215e" }, */
/*  { US"gacute",            US"&#x01f5" }, */
  { US"gbreve",            US"&#x011f" },
  { US"gcirc",             US"&#x011d" },
  { US"gdot",              US"&#x0121" },
  { US"gt",                US">" },
  { US"half",              US"&#x00bd" },
  { US"hcirc",             US"&#x0125" },
  { US"headcenter",        US"&headcentre;" },
  { US"headcentre",        US"&headcentre;" },
  { US"headleft",          US"&headleft;" },
  { US"headright",         US"&headright;" },
  { US"hearts",            US"&#x2665" },
  { US"hellip",            US"&#x2026" },
/*  { US"horbar",            US"&#x2015" }, */
  { US"hstrok",            US"&#x0127" },
/*  { US"hybull",            US"&#x2043" }, */
  { US"iacute",            US"&#x00ed" },
  { US"icirc",             US"&#x00ee" },
  { US"iexcl",             US"&#x00a1" },
  { US"igrave",            US"&#x00ec" },
  { US"ijlig",             US"&#x0133" },
  { US"imacr",             US"&#x012b" },
/*  { US"incare",            US"&#x2105" }, */
  { US"inodot",            US"&#x0131" },
  { US"iogon",             US"&#x012f" },
  { US"iquest",            US"&#x00bf" },
  { US"itilde",            US"&#x0129" },
  { US"iuml",              US"&#x00ef" },
  { US"jcirc",             US"&#x0135" },
  { US"kcedil",            US"&#x0137" },
  { US"kgreen",            US"&#x0138" },
  { US"lacute",            US"&#x013a" },
  { US"laquo",             US"&#x00ab" },
  { US"larr",              US"&#x2190" },
  { US"lcaron",            US"&#x013e" },
  { US"lcedil",            US"&#x013c" },
  { US"ldquo",             US"&#x201c" },
  { US"ldquor",            US"&#x201e" },
/*  { US"lhblk",             US"&#x2584" }, */
  { US"lmidot",            US"&#x0140" },
  { US"loz",               US"&#x25ca" },
  { US"lsquo",             US"&#x2018" },
  { US"lsquor",            US"&#x201a" },
  { US"lstrok",            US"&#x0142" },
  { US"lt",                US"<" },
/*  { US"ltri",              US"&#x25c3" }, */
/*  { US"ltrif",             US"&#x25c2" }, */
/*  { US"male",              US"&#x2642" }, */
  { US"malt",              US"&#x2720" },
/*  { US"marker",            US"&#x25ae" }, */
  { US"mdash",             US"&#x2014" },
  { US"micro",             US"&#x00b5" },
  { US"middot",            US"&#x00b7" },
  { US"mldr",              US"&#x2026" },
  { US"nacute",            US"&#x0144" },
  { US"napos",             US"&#x0149" },
/*  { US"natur",             US"&#x266e" }, */
  { US"nbsp",              US"&#x00a0" },
  { US"ncaron",            US"&#x0148" },
  { US"ncedil",            US"&#x0146" },
  { US"ndash",             US"&#x2013" },
/*  { US"nldr",              US"&#x2025" }, */
  { US"not",               US"&#x00ac" },
  { US"ntilde",            US"&#x00f1" },
  { US"oacute",            US"&#x00f3" },
  { US"ocirc",             US"&#x00f4" },
  { US"odblac",            US"&#x0151" },
  { US"oelig",             US"&#x0153" },
  { US"ograve",            US"&#x00f2" },
/*  { US"ohm",               US"&#x2126" }, */
  { US"omacr",             US"&#x014d" },
  { US"ordf",              US"&#x00aa" },
  { US"ordm",              US"&#x00ba" },
  { US"oslash",            US"&#x00f8" },
  { US"otilde",            US"&#x00f5" },
  { US"ouml",              US"&#x00f6" },
  { US"para",              US"&#x00b6" },
  { US"phone",             US"&#x260e" },
  { US"plusmn",            US"&#x00b1" },
  { US"pound",             US"&#x00a3" },
  { US"quot",              US"\"" },
  { US"racute",            US"&#x0155" },
  { US"raquo",             US"&#x00bb" },
  { US"rarr",              US"&#x2192" },
  { US"rcaron",            US"&#x0159" },
  { US"rcedil",            US"&#x0157" },
  { US"rdquo",             US"&#x201d" },
  { US"rdquor",            US"&#x201d" },
/*  { US"rect",              US"&#x25ad" }, */
  { US"reg",               US"&#x00ae" },
  { US"rsquo",             US"&#x2019" },
  { US"rsquor",            US"&#x2019" },
/*  { US"rtri",              US"&#x25b9" }, */
/*  { US"rtrif",             US"&#x25b8" }, */
/*  { US"rx",                US"&#x211e" }, */
  { US"sacute",            US"&#x015b" },
  { US"scaron",            US"&#x0161" },
  { US"scedil",            US"&#x015f" },
  { US"scirc",             US"&#x015d" },
  { US"sect",              US"&#x00a7" },
  { US"sext",              US"&#x2736" },
/*  { US"sharp",             US"&#x266f" }, */
  { US"shy",               US"&#x00ad" },
  { US"spades",            US"&#x2660" },
/*  { US"squ",               US"&#x25a1" }, */
/*  { US"squf",              US"&#x25aa" }, */
  { US"sup1",              US"&#x00b9" },
  { US"sup2",              US"&#x00b2" },
  { US"sup3",              US"&#x00b3" },
  { US"szlig",             US"&#x00df" },
/*  { US"target",            US"&#x2316" }, */
  { US"tcaron",            US"&#x0165" },
  { US"tcedil",            US"&#x0163" },
/*  { US"telrec",            US"&#x2315" }, */
  { US"thorn",             US"&#x00fe" },
  { US"times",             US"&#x00d7" },
  { US"trade",             US"&#x2122" },
  { US"tstrok",            US"&#x0167" },
  { US"uacute",            US"&#x00fa" },
  { US"uarr",              US"&#x2191" },
  { US"ubreve",            US"&#x016d" },
  { US"ucirc",             US"&#x00fb" },
  { US"udblac",            US"&#x0171" },
  { US"ugrave",            US"&#x00f9" },
/*  { US"uhblk",             US"&#x2580" }, */
/*  { US"ulcrop",            US"&#x230f" }, */
  { US"umacr",             US"&#x016b" },
  { US"uogon",             US"&#x0173" },
/*  { US"urcrop",            US"&#x230e" }, */
  { US"uring",             US"&#x016f" },
  { US"utilde",            US"&#x0169" },
/*  { US"utri",              US"&#x25b5" }, */
/*  { US"utrif",             US"&#x25b4" }, */
  { US"uuml",              US"&#x00fc" },
/*  { US"vellip",            US"&#x22ee" }, */
  { US"wcirc",             US"&#x0175" },
  { US"yacute",            US"&#x00fd" },
  { US"ycirc",             US"&#x0177" },
  { US"yen",               US"&#x00a5" },
  { US"yuml",              US"&#x00ff" },
  { US"zacute",            US"&#x017a" },
  { US"zcaron",            US"&#x017e" },
  { US"zdot",              US"&#x017c" }
};

int entity_list_count = sizeof(entity_list)/sizeof(entity_block);

/* These "entities" give access to data that is provided by the <bookinfo>
or <articleinfo> element amongst other sources, that is, to "variables". */

ventity_block ventity_list[] = {
  { US"author_address",         &author_address },
  { US"author_corpauthor",      &author_corpauthor },
  { US"author_firstname",       &author_firstname },
  { US"author_honorific",       &author_honorific },
  { US"author_initials",        &author_initials },
  { US"author_jobtitle",        &author_jobtitle },
  { US"author_lineage",         &author_lineage },
  { US"author_orgname",         &author_orgname },
  { US"author_othername",       &author_othername },
  { US"author_surname",         &author_surname },
  { US"book_cpyholder",         &book_cpyholder },
  { US"book_cpyyear",           &book_cpyyear },
  { US"book_date",              &book_date },
  { US"book_edition",           &book_edition },
  { US"book_issuenum",          &book_issuenum },
  { US"book_pubdate",           &book_pubdate },
  { US"book_publishername",     &book_publishername },
  { US"book_releaseinfo",       &book_releaseinfo },
  { US"book_revauthorinitials", &book_revauthorinitials },
  { US"book_revdate",           &book_revdate },
  { US"book_revnumber",         &book_revnumber },
  { US"book_subtitle",          &book_subtitle },
  { US"book_title",             &book_title },
  { US"book_titleabbrev",       &book_titleabbrev },
  { US"book_volumenum",         &book_volumenum },
  { US"editor_firstname",       &editor_firstname },
  { US"editor_honorific",       &editor_honorific },
  { US"editor_initials",        &editor_initials },
  { US"editor_jobtitle",        &editor_jobtitle },
  { US"editor_lineage",         &editor_lineage },
  { US"editor_orgname",         &editor_orgname },
  { US"editor_othername",       &editor_othername },
  { US"editor_surname",         &editor_surname },
  { US"othercredit_firstname",  &othercredit_firstname },
  { US"othercredit_honorific",  &othercredit_honorific },
  { US"othercredit_initials",   &othercredit_initials },
  { US"othercredit_jobtitle",   &othercredit_jobtitle },
  { US"othercredit_lineage",    &othercredit_lineage },
  { US"othercredit_orgname",    &othercredit_orgname },
  { US"othercredit_othername",  &othercredit_othername },
  { US"othercredit_surname",    &othercredit_surname },
  { US"toc_title",              &toc_title }
};

int ventity_list_count = sizeof(ventity_list)/sizeof(ventity_block);



/*************************************************
*           Miscellaneous fixed tables           *
*************************************************/

int   bullets_default[] = { BULLET_BULLET, BULLET_DASH, BULLET_OCIRCLE, 0 };

BOOL  number_sections_default[MAXSECTDEPTH] = { TRUE, TRUE, TRUE };



/*************************************************
*          Tables of font names, etc             *
*************************************************/

/* These are used only in debugging output. */

uschar *family_names[] =
  { US"ser", US"san", US"mon", US"spe" };

uschar *type_names[] =
  { US"rm", US"it", US"bf", US"bi", US"sp" };

/* Suffixes for the different font families */

fontsuffixstr fontsuffixes[] = {
  { US"AvantGarde",       { US"-Book",    US"-BookOblique", US"-Demi", US"-DemiOblique" } },
  { US"Bookman",          { US"-Light",   US"-LightItalic", US"-Demi", US"-DemiItalic"  } },
  { US"Courier",          { US"",         US"-Oblique",     US"-Bold", US"-BoldOblique" } },
  { US"Helvetica",        { US"",         US"-Oblique",     US"-Bold", US"-BoldOblique" } },
  { US"NewCenturySchlbk", { US"-Roman",   US"-Italic",      US"-Bold", US"-BoldItalic"  } },
  { US"Palatino",         { US"-Roman",   US"-Italic",      US"-Bold", US"-BoldItalic"  } },
  { US"Times",            { US"-Roman",   US"-Italic",      US"-Bold", US"-BoldItalic"  } },
  { US"Utopia",           { US"-Regular", US"-Italic",      US"-Bold", US"-BoldItalic"  } },
  { NULL, { NULL, NULL, NULL } }
};

/* Elements that just change font state, but can be configured */

fontelstr fontels[] = {
  { US"command",     &command_fs },
  { US"filename",    &filename_fs },
  { US"function",    &function_fs },
  { US"option",      &option_fs },
  { US"replaceable", &replaceable_fs },
  { US"userinput",   &userinput_fs },
  { US"varname",     &varname_fs },
  { NULL, NULL }
  };

/* Names of special fonts */

uschar *sfontname[] = { US"Symbol", US"ZapfDingbats" };

/* Tables for font selection. If additional fonts are added for each kind of
text, the definitions of FONTS_xxx must be changed. */

vfontstr *romanfonts[] = {
  /* Main text */
  &maintext_vfont,
  &maintext_vfont,
  &maintext_vfont,
  &maintext_vfont,
  &monotext_vfont,
  &monotext_vfont,
  &monotext_vfont,
  &monotext_vfont,
  /* Small main text */
  &small_maintext_vfont,
  &small_maintext_vfont,
  &small_maintext_vfont,
  &small_maintext_vfont,
  &small_monotext_vfont,
  &small_monotext_vfont,
  &small_monotext_vfont,
  &small_monotext_vfont,
  /* TOC text */
  &toc_maintext_vfont,
  &toc_maintext_vfont,
  &toc_maintext_vfont,
  &toc_maintext_vfont,
  &monotext_vfont,
  &monotext_vfont,
  &monotext_vfont,
  &monotext_vfont,
  /* Index text */
  &index_maintext_vfont,
  &index_maintext_vfont,
  &index_maintext_vfont,
  &index_maintext_vfont,
  &index_monotext_vfont,
  &index_monotext_vfont,
  &index_monotext_vfont,
  &index_monotext_vfont,
  /* Title pages text */
  &title_maintext_vfont,
  &title_maintext_vfont,
  &title_maintext_vfont,
  &title_maintext_vfont,
  &title_monotext_vfont,
  &title_monotext_vfont,
  &title_monotext_vfont,
  &title_monotext_vfont,
  /* Foonote text */
  &footnote_maintext_vfont,
  &footnote_maintext_vfont,
  &footnote_maintext_vfont,
  &footnote_maintext_vfont,
  &footnote_monotext_vfont,
  &footnote_monotext_vfont,
  &footnote_monotext_vfont,
  &footnote_monotext_vfont,
  /* Head/foot text */
  &headfoot_maintext_vfont,
  &headfoot_maintext_vfont,
  &headfoot_maintext_vfont,
  &headfoot_maintext_vfont,
  &headfoot_monotext_vfont,
  &headfoot_monotext_vfont,
  &headfoot_monotext_vfont,
  &headfoot_monotext_vfont
  };

vfontstr *boldfonts[] = {
  /* Main text */
  &boldtext_vfont,
  &boittext_vfont,
  &boldtext_vfont,
  &boittext_vfont,
  &monobold_vfont,
  &monoboit_vfont,
  &monobold_vfont,
  &monoboit_vfont,
  /* Small main text */
  &small_boldtext_vfont,
  &small_boittext_vfont,
  &small_boldtext_vfont,
  &small_boittext_vfont,
  &small_monobold_vfont,
  &small_monoboit_vfont,
  &small_monobold_vfont,
  &small_monoboit_vfont,
  /* TOC text */
  &toc_boldtext_vfont,
  &toc_boittext_vfont,
  &toc_boldtext_vfont,
  &toc_boittext_vfont,
  &monobold_vfont,
  &monoboit_vfont,
  &monobold_vfont,
  &monoboit_vfont,
  /* Index text */
  &index_boldtext_vfont,
  &index_boittext_vfont,
  &index_boldtext_vfont,
  &index_boittext_vfont,
  &index_monobold_vfont,
  &index_monoboit_vfont,
  &index_monobold_vfont,
  &index_monoboit_vfont,
  /* Title pages text */
  &title_boldtext_vfont,
  &title_boittext_vfont,
  &title_boldtext_vfont,
  &title_boittext_vfont,
  &title_monobold_vfont,
  &title_monoboit_vfont,
  &title_monobold_vfont,
  &title_monoboit_vfont,
  /* Foonote text */
  &footnote_boldtext_vfont,
  &footnote_boittext_vfont,
  &footnote_boldtext_vfont,
  &footnote_boittext_vfont,
  &footnote_monobold_vfont,
  &footnote_monoboit_vfont,
  &footnote_monobold_vfont,
  &footnote_monoboit_vfont,
  /* Head/foot text */
  &headfoot_boldtext_vfont,
  &headfoot_boittext_vfont,
  &headfoot_boldtext_vfont,
  &headfoot_boittext_vfont,
  &headfoot_monobold_vfont,
  &headfoot_monoboit_vfont,
  &headfoot_monobold_vfont,
  &headfoot_monoboit_vfont
  };

vfontstr *italfonts[] = {
  /* Main text */
  &italtext_vfont,
  &italtext_vfont,
  &boittext_vfont,
  &boittext_vfont,
  &monoital_vfont,
  &monoital_vfont,
  &monoboit_vfont,
  &monoboit_vfont,
  /* Small main text */
  &small_italtext_vfont,
  &small_italtext_vfont,
  &small_boittext_vfont,
  &small_boittext_vfont,
  &small_monoital_vfont,
  &small_monoital_vfont,
  &small_monoboit_vfont,
  &small_monoboit_vfont,
  /* TOC text */
  &toc_italtext_vfont,
  &toc_italtext_vfont,
  &toc_boittext_vfont,
  &toc_boittext_vfont,
  &monoital_vfont,
  &monoital_vfont,
  &monoboit_vfont,
  &monoboit_vfont,
  /* Index text */
  &index_italtext_vfont,
  &index_italtext_vfont,
  &index_boittext_vfont,
  &index_boittext_vfont,
  &index_monoital_vfont,
  &index_monoital_vfont,
  &index_monoboit_vfont,
  &index_monoboit_vfont,
  /* Title pages text */
  &title_italtext_vfont,
  &title_italtext_vfont,
  &title_boittext_vfont,
  &title_boittext_vfont,
  &title_monoital_vfont,
  &title_monoital_vfont,
  &title_monoboit_vfont,
  &title_monoboit_vfont,
  /* Footnote text */
  &footnote_italtext_vfont,
  &footnote_italtext_vfont,
  &footnote_boittext_vfont,
  &footnote_boittext_vfont,
  &footnote_monoital_vfont,
  &footnote_monoital_vfont,
  &footnote_monoboit_vfont,
  &footnote_monoboit_vfont,
  /* Head/foot text */
  &headfoot_italtext_vfont,
  &headfoot_italtext_vfont,
  &headfoot_boittext_vfont,
  &headfoot_boittext_vfont,
  &headfoot_monoital_vfont,
  &headfoot_monoital_vfont,
  &headfoot_monoboit_vfont,
  &headfoot_monoboit_vfont
  };

vfontstr *monofonts[] = {
  /* Main text */
  &monotext_vfont,
  &monoital_vfont,
  &monobold_vfont,
  &monoboit_vfont,
  &monotext_vfont,
  &monoital_vfont,
  &monobold_vfont,
  &monoboit_vfont,
  /* Small main text */
  &small_monotext_vfont,
  &small_monoital_vfont,
  &small_monobold_vfont,
  &small_monoboit_vfont,
  &small_monotext_vfont,
  &small_monoital_vfont,
  &small_monobold_vfont,
  &small_monoboit_vfont,
  /* TOC text */
  &monotext_vfont,
  &monoital_vfont,
  &monobold_vfont,
  &monoboit_vfont,
  &monotext_vfont,
  &monoital_vfont,
  &monobold_vfont,
  &monoboit_vfont,
  /* Index text */
  &index_monotext_vfont,
  &index_monoital_vfont,
  &index_monobold_vfont,
  &index_monoboit_vfont,
  &index_monotext_vfont,
  &index_monoital_vfont,
  &index_monobold_vfont,
  &index_monoboit_vfont,
  /* Title pages text */
  &title_monotext_vfont,
  &title_monoital_vfont,
  &title_monobold_vfont,
  &title_monoboit_vfont,
  &title_monotext_vfont,
  &title_monoital_vfont,
  &title_monobold_vfont,
  &title_monoboit_vfont,
  /* Footnote text */
  &footnote_monotext_vfont,
  &footnote_monoital_vfont,
  &footnote_monobold_vfont,
  &footnote_monoboit_vfont,
  &footnote_monotext_vfont,
  &footnote_monoital_vfont,
  &footnote_monobold_vfont,
  &footnote_monoboit_vfont,
  /* Head/foot text */
  &headfoot_monotext_vfont,
  &headfoot_monoital_vfont,
  &headfoot_monobold_vfont,
  &headfoot_monoboit_vfont,
  &headfoot_monotext_vfont,
  &headfoot_monoital_vfont,
  &headfoot_monobold_vfont,
  &headfoot_monoboit_vfont
  };

/* Table for layout parameters - main text and footnotes. */

layoutparam *lptable[] = {
  &formalpara_layparm,
  &para_layparm,
  &blockquote_layparm,
  &sidebar_layparm,
  &note_layparm,
  &ilistpara_layparm,
  &olistpara_layparm,
  &vlistpara_layparm,
  &vlistpara1_layparm,
  &term_layparm,
  &termfirst_layparm,
  &termmid_layparm,
  &termlast_layparm,
  &literalpara_layparm,
  &table_layparm,
  &framed_table_layparm,
  &table_title_layparm,
  &figure_title_layparm,
  &example_title_layparm,
  /* Footnotes */
  &footnote_formalpara_layparm,
  &footnote_para_layparm,
  &footnote_blockquote_layparm,
  &footnote_sidebar_layparm,
  &footnote_note_layparm,
  &footnote_ilistpara_layparm,
  &footnote_olistpara_layparm,
  &footnote_vlistpara_layparm,
  &footnote_vlistpara1_layparm,
  &footnote_term_layparm,
  &footnote_termfirst_layparm,
  &footnote_termmid_layparm,
  &footnote_termlast_layparm,
  &footnote_literalpara_layparm,
  &footnote_table_layparm,
  &footnote_framed_table_layparm,
  &footnote_para_layparm,     /* Don't expect titled tables in footnotes */
  &footnote_para_layparm,     /* Don't expect titled footnotes in footnotes */
  &footnote_para_layparm      /* Don't expect titled examples in footnotes */
  };


/*************************************************
*         Various character tables               *
*************************************************/

/* This table translates character names from PostScript fonts that use Adobe's
standard encoding into Unicode. In addition, for characters whose Unicode
values are greater than LOWCHARLIMIT, it includes the offset above LOWCHARLIMIT
that we use for printing these characters. */

an2uencod an2ulist[] = {
  { US"A",               0x0041, -1 },
  { US"AE",              0x00c6, -1 },
  { US"Aacute",          0x00c1, -1 },
  { US"Abreve",          0x0102, -1 },
  { US"Acircumflex",     0x00c2, -1 },
  { US"Adieresis",       0x00c4, -1 },
  { US"Agrave",          0x00c0, -1 },
  { US"Amacron",         0x0100, -1 },
  { US"Aogonek",         0x0104, -1 },
  { US"Aring",           0x00c5, -1 },
  { US"Atilde",          0x00c3, -1 },
  { US"B",               0x0042, -1 },
  { US"C",               0x0043, -1 },
  { US"Cacute",          0x0106, -1 },
  { US"Ccaron",          0x010c, -1 },
  { US"Ccedilla",        0x00c7, -1 },
  { US"Ccircumflex",     0x0108, -1 },
  { US"Cdotaccent",      0x010a, -1 },
  { US"D",               0x0044, -1 },
  { US"Dcaron",          0x010e, -1 },
  { US"Dcroat",          0x0110, -1 },
  { US"Delta",           0x0394, +0 },
  { US"E",               0x0045, -1 },
  { US"Eacute",          0x00c9, -1 },
  { US"Ebreve",          0x0114, -1 },
  { US"Ecaron",          0x011a, -1 },
  { US"Ecircumflex",     0x00ca, -1 },
  { US"Edieresis",       0x00cb, -1 },
  { US"Edotaccent",      0x0116, -1 },
  { US"Egrave",          0x00c8, -1 },
  { US"Emacron",         0x0112, -1 },
  { US"Eng",             0x014a, -1 },
  { US"Eogonek",         0x0118, -1 },
  { US"Eth",             0x00d0, -1 },
  { US"Euro",            0x20ac, +1 },
  { US"F",               0x0046, -1 },
  { US"G",               0x0047, -1 },
  { US"Gbreve",          0x011e, -1 },
  { US"Gcircumflex",     0x011c, -1 },
  { US"Gcommaaccent",    0x0122, -1 },
  { US"Gdotaccent",      0x0120, -1 },
  { US"H",               0x0048, -1 },
  { US"Hbar",            0x0126, -1 },
  { US"Hcircumflex",     0x0124, -1 },
  { US"I",               0x0049, -1 },
  { US"IJ",              0x0132, -1 },
  { US"Iacute",          0x00cd, -1 },
  { US"Ibreve",          0x012c, -1 },
  { US"Icircumflex",     0x00ce, -1 },
  { US"Idieresis",       0x00cf, -1 },
  { US"Idotaccent",      0x0130, -1 },
  { US"Igrave",          0x00cc, -1 },
  { US"Imacron",         0x012a, -1 },
  { US"Iogonek",         0x012e, -1 },
  { US"Itilde",          0x0128, -1 },
  { US"J",               0x004a, -1 },
  { US"Jcircumflex",     0x0134, -1 },
  { US"K",               0x004b, -1 },
  { US"Kcommaaccent",    0x0136, -1 },
  { US"L",               0x004c, -1 },
  { US"Lacute",          0x0139, -1 },
  { US"Lcaron",          0x013d, -1 },
  { US"Lcommaaccent",    0x013b, -1 },
  { US"Ldot",            0x013f, -1 },
  { US"Lslash",          0x0141, -1 },
  { US"M",               0x004d, -1 },
  { US"N",               0x004e, -1 },
  { US"NBspace",         0x00a0, -1 },
  { US"Nacute",          0x0143, -1 },
  { US"Ncaron",          0x0147, -1 },
  { US"Ncommaaccent",    0x0145, -1 },
  { US"Ntilde",          0x00d1, -1 },
  { US"O",               0x004f, -1 },
  { US"OE",              0x0152, -1 },
  { US"Oacute",          0x00d3, -1 },
  { US"Obreve",          0x014e, -1 },
  { US"Ocircumflex",     0x00d4, -1 },
  { US"Odieresis",       0x00d6, -1 },
  { US"Ograve",          0x00d2, -1 },
  { US"Ohungarumlaut",   0x0150, -1 },
  { US"Omacron",         0x014c, -1 },
  { US"Oslash",          0x00d8, -1 },
  { US"Otilde",          0x00d5, -1 },
  { US"P",               0x0050, -1 },
  { US"Q",               0x0051, -1 },
  { US"R",               0x0052, -1 },
  { US"Racute",          0x0154, -1 },
  { US"Rcaron",          0x0158, -1 },
  { US"Rcommaaccent",    0x0156, -1 },
  { US"S",               0x0053, -1 },
  { US"Sacute",          0x015a, -1 },
  { US"Scaron",          0x0160, -1 },
  { US"Scedilla",        0x015e, -1 },
  { US"Scircumflex",     0x015c, -1 },
  { US"Scommaaccent",    0x0218, +2 },
  { US"T",               0x0054, -1 },
  { US"Tbar",            0x0166, -1 },
  { US"Tcaron",          0x0164, -1 },
  { US"Tcedilla",        0x0162, -1 },
  { US"Tcommaaccent",    0x021a, +3 },
  { US"Thorn",           0x00de, -1 },
  { US"U",               0x0055, -1 },
  { US"Uacute",          0x00da, -1 },
  { US"Ubreve",          0x016c, -1 },
  { US"Ucircumflex",     0x00db, -1 },
  { US"Udieresis",       0x00dc, -1 },
  { US"Ugrave",          0x00d9, -1 },
  { US"Uhungarumlaut",   0x0170, -1 },
  { US"Umacron",         0x016a, -1 },
  { US"Uogonek",         0x0172, -1 },
  { US"Uring",           0x016e, -1 },
  { US"Utilde",          0x0168, -1 },
  { US"V",               0x0056, -1 },
  { US"W",               0x0057, -1 },
  { US"Wcircumflex",     0x0174, -1 },
  { US"X",               0x0058, -1 },
  { US"Y",               0x0059, -1 },
  { US"Yacute",          0x00dd, -1 },
  { US"Ycircumflex",     0x0176, -1 },
  { US"Ydieresis",       0x0178, -1 },
  { US"Z",               0x005a, -1 },
  { US"Zacute",          0x0179, -1 },
  { US"Zcaron",          0x017d, -1 },
  { US"Zdotaccent",      0x017b, -1 },
  { US"a",               0x0061, -1 },
  { US"aacute",          0x00e1, -1 },
  { US"abreve",          0x0103, -1 },
  { US"acircumflex",     0x00e2, -1 },
  { US"acute",           0x00b4, -1 },
  { US"adieresis",       0x00e4, -1 },
  { US"ae",              0x00e6, -1 },
  { US"agrave",          0x00e0, -1 },
  { US"amacron",         0x0101, -1 },
  { US"ampersand",       0x0026, -1 },
  { US"aogonek",         0x0105, -1 },
  { US"aring",           0x00e5, -1 },
  { US"asciicircum",     0x005e, -1 },
  { US"asciitilde",      0x007e, -1 },
  { US"asterisk",        0x002a, -1 },
  { US"at",              0x0040, -1 },
  { US"atilde",          0x00e3, -1 },
  { US"b",               0x0062, -1 },
  { US"backslash",       0x005c, -1 },
  { US"bar",             0x007c, -1 },
  { US"braceleft",       0x007b, -1 },
  { US"braceright",      0x007d, -1 },
  { US"bracketleft",     0x005b, -1 },
  { US"bracketright",    0x005d, -1 },
  { US"breve",           0x0306, +4 },
  { US"brokenbar",       0x00a6, -1 },
  { US"bullet",          0x00b7, -1 },
  { US"c",               0x0063, -1 },
  { US"cacute",          0x0107, -1 },
  { US"caron",           0x030c, +5 },
  { US"ccaron",          0x010d, -1 },
  { US"ccedilla",        0x00e7, -1 },
  { US"ccircumflex",     0x0109, -1 },
  { US"cdotaccent",      0x010b, -1 },
  { US"cedilla",         0x00b8, -1 },
  { US"cent",            0x00a2, -1 },
  { US"circumflex",      0x0302, +6 },
  { US"colon",           0x003a, -1 },
  { US"comma",           0x002c, -1 },
  { US"commaaccent",     0x0326, +7 },
  { US"copyright",       0x00a9, -1 },
  { US"currency",        0x00a4, -1 },
  { US"d",               0x0064, -1 },
  { US"dagger",          0x2020, +8 },
  { US"daggerdbl",       0x2021, +9 },
  { US"dcaron",          0x010f, -1 },
  { US"dcroat",          0x0111, -1 },
  { US"degree",          0x00b0, -1 },
  { US"dieresis",        0x00a8, -1 },
  { US"divide",          0x00f7, -1 },
  { US"dollar",          0x0024, -1 },
  { US"dotaccent",       0x0307, 10 },
  { US"dotlessi",        0x0131, -1 },
  { US"e",               0x0065, -1 },
  { US"eacute",          0x00e9, -1 },
  { US"ebreve",          0x0115, -1 },
  { US"ecaron",          0x011b, -1 },
  { US"ecircumflex",     0x00ea, -1 },
  { US"edieresis",       0x00eb, -1 },
  { US"edotaccent",      0x0117, -1 },
  { US"egrave",          0x00e8, -1 },
  { US"eight",           0x0038, -1 },
  { US"ellipsis",        0x2026, 11 },
  { US"emacron",         0x0113, -1 },
  { US"emdash",          0x2014, 12 },
  { US"endash",          0x2013, 13 },
  { US"eng",             0x014b, -1 },
  { US"eogonek",         0x0119, -1 },
  { US"equal",           0x003d, -1 },
  { US"eth",             0x00f0, -1 },
  { US"exclam",          0x0021, -1 },
  { US"exclamdown",      0x00a1, -1 },
  { US"f",               0x0066, -1 },
  { US"fi",              0xfb01, 14 },
  { US"five",            0x0035, -1 },
  { US"fl",              0xfb02, 15 },
  { US"florin",          0x0192, 16 },
  { US"four",            0x0034, -1 },
  { US"fraction",        0x2044, 17 },
  { US"g",               0x0067, -1 },
  { US"gbreve",          0x011f, -1 },
  { US"gcircumflex",     0x011d, -1 },
  { US"gcommaaccent",    0x0123, -1 },
  { US"gdotaccent",      0x0121, -1 },
  { US"germandbls",      0x00df, -1 },
  { US"grave",           0x0060, -1 },
  { US"greater",         0x003e, -1 },
  { US"greaterequal",    0x2265, 18 },
  { US"guillemotleft",   0x00ab, -1 },
  { US"guillemotright",  0x00bb, -1 },
  { US"guilsinglleft",   0x2039, 19 },
  { US"guilsinglright",  0x203a, 20 },
  { US"h",               0x0068, -1 },
  { US"hbar",            0x0127, -1 },
  { US"hcircumflex",     0x0125, -1 },
  { US"hungarumlaut",    0x030b, 21 },
  /* 002d is "hyphen-minus"; Unicode also has separate codes for hyphen and
  for minus. We use the latter below. */
  { US"hyphen",          0x002d, -1 },
  { US"i",               0x0069, -1 },
  { US"iacute",          0x00ed, -1 },
  { US"ibreve",          0x012d, -1 },
  { US"icircumflex",     0x00ee, -1 },
  { US"idieresis",       0x00ef, -1 },
  { US"igrave",          0x00ec, -1 },
  { US"ij",              0x0133, -1 },
  { US"imacron",         0x012b, -1 },
  { US"infinity",        0x221e, 43 },
  { US"iogonek",         0x012f, -1 },
  { US"itilde",          0x0129, -1 },
  { US"j",               0x006a, -1 },
  { US"jcircumflex",     0x0135, -1 },
  { US"k",               0x006b, -1 },
  { US"kcommaaccent",    0x0137, -1 },
  { US"kgreenlandic",    0x0138, -1 },
  { US"l",               0x006c, -1 },
  { US"lacute",          0x013a, -1 },
  { US"lcaron",          0x013e, -1 },
  { US"lcommaaccent",    0x013c, -1 },
  { US"ldot",            0x0140, -1 },
  { US"less",            0x003c, -1 },
  { US"lessequal",       0x2264, 22 },
  { US"logicalnot",      0x00ac, -1 },
  { US"longs",           0x017f, -1 },
  { US"lozenge",         0x25ca, 23 },
  { US"lslash",          0x0142, -1 },
  { US"m",               0x006d, -1 },
  { US"macron",          0x00af, -1 },
  { US"minus",           0x2212, 24 },
  { US"mu",              0x00b5, -1 },
  { US"multiply",        0x00d7, -1 },
  { US"n",               0x006e, -1 },
  { US"nacute",          0x0144, -1 },
  { US"napostrophe",     0x0149, -1 },
  { US"ncaron",          0x0148, -1 },
  { US"ncommaaccent",    0x0146, -1 },
  { US"nine",            0x0039, -1 },
  { US"notequal",        0x2260, 25 },
  { US"ntilde",          0x00f1, -1 },
  { US"numbersign",      0x0023, -1 },
  { US"o",               0x006f, -1 },
  { US"oacute",          0x00f3, -1 },
  { US"obreve",          0x014f, -1 },
  { US"ocircumflex",     0x00f4, -1 },
  { US"odieresis",       0x00f6, -1 },
  { US"oe",              0x0153, -1 },
  { US"ogonek",          0x0328, 26 },
  { US"ograve",          0x00f2, -1 },
  { US"ohungarumlaut",   0x0151, -1 },
  { US"omacron",         0x014d, -1 },
  { US"one",             0x0031, -1 },
  { US"onehalf",         0x00bd, -1 },
  { US"onequarter",      0x00bc, -1 },
  { US"onesuperior",     0x00b9, -1 },
  { US"ordfeminine",     0x00aa, -1 },
  { US"ordmasculine",    0x00ba, -1 },
  { US"oslash",          0x00f8, -1 },
  { US"otilde",          0x00f5, -1 },
  { US"p",               0x0070, -1 },
  { US"paragraph",       0x00b6, -1 },
  { US"parenleft",       0x0028, -1 },
  { US"parenright",      0x0029, -1 },
  { US"partialdiff",     0x2202, 27 },
  { US"percent",         0x0025, -1 },
  { US"period",          0x002e, -1 },
  { US"periodcentered",  0x2027, 28 },
  { US"perthousand",     0x2031, 29 },
  { US"plus",            0x002b, -1 },
  { US"plusminus",       0x00b1, -1 },
  { US"q",               0x0071, -1 },
  { US"question",        0x003f, -1 },
  { US"questiondown",    0x00bf, -1 },
  { US"quotedbl",        0x0022, -1 },
  { US"quotedblbase",    0x201e, 30 },
  { US"quotedblleft",    0x201c, 31 },
  { US"quotedblright",   0x201d, 32 },
  { US"quoteleft",       0x2018, 33 },
  { US"quoteright",      0x2019, 34 },
  { US"quotesinglbase",  0x201a, 35 },
  { US"quotesingle",     0x0027, -1 },
  { US"r",               0x0072, -1 },
  { US"racute",          0x0155, -1 },
  { US"radical",         0x221a, 36 },
  { US"rcaron",          0x0159, -1 },
  { US"rcommaaccent",    0x0157, -1 },
  { US"registered",      0x00ae, -1 },
  { US"ring",            0x030a, 37 },
  { US"s",               0x0073, -1 },
  { US"sacute",          0x015b, -1 },
  { US"scaron",          0x0161, -1 },
  { US"scedilla",        0x015f, -1 },
  { US"scircumflex",     0x015d, -1 },
  { US"scommaaccent",    0x0219, 38 },
  { US"section",         0x00a7, -1 },
  { US"semicolon",       0x003b, -1 },
  { US"seven",           0x0037, -1 },
  { US"six",             0x0036, -1 },
  { US"slash",           0x002f, -1 },
  { US"space",           0x0020, -1 },
  { US"sterling",        0x00a3, -1 },
  { US"summation",       0x2211, 39 },
  { US"t",               0x0074, -1 },
  { US"tbar",            0x0167, -1 },
  { US"tcaron",          0x0165, -1 },
  { US"tcedilla",        0x0163, -1 },
  { US"tcommaaccent",    0x021b, 40 },
  { US"thorn",           0x00fe, -1 },
  { US"three",           0x0033, -1 },
  { US"threequarters",   0x00be, -1 },
  { US"threesuperior",   0x00b3, -1 },
  { US"tilde",           0x0303, 41 },
  { US"trademark",       0x2122, 42 },
  { US"two",             0x0032, -1 },
  { US"twosuperior",     0x00b2, -1 },
  { US"u",               0x0075, -1 },
  { US"uacute",          0x00fa, -1 },
  { US"ubreve",          0x016d, -1 },
  { US"ucircumflex",     0x00fb, -1 },
  { US"udieresis",       0x00fc, -1 },
  { US"ugrave",          0x00f9, -1 },
  { US"uhungarumlaut",   0x0171, -1 },
  { US"umacron",         0x016b, -1 },
  { US"underscore",      0x005f, -1 },
  { US"uogonek",         0x0173, -1 },
  { US"uring",           0x016f, -1 },
  { US"utilde",          0x0169, -1 },
  { US"v",               0x0076, -1 },
  { US"w",               0x0077, -1 },
  { US"wcircumflex",     0x0175, -1 },
  { US"x",               0x0078, -1 },
  { US"y",               0x0079, -1 },
  { US"yacute",          0x00fd, -1 },
  { US"ycircumflex",     0x0177, -1 },
  { US"ydieresis",       0x00ff, -1 },
  { US"yen",             0x00a5, -1 },
  { US"z",               0x007a, -1 },
  { US"zacute",          0x017a, -1 },
  { US"zcaron",          0x017e, -1 },
  { US"zdotaccent",      0x017c, -1 },
  { US"zero",            0x0030, -1 }
};

int an2ucount = sizeof(an2ulist)/sizeof(an2uencod);


/* This table translates from Unicode code points into characters in the
special fonts, currently Symbol and Dingbats. Some of these characters do
appear in the Standard Encoding as well, but they are not present in every
Adobe font, so are sometimes used from here. Characters in the Symbol font that
are generally available (such as exclam, digits, plus, period, etc) are not
included here, as they would never be used, so it would bloat the table
unnecessarily. */

u2sencod u2slist[] = {
  { 0x0391, SF_SYMB,  65 },       /* Alpha */
  { 0x0392, SF_SYMB,  66 },       /* Beta */
  { 0x0393, SF_SYMB,  71 },       /* Gamma */
  { 0x0394, SF_SYMB,  68 },       /* Delta */
  { 0x0395, SF_SYMB,  69 },       /* Epsilon */
  { 0x0396, SF_SYMB,  90 },       /* Zeta */
  { 0x0397, SF_SYMB,  72 },       /* Eta */
  { 0x0398, SF_SYMB,  81 },       /* Theta */
  { 0x0399, SF_SYMB,  73 },       /* Iota */
  { 0x039a, SF_SYMB,  75 },       /* Kappa */
  { 0x039b, SF_SYMB,  76 },       /* Lambda */
  { 0x039c, SF_SYMB,  77 },       /* Mu */
  { 0x039d, SF_SYMB,  78 },       /* Nu */
  { 0x039e, SF_SYMB,  88 },       /* Xi */
  { 0x039f, SF_SYMB,  79 },       /* Omicron */
  { 0x03a0, SF_SYMB,  80 },       /* Pi */
  { 0x03a1, SF_SYMB,  82 },       /* Rho */
  { 0x03a3, SF_SYMB,  83 },       /* Sigma */
  { 0x03a4, SF_SYMB,  84 },       /* Tau */
  { 0x03a5, SF_SYMB,  85 },       /* Upsilon */
  { 0x03a6, SF_SYMB,  70 },       /* Phi */
  { 0x03a7, SF_SYMB,  67 },       /* Chi */
  { 0x03a8, SF_SYMB,  89 },       /* Psi */
  { 0x03a9, SF_SYMB,  87 },       /* Omega */
  { 0x03b1, SF_SYMB,  97 },       /* alpha */
  { 0x03b2, SF_SYMB,  98 },       /* beta */
  { 0x03b3, SF_SYMB, 103 },       /* gamma */
  { 0x03b4, SF_SYMB, 100 },       /* delta */
  { 0x03b5, SF_SYMB, 101 },       /* epsilon */
  { 0x03b6, SF_SYMB, 122 },       /* zeta */
  { 0x03b7, SF_SYMB, 104 },       /* eta */
  { 0x03b8, SF_SYMB, 113 },       /* theta */
  { 0x03b9, SF_SYMB, 105 },       /* iota */
  { 0x03ba, SF_SYMB, 107 },       /* kappa */
  { 0x03bb, SF_SYMB, 108 },       /* lambda */
  { 0x03bc, SF_SYMB, 109 },       /* mu */
  { 0x03bd, SF_SYMB, 110 },       /* nu */
  { 0x03be, SF_SYMB, 120 },       /* xi */
  { 0x03bf, SF_SYMB, 111 },       /* omicron */
  { 0x03c0, SF_SYMB, 112 },       /* pi */
  { 0x03c1, SF_SYMB, 114 },       /* rho */
  { 0x03c2, SF_SYMB,  86 },       /* sigma1 */
  { 0x03c3, SF_SYMB, 115 },       /* sigma */
  { 0x03c4, SF_SYMB, 116 },       /* tau */
  { 0x03c5, SF_SYMB, 117 },       /* upsilon */
  { 0x03c6, SF_SYMB, 106 },       /* phi1 */
  { 0x03c7, SF_SYMB,  99 },       /* chi */
  { 0x03c8, SF_SYMB, 121 },       /* psi */
  { 0x03c9, SF_SYMB, 119 },       /* omega */
  { 0x03d1, SF_SYMB,  74 },       /* theta1 */
  { 0x03d2, SF_SYMB, 161 },       /* Upsilon1 */
  { 0x03d5, SF_SYMB, 102 },       /* phi */
  { 0x03d6, SF_SYMB, 118 },       /* omega1 */
  { 0x12aa, SF_SYMB, 239 },       /* braceex */
  { 0x2032, SF_SYMB, 162 },       /* minute */
  { 0x2033, SF_SYMB, 178 },       /* second */
  { 0x20ac, SF_SYMB, 160 },       /* Euro */
  { 0x2111, SF_SYMB, 193 },       /* Ifraktur */
  { 0x2118, SF_SYMB, 195 },       /* weierstrass */
  { 0x211c, SF_SYMB, 194 },       /* Rfraktur */
  { 0x2135, SF_SYMB, 192 },       /* aleph */
  { 0x2190, SF_SYMB, 172 },       /* arrowleft */
  { 0x2191, SF_SYMB, 173 },       /* arrowup */
  { 0x2192, SF_SYMB, 174 },       /* arrowright */
  { 0x2193, SF_SYMB, 175 },       /* arrowdown */
  { 0x2194, SF_SYMB, 171 },       /* arrowboth */
  { 0x21b5, SF_SYMB, 191 },       /* carriagereturn */
  { 0x21d0, SF_SYMB, 220 },       /* arrowdblleft */
  { 0x21d1, SF_SYMB, 221 },       /* arrowdblup */
  { 0x21d2, SF_SYMB, 222 },       /* arrowdblright */
  { 0x21d3, SF_SYMB, 223 },       /* arrowdbldown */
  { 0x21d4, SF_SYMB, 219 },       /* arrowdblboth */
  { 0x2200, SF_SYMB,  34 },       /* universal */
  { 0x2202, SF_SYMB, 182 },       /* partialdiff */
  { 0x2203, SF_SYMB,  36 },       /* existential */
  { 0x2205, SF_SYMB, 198 },       /* emptyset */
  { 0x2207, SF_SYMB, 209 },       /* gradient */
  { 0x2208, SF_SYMB, 206 },       /* element */
  { 0x2209, SF_SYMB, 207 },       /* notelement */
  { 0x220d, SF_SYMB,  39 },       /* suchthat */
  { 0x220f, SF_SYMB, 213 },       /* product */
  { 0x2211, SF_SYMB, 229 },       /* summation */
  { 0x2215, SF_SYMB, 164 },       /* fraction */
  { 0x221a, SF_SYMB, 214 },       /* radical */
  { 0x221d, SF_SYMB, 181 },       /* proportional */
  { 0x221e, SF_SYMB, 165 },       /* infinity */
  { 0x2220, SF_SYMB, 208 },       /* angle */
  { 0x2229, SF_SYMB, 199 },       /* intersection */
  { 0x222a, SF_SYMB, 200 },       /* union */
  { 0x222b, SF_SYMB, 242 },       /* integral */
  { 0x2234, SF_SYMB,  92 },       /* therefore */
  { 0x223c, SF_SYMB, 123 },       /* similar */
  { 0x2245, SF_SYMB,  64 },       /* congruent */
  { 0x2248, SF_SYMB, 187 },       /* approxequal */
  { 0x2260, SF_SYMB, 185 },       /* notequal */
  { 0x2261, SF_SYMB, 186 },       /* equivalence */
  { 0x2264, SF_SYMB, 163 },       /* lessequal */
  { 0x2265, SF_SYMB, 179 },       /* greaterequal */
  { 0x2282, SF_SYMB, 204 },       /* propersubset */
  { 0x2283, SF_SYMB, 201 },       /* propersuperset */
  { 0x2284, SF_SYMB, 203 },       /* notsubset */
  { 0x2286, SF_SYMB, 205 },       /* reflexsubset */
  { 0x2287, SF_SYMB, 202 },       /* reflexsuperset */
  { 0x2295, SF_SYMB, 197 },       /* circleplus */
  { 0x2297, SF_SYMB, 196 },       /* circlemultiply */
  { 0x22a5, SF_SYMB,  93 },       /* perpendicular */
  { 0x22c0, SF_SYMB, 217 },       /* logicaland */
  { 0x22c1, SF_SYMB, 218 },       /* logicalor */
  { 0x22c5, SF_SYMB, 215 },       /* dotmath */
  { 0x2320, SF_SYMB, 243 },       /* integraltp */
  { 0x2321, SF_SYMB, 245 },       /* integralbt */
  { 0x2329, SF_SYMB, 225 },       /* angleleft */
  { 0x232a, SF_SYMB, 241 },       /* angleright */
  { 0x239b, SF_SYMB, 230 },       /* parenlefttp */
  { 0x239c, SF_SYMB, 231 },       /* parenleftex */
  { 0x239d, SF_SYMB, 232 },       /* parenleftbt */
  { 0x239e, SF_SYMB, 246 },       /* parenrighttp */
  { 0x239f, SF_SYMB, 247 },       /* parenrightex */
  { 0x23a0, SF_SYMB, 248 },       /* parenrightbt */
  { 0x23a1, SF_SYMB, 233 },       /* bracketlefttp */
  { 0x23a2, SF_SYMB, 234 },       /* bracketleftex */
  { 0x23a3, SF_SYMB, 235 },       /* bracketleftbt */
  { 0x23a4, SF_SYMB, 249 },       /* bracketrighttp */
  { 0x23a5, SF_SYMB, 250 },       /* bracketrightex */
  { 0x23a6, SF_SYMB, 251 },       /* bracketrightbt */
  { 0x23a7, SF_SYMB, 236 },       /* bracelefttp */
  { 0x23a8, SF_SYMB, 237 },       /* braceleftmid */
  { 0x23a9, SF_SYMB, 238 },       /* braceleftbt */
  { 0x23ab, SF_SYMB, 252 },       /* bracerighttp */
  { 0x23ac, SF_SYMB, 253 },       /* bracerightmid */
  { 0x23ad, SF_SYMB, 254 },       /* bracerightbt */
  { 0x23ae, SF_SYMB, 244 },       /* integralex */
  { 0x25a0, SF_DBAT, 110 },       /* filled square */
  { 0x25b2, SF_DBAT, 115 },       /* filled point up triangle */
  { 0x25bc, SF_DBAT, 116 },       /* filled point down triangle */
  { 0x25c6, SF_DBAT, 117 },       /* filled diamond */
  { 0x25ca, SF_SYMB, 224 },       /* lozenge */
  { 0x25d7, SF_DBAT, 119 },       /* right half circle, filled */
  { 0x260e, SF_DBAT,  37 },       /* telephone */
  { 0x261b, SF_DBAT,  42 },       /* filled hand pointing right */
  { 0x261e, SF_DBAT,  43 },       /* unfilled hand pointing right */
  { 0x2660, SF_SYMB, 170 },       /* spade */
  { 0x2663, SF_SYMB, 167 },       /* club */
  { 0x2665, SF_SYMB, 169 },       /* heart */
  { 0x2666, SF_SYMB, 168 },       /* diamond */
  { 0x26ab, SF_DBAT, 108 },       /* filled circle */
  { 0x2701, SF_DBAT,  33 },
  { 0x2702, SF_DBAT,  34 },
  { 0x2703, SF_DBAT,  35 },
  { 0x2704, SF_DBAT,  36 },
  { 0x2706, SF_DBAT,  38 },
  { 0x2707, SF_DBAT,  39 },
  { 0x2708, SF_DBAT,  40 },
  { 0x2709, SF_DBAT,  41 },
  { 0x270c, SF_DBAT,  44 },
  { 0x270d, SF_DBAT,  45 },
  { 0x270e, SF_DBAT,  46 },
  { 0x270f, SF_DBAT,  47 },
  { 0x2710, SF_DBAT,  48 },
  { 0x2711, SF_DBAT,  49 },
  { 0x2712, SF_DBAT,  50 },
  { 0x2713, SF_DBAT,  51 },
  { 0x2714, SF_DBAT,  52 },
  { 0x2715, SF_DBAT,  53 },
  { 0x2716, SF_DBAT,  54 },
  { 0x2717, SF_DBAT,  55 },
  { 0x2718, SF_DBAT,  56 },
  { 0x2719, SF_DBAT,  57 },
  { 0x271a, SF_DBAT,  58 },
  { 0x271b, SF_DBAT,  59 },
  { 0x271c, SF_DBAT,  60 },
  { 0x271d, SF_DBAT,  61 },
  { 0x271e, SF_DBAT,  62 },
  { 0x271f, SF_DBAT,  63 },
  { 0x2720, SF_DBAT,  64 },
  { 0x2721, SF_DBAT,  65 },
  { 0x2722, SF_DBAT,  66 },
  { 0x2723, SF_DBAT,  67 },
  { 0x2724, SF_DBAT,  68 },
  { 0x2725, SF_DBAT,  69 },
  { 0x2726, SF_DBAT,  70 },
  { 0x2727, SF_DBAT,  71 },
  { 0x2729, SF_DBAT,  73 },
  { 0x272a, SF_DBAT,  74 },
  { 0x272b, SF_DBAT,  75 },
  { 0x272c, SF_DBAT,  76 },
  { 0x272d, SF_DBAT,  77 },
  { 0x272e, SF_DBAT,  78 },
  { 0x272f, SF_DBAT,  79 },
  { 0x2730, SF_DBAT,  80 },
  { 0x2731, SF_DBAT,  81 },
  { 0x2732, SF_DBAT,  82 },
  { 0x2733, SF_DBAT,  83 },
  { 0x2734, SF_DBAT,  84 },
  { 0x2735, SF_DBAT,  85 },
  { 0x2736, SF_DBAT,  86 },
  { 0x2737, SF_DBAT,  87 },
  { 0x2738, SF_DBAT,  88 },
  { 0x2739, SF_DBAT,  89 },
  { 0x273a, SF_DBAT,  90 },
  { 0x273b, SF_DBAT,  91 },
  { 0x273c, SF_DBAT,  92 },
  { 0x273d, SF_DBAT,  93 },
  { 0x273e, SF_DBAT,  94 },
  { 0x273f, SF_DBAT,  95 },
  { 0x2740, SF_DBAT,  96 },
  { 0x2741, SF_DBAT,  97 },
  { 0x2742, SF_DBAT,  98 },
  { 0x2743, SF_DBAT,  99 },
  { 0x2744, SF_DBAT, 100 },
  { 0x2745, SF_DBAT, 101 },
  { 0x2746, SF_DBAT, 102 },
  { 0x2747, SF_DBAT, 103 },
  { 0x2748, SF_DBAT, 104 },
  { 0x2749, SF_DBAT, 105 },
  { 0x274a, SF_DBAT, 106 },
  { 0x274b, SF_DBAT, 107 },
  { 0x274d, SF_DBAT, 109 },
  { 0x274f, SF_DBAT, 111 },
  { 0x2750, SF_DBAT, 112 },
  { 0x2751, SF_DBAT, 113 },
  { 0x2752, SF_DBAT, 114 },
  { 0x2756, SF_DBAT, 118 },
  { 0x2758, SF_DBAT, 120 },
  { 0x2759, SF_DBAT, 121 },
  { 0x275a, SF_DBAT, 122 },
  { 0x275b, SF_DBAT, 123 },
  { 0x275c, SF_DBAT, 124 },
  { 0x275d, SF_DBAT, 125 },
  { 0x275e, SF_DBAT, 126 },
  { 0x2761, SF_DBAT, 161 },
  { 0x2762, SF_DBAT, 162 },
  { 0x2763, SF_DBAT, 163 },
  { 0x2764, SF_DBAT, 164 },
  { 0x2765, SF_DBAT, 165 },
  { 0x2766, SF_DBAT, 166 },
  { 0x2767, SF_DBAT, 167 },
  { 0x2776, SF_DBAT, 182 },
  { 0x2777, SF_DBAT, 183 },
  { 0x2778, SF_DBAT, 184 },
  { 0x2779, SF_DBAT, 185 },
  { 0x277a, SF_DBAT, 186 },
  { 0x277b, SF_DBAT, 187 },
  { 0x277c, SF_DBAT, 188 },
  { 0x277d, SF_DBAT, 189 },
  { 0x277e, SF_DBAT, 190 },
  { 0x277f, SF_DBAT, 191 },
  { 0x2780, SF_DBAT, 192 },
  { 0x2781, SF_DBAT, 193 },
  { 0x2782, SF_DBAT, 194 },
  { 0x2783, SF_DBAT, 195 },
  { 0x2784, SF_DBAT, 196 },
  { 0x2785, SF_DBAT, 197 },
  { 0x2786, SF_DBAT, 198 },
  { 0x2787, SF_DBAT, 199 },
  { 0x2788, SF_DBAT, 200 },
  { 0x2789, SF_DBAT, 201 },
  { 0x278a, SF_DBAT, 202 },
  { 0x278b, SF_DBAT, 203 },
  { 0x278c, SF_DBAT, 204 },
  { 0x278d, SF_DBAT, 205 },
  { 0x278e, SF_DBAT, 206 },
  { 0x278f, SF_DBAT, 207 },
  { 0x2790, SF_DBAT, 208 },
  { 0x2791, SF_DBAT, 209 },
  { 0x2792, SF_DBAT, 210 },
  { 0x2793, SF_DBAT, 211 },
  { 0x2794, SF_DBAT, 212 },
  { 0x2798, SF_DBAT, 216 },
  { 0x2799, SF_DBAT, 217 },
  { 0x279a, SF_DBAT, 218 },
  { 0x279b, SF_DBAT, 219 },
  { 0x279c, SF_DBAT, 220 },
  { 0x279d, SF_DBAT, 221 },
  { 0x279e, SF_DBAT, 222 },
  { 0x279f, SF_DBAT, 223 },
  { 0x27a0, SF_DBAT, 224 },
  { 0x27a1, SF_DBAT, 225 },
  { 0x27a2, SF_DBAT, 226 },
  { 0x27a3, SF_DBAT, 227 },
  { 0x27a4, SF_DBAT, 228 },
  { 0x27a5, SF_DBAT, 229 },
  { 0x27a6, SF_DBAT, 230 },
  { 0x27a7, SF_DBAT, 231 },
  { 0x27a8, SF_DBAT, 232 },
  { 0x27a9, SF_DBAT, 233 },
  { 0x27aa, SF_DBAT, 234 },
  { 0x27ab, SF_DBAT, 235 },
  { 0x27ac, SF_DBAT, 236 },
  { 0x27ad, SF_DBAT, 237 },
  { 0x27ae, SF_DBAT, 238 },
  { 0x27af, SF_DBAT, 239 },
  { 0x27b1, SF_DBAT, 241 },
  { 0x27b2, SF_DBAT, 242 },
  { 0x27b3, SF_DBAT, 243 },
  { 0x27b4, SF_DBAT, 244 },
  { 0x27b5, SF_DBAT, 245 },
  { 0x27b6, SF_DBAT, 246 },
  { 0x27b7, SF_DBAT, 247 },
  { 0x27b8, SF_DBAT, 248 },
  { 0x27b9, SF_DBAT, 249 },
  { 0x27ba, SF_DBAT, 250 },
  { 0x27bb, SF_DBAT, 251 },
  { 0x27bc, SF_DBAT, 252 },
  { 0x27bd, SF_DBAT, 253 },
  { 0x27be, SF_DBAT, 254 }
};

int u2scount = sizeof(u2slist)/sizeof(u2sencod);

/* These characters in the Symbol font are omitted because they are also in
the standard encoding for all other Adobe fonts, and so would never be used
from the Symbol font. These are the characters I've noticed. There may be some
that were overlooked...

ampersand
asteriskmath
bar
braceleft
braceright
bracketleft
bracketright
bullet
colon
comma
degree
divide
eight
ellipsis
equal
exclam
five
four
greater
less
logicalnot
minus
multiply
nine
numbersign
one
parenleft
parenright
percent
period
plus
plusminus
question
semicolon
seven
six
slash
space
three
two
underscore
zero

These characters in the Symbol font are omitted because there are no Unicode
equivalents (that I have yet found :-).

arrowhorizex
arrowvertex
florin
radicalex

These are just typographic variations:

copyrightsans
copyrightserif
registersans
registerserif
trademarksans
trademarkserif
*/

/* End of datatables.c */
