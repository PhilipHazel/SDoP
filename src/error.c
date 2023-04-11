/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: April 2023 */

/* Error handling functions */

#include "sdop.h"



/*************************************************
*             Static variables                   *
*************************************************/

static int  error_count = 0;
static int  warning_count = 0;
static BOOL suppress_warnings = FALSE;



/*************************************************
*            Texts and return codes              *
*************************************************/

typedef struct {
  char ec;
  const char *text;
} error_struct;


static error_struct error_data[] = {
/* 0-4 */
{ ec_disaster, "failed to open %s (%s): %s" },
{ ec_disaster, "malloc failed: requested %d bytes" },
{ ec_serious,  "'>' expected after '</%s'" },
{ ec_serious,  "nesting error: found '</%s>' before any other elements" },
{ ec_disaster, "elements too deeply nested" },
/* 5-9 */
{ ec_serious,  "missing '>' for element '%s'" },
{ ec_serious,  "expected '=' after '%s'" },
{ ec_serious,  "expected \" or ' after '%s='" },
{ ec_serious,  "missing terminating %c after %s=%c..." },
{ ec_serious,  "expected '>' after '/' or '?' in element '%s'" },
/* 10-14 */
{ ec_serious,  "unclosed element%s at end of file" },
{ ec_serious,  "duplicate element id '%s'" },
{ ec_failed,   "cross-reference to unknown id '%s'" },
{ ec_failed,   "cross-reference to unnumbered item '%s'" },
{ ec_serious,  "missing ';' after numeric entity '%.*s'" },
/* 15-19 */
{ ec_serious,  "missing ';' after named entity '%.*s'" },
{ ec_serious,  "unknown named entity \"%s\"%s" },
{ ec_serious,  "entity name '%.*s' is too long" },
{ ec_disaster, "unexpected EOF inside comment (started on line %d)" },
{ ec_disaster, "fontstack overflow: probable internal error" },
/* 20-24 */
{ ec_disaster, "fontstack underflow: probable internal error" },
{ ec_warning,  "unknown character '%s' found in '%s' font" },
{ ec_disaster, "unexpected end of kerning data in '%s'" },
{ ec_disaster, "cannot find font suffixes for '%s'" },
{ ec_disaster, "unexpected EOF inside element (started on input line %d)" },
/* 25-29 */
{ ec_warning,  "line overflow (%spt)" },
{ ec_warning,  "internal error: page %d is too long: new page started" },
{ ec_disaster, "error loading metric data for font \"%s\": %s%s" },
{ ec_disaster, "internal error: special auxiliary font missing" },
{ ec_disaster, "internal error: substitute character not found" },
/* 30-34 */
{ ec_warning,  "unsupported character &#x%04x found (first time only is reported)\n"
               "   Current font is \"%s\"" },
{ ec_warning,  "unknown value \"%s\" for \"%s\" parameter in processing "
                 "instruction" },
{ ec_disaster, "internal error: footnote key definition without footnote text" },
{ ec_serious,  "table contains more than one <tgroup>: not supported" },
{ ec_serious,  "table contains no <tgroup>" },
/* 35 - 39 */
{ ec_serious,  "table column widths not defined: not supported" },
{ ec_serious,  "%s expected for %s but \"%s\" found" },
{ ec_serious,  "<colspec> for column %d specified twice" },
{ ec_serious,  "invalid column width \"%s\": set to %s" },
{ ec_serious,  "no width specified for column %d: set to %s" },
/* 40 - 44 */
{ ec_serious,  "total of table column widths (%s) is greater than linewidth (%s)" },
{ ec_serious,  "too many entries in row: only %d column%s defined" },
{ ec_disaster, "internal error: missing #TDATA for table" },
{ ec_serious,  "%s encountered not in a table" },
{ ec_serious,  "syntax error in %s=\"%s\"" },
/* 45 - 49 */
{ ec_serious,  "<tgroup> is missing mandatory \"cols\" setting" },
{ ec_serious,  "<%s> is missing \"%s\" setting: not supported" },
{ ec_serious,  "column %d specified but table has only %d columns" },
{ ec_disaster, "sdop limitation: cannot handle more than %d lines in a paragraph" },
{ ec_disaster, "sdop limitation: cannot handle more than %d rows in a table" },
/* 50 - 54 */
{ ec_disaster, "sdop limitation: cannot handle more than %d text blocks in head or foot" },
{ ec_serious,  "malformed %s in %s" },
{ ec_serious,  "too many values in \"%s\" (max %d)" },
{ ec_serious,  "unexpected text: <para> missing?" },
{ ec_disaster, "file %s is recursively included" },
/* 55 - 59 */
{ ec_disaster, "too many different indexes (max %d)" },
{ ec_disaster, "ridiculously long index entry" },
{ ec_disaster, "syntax error in %s line %d at offset %d" },
{ ec_serious,  "end-of-range <indexterm> has no \"startref\" attribute" },
{ ec_serious,  "no previous <indexterm> with id=\"%s\" found" },
/* 60 - 64 */
{ ec_disaster, "<index> specified twice" },
{ ec_disaster, "<index role=\"%s\"> specified twice" },
{ ec_warning,  "cell overflow (%spt) in table entry for column %d" },
{ ec_disaster, "internal error: #PDATA missing before <index>" },
{ ec_serious,  "missing <primary> in <indexterm>" },
/* 65 - 69 */
{ ec_serious,  "internal error: paragraph split position failure" },
{ ec_serious,  "\"%s\" does not define a Unicode character" },
{ ec_serious,  "unknown numeration \"%s\"" },
{ ec_serious,  "missing \"endif\": <?sdop %s?> ignored" },
{ ec_disaster, "cannot find SDoP data file \"%s\" in %s or %s" },
/* 70 - 74 */
{ ec_disaster, "cannot find SDoP data file \"%s\" in %s" },
{ ec_serious,  "no text items in footnote" },
{ ec_disaster, "internal error: cannot find footnote from reference" },
{ ec_warning,  "more than 9 footnotes on page %d: line lengths may be wrong" },
{ ec_warning,  "failed to open %s (%s): %s" },
/* 75 - 79 */
{ ec_serious,  "malformed number \"%s\" (%s)" },
{ ec_serious,  "cannot implement scalefit - unknown image size" },
{ ec_warning,  "both \"scale\" and \"scalefit\" set: ignored \"scale\""},
{ ec_serious,  "media object is deeper than the page depth" },
{ ec_serious,  "invalid point size or leading value in \"%s\"" },
/* 80 - 84 */
{ ec_disaster, "internal error: invalid image type %s" },
{ ec_serious,  "%s page list specified twice" },
{ ec_serious,  "\"odd\" and \"even\" must be specified with -p, not -pf" },
{ ec_serious,  "format error in %s page list item \"%s\"" },
{ ec_serious,  "numbers out of order in %s page list" },
/* 85 - 89 */
{ ec_serious,  "nesting error: found '</%s>', expected '</%s'>" },
{ ec_disaster, "internal error: paragraph split end failure" },
{ ec_serious,  "unknown justify value \"%s\"" },
{ ec_disaster, "unexpected EOF inside <![CDATA[ (started on line %d)" },
{ ec_disaster, "unexpected EOF while skipping processing instruction "
               "starting on line %d" },
/* 90 - 94 */
{ ec_serious,  "cannot set font type for a font family" },
{ ec_serious,  "malformed font description" },
{ ec_serious,  "malformed paper size" },
{ ec_serious,  "malformed colour triple" },
{ ec_serious,  "rgb colour value outside the range 0.0 to 1.0" },
/* 95 - 99 */
{ ec_serious,  "%d dimensions expected in \"%s\"" },
{ ec_warning,  "<bookinfo> does not follow <book>" },
{ ec_warning,  "<articleinfo> does not follow <article>" },
{ ec_serious,  "unrecognized insert value \"%s\"" },
{ ec_serious,  "total of column widths (%s) plus table indent (%s) is greater "
               "than linewidth (%s)" },
/* 100 - 104 */
{ ec_serious,  "\"char\" setting must specify just one character" },
{ ec_serious,  "\"char\" alignment would cause underflow; used \"left\"" },
{ ec_serious,  "\"char\" alignment would cause overflow; used \"right\"" },
{ ec_warning,  "character &#x%04x; substituted for &#x%04x; in font \"%s\"" },
{ ec_serious,  "no substitute character available in font \"%s\" for "
               "unknown character &#x%04x;" },
/* 105 - 109 */
{ ec_warning,  "unrecognized SDoP processing parameter \"%s\"" },
{ ec_serious,  "subscript_down or superscript_up value is too large" },
{ ec_serious,  "\"yes\" or \"no\" expected but \"%s\" found" },
{ ec_warning,  "page_full_length=%s is too small; set to 108" },
{ ec_disaster, "error while processing %s file: %s" },
/* 110 - 114 */
{ ec_disaster, "internal error: missing <xref> while generating output" },
{ ec_disaster, "internal error: unset reference to %s while generating output" },
{ ec_disaster, "internal error: absolute page number unset for reference to "
               "%s while generating output" },
{ ec_disaster, "internal error: absolute page y position unset for reference "
               "to %s while generating output" },
{ ec_disaster, "too many nested lists" },
/* 115 - 119 */
{ ec_noerror,  "'%s' is deprecated: use '%s' instead" },
{ ec_disaster, "internal error: font for %s list has not been set up" },
{ ec_disaster, "internal error: call to atexit() failed" },
{ ec_disaster, "internal error: freeing block not on the list" }
};

#define ERROR_MAXERROR (int)(sizeof(error_data)/sizeof(error_struct))



/*************************************************
*              Error message generator           *
*************************************************/

/* This function outputs an error or warning message, and may abandon the
program if the error is sufficiently serious, or if there have been too many
less serious errors. If there are too many warnings, subsequent ones are
suppressed.

Arguments:
  n           error number
  ...         arguments to fill into message

Returns:      FALSE, because that's useful when continuing,
              but some errors do not return
*/

BOOL
error(int n, ...)
{
int ec;
va_list ap;
va_start(ap, n);

if (n > ERROR_MAXERROR)
  {
  (void)fprintf(stderr, "** Unknown error number %d\n", n);
  ec = ec_disaster;
  }
else
  {
  ec = error_data[n].ec;
  if (ec == ec_warning)
    {
    if (suppress_warnings) return FALSE;
    (void)fprintf(stderr, "** Warning: ");
    }
  else
    {
    if (ec > ec_warning) (void)fprintf(stderr, "** Error: ");
      else (void)fprintf(stderr, "** ");
    }
  (void)vfprintf(stderr, error_data[n].text, ap);
  (void)fprintf(stderr, "\n");
  }

va_end(ap);

if (read_linenumber > 0)
  {
  if (read_done)
    (void)fprintf(stderr, "   Detected in element starting in line %d of %s\n",
      read_linenumber, read_filename);
  else
    (void)fprintf(stderr, "   Detected near line %d of %s\n",
      read_linenumber, read_filename);
  }
else if (read_what != NULL)
  (void)fprintf(stderr, "   Detected while %s\n", read_what);

if (ec == ec_warning)
  {
  warning_count++;
  if (warning_count > 40)
    {
    (void)fprintf(stderr, "** Too many warnings - subsequent ones suppressed\n");
    suppress_warnings = TRUE;
    }
  }

else if (ec > ec_warning)
  {
  error_count++;
  if (error_count > 40)
    {
    (void)fprintf(stderr, "** Too many errors\n** SDoP processing abandoned\n");
    exit(EXIT_FAILURE);
    }
  }

if (ec > ec_failed)
  {
  (void)fprintf(stderr, "** SDoP processing abandoned\n");
  exit(EXIT_FAILURE);
  }

if (ec > error_max) error_max = ec;   /* Highest error code */
(void)fprintf(stderr, "\n");          /* blank before next output */
return FALSE;
}

/* End of error.c */
