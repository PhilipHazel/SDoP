/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains functions for reading a source file and parsing it into
a sequence of chained item blocks. Chapters and sections are numbered as we
encounter them. When an element has an id parameter, we add the id to a tree
for later referencing. */


#include "sdop.h"


/*************************************************
*             Static variables                   *
*************************************************/

static int  chapter_number = 0;
static int  section_number = 0;
static int  subsection_number = 0;
static int  appendix_number = 0;

static int  inchapter = 0;
static BOOL inliterallayout = FALSE;
static BOOL inpreface = FALSE;
static int  insection = 0;
static int  insubsection = 0;
static int  inappendix = 0;

static FILE *infile;
static uschar *linebuffer;



/*************************************************
*           Check for supported element          *
*************************************************/

/* This function is called when a new element is encountered. We check to see
whether it and its attributes are supported. If not, remember what is not
supported for outputting at the end of processing.

Argument:   pointer to the item
Returns:    nothing
*/

static void
check_supported(item *new)
{
int c = -1;
int bot = 0;
int top = supported_elements_count;
int mid;
tree_node *tn;
paramstr *param;

while (top > bot)
  {
  mid = (top + bot)/2;
  c = Ustrcmp(new->name, supported_elements[mid].name);
  if (c == 0) break;
  if (c < 0) top = mid; else bot = mid + 1;
  }

/* Element is not recognized; insert in tree if not already there. */

if (c != 0)
  {
  tn = tree_search(unknown_element_tree, new->name);
  if (tn == NULL)
    {
    tn = misc_malloc(sizeof(tree_node) + Ustrlen(new->name));
    Ustrcpy(tn->name, new->name);
    (void)tree_insertnode(&unknown_element_tree, tn);
    }
  return;
  }

/* Element is recognized, check its attributes. Those that start with "#" are
invented internal ones. */

for (param = new->p.param; param != NULL; param = param->next)
  {
  uschar buffer[256];
  uschar **aptr;
  if (param->name[0] == '#') continue;

  /* Check common attributes - check their values */

  if (Ustrcmp(param->name, "revisionflag") == 0)
    {
    if (Ustrcmp(param->value, "changed") == 0) continue;
    (void)sprintf(CS buffer, "+%s=%s:%s", param->name, param->value,
      new->name);
    }

  /* See if this attribute is listed as supported unless the first item
  in the list is "*", which means the element itself is ignored. */

  else
    {
    aptr = supported_elements[mid].attrs;
    if (aptr != NULL)
      {
      if (Ustrcmp(*aptr, "*") == 0) continue;
      for (; *aptr != NULL; aptr++)
        if (Ustrcmp(*aptr, param->name) == 0) break;
      if (*aptr != NULL) continue;
      }
    (void)sprintf(CS buffer, "+%s:%s", param->name, new->name);
    }

  /* Add to unknown tree */

  tn = tree_search(unknown_element_tree, buffer);
  if (tn == NULL)
    {
    tn = misc_malloc(sizeof(tree_node) + Ustrlen(buffer));
    Ustrcpy(tn->name, buffer);
    (void)tree_insertnode(&unknown_element_tree, tn);
    }
  }
}



/*************************************************
*            Handle some actual text             *
*************************************************/

/* This function is called when a character that is not inside an element is
encountered. Copy the text until we hit '<' or end of line. In CDATA text we
don't check for '<' until we have passed ']]>'.

Argument:
  p          current data pointer
  iscdata    TRUE if reading CDATA; updated if end of section reached
  pinflags   used for flagging index reference lines

Returns:     updated data pointer
*/

static uschar *
read_text(uschar *p, BOOL *iscdata, int pinflags)
{
int len;
int extra = 0;
uschar *pp = p;
uschar *temp = NULL;

/* If we are handling CDATA, search till end of line or ]]>. If there are any
ampersands in the string, we have to convert them into &amp; because there is
no memory that this data is literal, and it will be scanned later for entities.
*/

if (*iscdata)
  {
  int ampcount = 0;
  while (*p != 0 && Ustrncmp(p, "]]>", 3) != 0)
    if (*p++ == '&') ampcount++;
  len = p - pp;

  if (*p != 0)
    {
    *iscdata = FALSE;
    extra = 3;
    }

  if (ampcount != 0)
    {
    uschar *t;
    len += 4*ampcount;
    t = temp = misc_malloc(len);

    while (pp < p)
      {
      *t = *pp++;
      if (*t++ == '&')
        {
        (void)memcpy(t, "amp;", 4);
        t += 4;
        }
      }

    pp = temp;
    }
  }

/* Otherwise, search till end of line or next element. */

else
  {
  while (*p != 0 && *p != '<') p++;
  len = p - pp;
  }

/* If the previous item is a data item, tack this text onto it. */

if (Ustrcmp(read_addto->name, "#PCDATA") == 0)
  {
  textblock *tb = read_addto->p.txtblk;
  textblock *tbnew = misc_malloc(sizeof(textblock) + tb->length + len + 1);
  tbnew->next = NULL;
  tbnew->vfont = NULL;
  tbnew->pin_flags = tb->pin_flags | pinflags;
  tbnew->colour = 0;

  (void)memcpy(tbnew->string, tb->string, tb->length);
  (void)memcpy(tbnew->string + tb->length, pp, len);
  tbnew->length = tb->length + len;
  tbnew->string[tbnew->length] = 0;

  read_addto->p.txtblk = tbnew;
  misc_free(tb, sizeof(textblock) + tb->length);
  }

/* Otherwise we have to make a new data item. The item's name is #PCDATA;
it points to a textblock item. */

else
  {
  item *new;
  textblock *tbnew;

  tbnew = misc_malloc(sizeof(textblock) + len + 1);
  tbnew->next = NULL;
  tbnew->vfont = NULL;
  tbnew->pin_flags = pinflags;
  tbnew->colour = 0;

  (void)memcpy(tbnew->string, pp, len);
  tbnew->length = len;
  tbnew->string[tbnew->length] = 0;

  new = misc_malloc(sizeof(item));
  new->next = read_addto->next;
  new->prev = read_addto;
  new->partner = new;
  new->linenumber = read_linenumber;
  new->flags = 0;
  Ustrcpy(new->name, US"#PCDATA");
  new->p.txtblk = tbnew;

  read_addto->next = new;
  read_addto = new;
  }

if (temp != NULL) misc_free(temp, len);
return p + extra;
}



/*************************************************
*             Handle DocBook markup item         *
*************************************************/

/* This function is called when a '<' character is encountered in a text line.
It creates an element item with a chain of parameter settings. If the element
ends with /> the partner pointer is set to point to itself. Otherwise, we push
the element onto the stack so that it can get matched up when the terminator is
encountered later.

This function keeps track of chapters and sections, and creates attributes for
them that contain their numbers. These attributes are called "#number".

This function also keeps track of <literallayout> elements so that whitespace
at the start of lines can be discarded outside of them.

Although mainly used within this module, when reading files, this function is
also called when constructing the TOC and indexes, to handle constructed text
strings.

Arguments:
  p            pointer in input line, at initial '<'
  nest_stack   the nesting stack
  nest_ptrptr  pointer to the stack pointer

Returns:       updated pointer, past the terminating '>'
               updates the stack pointer
*/

uschar *
read_element(uschar *p, item **nest_stack, int *nest_ptrptr)
{
BOOL ender = FALSE;
BOOL procinst = FALSE;
int i = 0;
int nest_stackptr = *nest_ptrptr;
int elementstartline = read_linenumber;
uschar name[DBNAMESIZE];
uschar *pp = name;
item *new;

/* Handle special kinds of markup:

  <?.....?> is a processing instruction - ignore unless for sdop
  <!......> is a heading (?) - currently ignored
  <!--..--> is a comment that may span multiple lines, and be nested (?)
  <![CDATA[....]]> is literal character data, possibly spanning linebreaks
*/

if (*(++p) == '?')
  {
  if (Ustrncmp(p, "?sdop", 5) != 0 || !isspace(p[5]))
    {
    for (;;)
      {
      while (*(++p) != 0 && *p != '>');
      if (*p == '>')
        {
        p++;
        return p;
        }
      p = Ufgets(linebuffer, LINEBUFSIZE, infile);
      read_linenumber++;
      if (p == NULL) (void)error(89, elementstartline);  /* Hard */
      }
    }

  /* We have a processing instruction for sdop. Arrange to set this up
  as an item whose name is "?sdop", with appropriate attributes. We can do this
  by setting the leading '?' and then falling through. */

  procinst = TRUE;
  *pp++ = '?';
  i++;
  p++;
  }

/* Handle CDATA, headings, and comments */

else if (*p == '!')
  {
  int nestcount;

  if (Ustrncmp(p, "![CDATA[", 8) == 0)
    {
    BOOL incdata = TRUE;
    p += 8;
    for (;;)
      {
      uschar *ppp;
      p = read_text(p, &incdata, 0);
      if (!incdata)break;
      ppp = Ufgets(linebuffer, LINEBUFSIZE, infile);
      read_linenumber++;
      if (ppp == NULL) (void)error(88, elementstartline);  /* Hard */
        else p = ppp;
      }
    return p;
    }

  /* If not a comment, just skip to the closing '>' */

  if (Ustrncmp(p, "!--", 3) != 0)
    {
    while (*(++p) != 0 && *p != '>');
    if (*p == '>') p++;
    return p;
    }

  /* Handle comments */

  nestcount = 1;
  p += 3;

  while (nestcount > 0)
    {
    while (*p != 0)
      {
      if (Ustrncmp(p, "-->", 3) == 0)
        {
        p += 3;
        if (--nestcount <= 0) break;
        }
      else if (Ustrncmp(p, "<!--", 4) == 0)
        {
        nestcount++;
        p += 4;
        }
      else p++;
      }

    /* Comment continues onto the next line */

    if (*p == 0 && nestcount > 0)
      {
      uschar *ppp = Ufgets(linebuffer, LINEBUFSIZE, infile);
      read_linenumber++;
      if (ppp == NULL) (void)error(18, elementstartline);  /* Hard */
        else p = ppp;
      }
    }

  return p;
  }

/* Handle "normal" markup: test for an ending tag. */

if (*p == '/') { ender = TRUE; p++; }

/* Scan for the element name */

while (isalnum(*p) || *p == '_' || *p == '-' || *p == '.')
  {
  if (i++ < DBNAMESIZE - 1) *pp++ = *p;
  p++;
  }
*pp = 0;

/* Deal with an ending tag */

if (ender)
  {
  item *partner;

  if (Ustrcmp(name, "chapter") == 0)
    inchapter = insection = insubsection = 0;

  else if (Ustrcmp(name, "preface") == 0)
    inpreface = FALSE;

  else if (ISSECT(name))
    {
    if (insubsection > 0) insubsection = 0;
      else insection = insubsection = 0;
    }

  else if (Ustrcmp(name, "appendix") == 0)
    inappendix = insection = insubsection = 0;

  else if (Ustrcmp(name, "literallayout") == 0)
    inliterallayout = FALSE;

  if (*p != '>')
    {
    (void)error(2, name);
    while (*p != 0 && *p != '>') p++;
    }

  else
    {
    p++;
    if (nest_stackptr <= 0)
      {
      (void)error(3, name);
      }
    else if (Ustrcmp(nest_stack[nest_stackptr-1]->name, name) != 0)
      {
      (void)error(85, name, nest_stack[nest_stackptr-1]->name);
      }
    else
      {
      partner = nest_stack[--nest_stackptr];
      new = misc_malloc(sizeof(item));
      new->prev = read_addto;
      new->next = read_addto->next;
      if (new->next != NULL) new->next->prev = new;
      new->linenumber = read_linenumber;
      new->flags = 0;
      Ustrcpy(new->name, "/");
      new->p.param = NULL;
      read_addto->next = new;
      read_addto = new;

      new->partner = partner;
      partner->partner = new;
      }
    }
  }

/* Deal with a starting tag */

else
  {
  BOOL ended = FALSE;
  BOOL numbered = FALSE;
  tree_node *tn = NULL;
  paramstr *param = NULL;
  paramstr *lastparam = NULL;
  paramstr *newparam;

  if (Ustrcmp(name, "preface") == 0)
    inpreface = TRUE;

  else if (Ustrcmp(name, "chapter") == 0)
    {
    inchapter = ++chapter_number;
    insection = insubsection = 0;
    section_number = subsection_number = 0;
    numbered = TRUE;
    }

  else if (Ustrcmp(name, "appendix") == 0)
    {
    inappendix = ++appendix_number;
    insection = insubsection = 0;
    section_number = subsection_number = 0;
    numbered = TRUE;
    }

  else if (ISSECT(name) && !inpreface)
    {
    if (insection > 0)
      {
      insubsection = ++subsection_number;
      }
    else
      {
      insection = ++section_number;
      insubsection = 0;
      subsection_number = 0;
      }
    numbered = TRUE;
    }

  else if (Ustrcmp(name, "literallayout") == 0)
    inliterallayout = TRUE;

  if (numbered)
    {
    param = lastparam = misc_malloc(sizeof(paramstr) + 16);
    param->next = NULL;
    param->seen = TRUE;
    Ustrcpy(param->name, "#number");
    pp = param->value;
    if (inchapter > 0) pp += sprintf(CS pp, "%d", inchapter);
    if (inappendix > 0) pp += sprintf(CS pp, "%c", 'A' + inappendix - 1);
    if (insection > 0) pp += sprintf(CS pp, "%s%d",
      (pp == param->value)? "" : ".", insection);
    if (insubsection > 0) pp += sprintf(CS pp, "%s%d",
      (pp == param->value)? "" : ".", insubsection);
    }

  /* Now read any attributes that are set in the element. This may continue
  onto more than one line. */

  while (isspace(*p)) p++;
  for (;;)
    {
    int quote, dlen;
    uschar attname[DBNAMESIZE];

    /* Handle line continuations */

    while (*p == 0)
      {
      uschar *pnew = Ufgets(linebuffer, LINEBUFSIZE, infile);
      read_linenumber++;
      if (pnew == NULL)
        {
        (void)error(24, elementstartline);
        break;
        }
      else p = pnew;
      while (isspace(*p)) p++;
      }

    /* Test for end of the element */

    if (*p == '>' || *p == '/' || *p == '?')  break;

    /* Now read the name of the attribute */

    pp = attname;
    i = 0;
    while (isalnum(*p) || *p == '-' || *p == '_' || *p == '.')
      {
      if (i++ < DBNAMESIZE - 1) *pp++ = *p;
      p++;
      }
    *pp = 0;

    while (isspace(*p)) p++;
    if (*p != '=') { (void)error(6, attname); break; }
    while (isspace(*(++p)));
    if (*p != '"' && *p != '\'') { (void)error(7, attname); break; }
    quote = *p++;
    pp = p;

    while (*p != 0 && *p != quote) p++;
    if (*p != quote) { (void)error(8, quote, attname, quote); break; }

    dlen = p - pp;
    newparam = misc_malloc(sizeof(paramstr) + dlen);
    newparam->next = NULL;
    newparam->seen = FALSE;
    Ustrcpy(newparam->name, attname);
    Ustrncpy(newparam->value, pp, dlen);
    newparam->value[dlen] = 0;

    if (param == NULL) param = newparam;
      else lastparam->next = newparam;
    lastparam = newparam;
    while (isspace(*(++p)));

    /* If the attribute is "id", add it to the id tree */

    if (Ustrcmp(attname, "id") == 0)
      {
      tn = misc_malloc(sizeof(tree_node) + dlen);
      Ustrcpy(tn->name, newparam->value);
      if (!tree_insertnode(&id_tree, tn)) (void)error(11, tn->name);
      }
    }

  if (*p == '/' || (procinst && *p == '?'))
    {
    ended = TRUE;
    if (*(++p) != '>') (void)error(9, name);
    }

  /* Skip to end of element (in case jumped out after an error) */

  while (*p != 0 && *p != '>') p++;
  if (*p == 0) (void)error(5, name); else p++;

  /* Create the new element, crossreference it's id, note if it or any of its
  attributes are not supported, and push it onto the stack for checking its
  partner. */

  new = misc_malloc(sizeof(item));
  new->linenumber = read_linenumber;
  new->flags = 0;
  new->partner = ended? new : NULL;
  Ustrcpy(new->name, name);
  new->p.param = param;

  new->prev = read_addto;
  new->next = read_addto->next;
  if (new->next != NULL) new->next->prev = new;
  read_addto->next = new;
  read_addto = new;

  if (tn != NULL) tn->data.ptr = new;
  if (new->name[0] != '?') check_supported(new);

  if (!ended)
    {
    if (nest_stackptr >= NESTSTACKSIZE) (void)error(4);  /* Hard error */
    nest_stack[nest_stackptr++] = new;
    }

  /* Remember if this is the first <preface> element. */

  if (inpreface && preface_item_list == NULL) preface_item_list = new;
  }

/* Update stack pointer and return char pointer */

*nest_ptrptr = nest_stackptr;
return p;
}



/*************************************************
*       Read a string of elements and text       *
*************************************************/

/* This function processes a single string that may contain both elements and
text. Until we hit some genuine text, we skip white space that follows an
element or processing instruction. Otherwise it may contribute unwanted white
space to the output. This works because most elements do not contribute
directly to the output character string. However, there are some such as
<quote> and <xref> that do. The <quote> element is handled directly in
read_file2() below because it just turns into fixed special characters. The
others are identified by misc_istext_name() and treated as special cases here.

This function is also called from toc.c when constructing the TOC and from
index.c when constructing indexes.

Arguments:
  p            pointer to input line
  nest_stack   the nesting stack
  nest_ptrptr  pointer to the stack pointer
  pinflags     passed to read_text; used for flagging index references

Returns:       nothing
*/

void
read_string(uschar *p, item **nest_stack, int *nest_stackptr, int pinflags)
{
BOOL hadtext = inliterallayout;
while (*p != 0)
  {
  if (*p == '<')
    {
    if (misc_istext_elname(p+1)) hadtext = TRUE;
    p = read_element(p, nest_stack, nest_stackptr);
    if (!hadtext) while (isspace(*p)) p++;
    }
  else
    {
    BOOL dummy = FALSE;
    p = read_text(p, &dummy, pinflags);
    hadtext = TRUE;
    }
  }
}



/*************************************************
*         Read the input file into memory        *
*************************************************/

/* This function opens an input file and reads it, creating a chain of element
and text items. The basic function passes back a list of unclosed items. There
is a second function that generates an error for any unclosed items.

Arguments:
  filename       file name, or NULL for stdin
  nest_stack     stack for nesting, size must be NESTSTACKSIZE
  nest_stackptr  points to stack offset pointer

  The read_addto global variable must be set to point the existing last item of
  the list that is being created. The variable is updated.

Returns:         TRUE if OK (currently always)
*/

BOOL
read_file2(uschar *filename, item **nest_stack, int *nest_stackptr)
{
item *fn;
uschar *p;
uschar buffer1[LINEBUFSIZE];
uschar buffer2[LINEBUFSIZE];

linebuffer = buffer1;      /* For use when comments overflow lines */

if (filename == NULL)
  {
  DEBUG(D_any) debug_printf("===> Reading from stdin\n");
  infile = stdin;
  }
else
  {
  DEBUG(D_any) debug_printf("===> Reading from %s\n", filename);
  infile = Ufopen(filename, "rb");
  if (infile == NULL)
    (void)error(0, filename, "input file", strerror(errno));  /* Hard */
  }

read_filename = (filename == NULL)? US"(stdin)" : filename;
read_linenumber = 0;

/* Stick in a dummy element to hold the file name so we can distinguish
included files in error messages. */

fn = misc_malloc(sizeof(item));
fn->prev = read_addto;
fn->next = read_addto->next;
fn->linenumber = 0;
fn->flags = 0;
fn->partner = fn;
Ustrcpy(fn->name, "#FILENAME");
fn->p.string = misc_malloc(Ustrlen(read_filename) + 1);
Ustrcpy(fn->p.string, read_filename);

read_addto->next = fn;
read_addto = fn;

/* Now process the lines of the file */

while ((p = Ufgets(buffer1, sizeof(buffer1), infile)) != NULL)
  {
  uschar *pp = p + Ustrlen(p);

  /* Retain the newline on the end of the line, but remove any white space
  that precedes it. */

  while (pp > p + 1)
    {
    if (!isspace(pp[-2])) break;
    (pp--)[-2] = '\n';
    }
  *pp = 0;

  read_linenumber++;

  /* Unless we are in <literallayout>, remove leading whitespace before
  processing the line. This is an optimization just to save storing all those
  leading spaces when the input is indented. The later code would in fact work
  without it. */

  if (!inliterallayout) while (isspace(*p)) p++;

  /* The <quote> and </quote> elements get converted into quote characters, and
  so are not like other elements. We do the conversion here so that they do not
  interrupt strings of data. Otherwise, there can be problems when </quote> is
  at the end of a line (because of the way newline is handled). These elements
  are presumed not to be very common, so I've done this quite crudely. */

  while ((pp = Ustrstr(p, "<quote>")) != NULL ||
         (pp = Ustrstr(p, "</quote>")) != NULL)
    {
    int len;
    uschar *c, *pn;
    if (pp[1] == '/')
      {
      c = US"&#x201D;";
      len = 8;
      }
    else
      {
      c = US"&#x201C;";
      len = 7;
      }
    memcpy(buffer2, p, pp - p);
    pn = buffer2 + (pp - p);
    memcpy(pn, c, 8);
    Ustrcpy(pn + 8, pp + len);
    Ustrcpy(buffer1, buffer2);
    p = buffer1;
    }

  /* Now process the input line */

  read_string(p, nest_stack, nest_stackptr, 0);
  }

(void)fclose(infile);
read_linenumber = 0;
return TRUE;
}



/*************************************************
*     Read input file and check nest closed      *
*************************************************/

/* This is a wrapper function for the one above; if there are any unclosed
elements at the end, it generates an error. This function is called both for
the main input file and to read inclusions and also the head/foot/TOC
templates.

Arguments:
  filename   file name, or NULL for stdin
  item_list  the dummy item at the start of the list that's being read

Returns:     TRUE if all went well
*/

BOOL
read_file(uschar *filename, item *item_list)
{
int nest_stackptr = 0;
item *nest_stack[NESTSTACKSIZE];

read_addto = item_list;
(void)read_file2(filename, nest_stack, &nest_stackptr);

if (nest_stackptr > 0)
  {
  uschar *s = (nest_stackptr > 1)? US"s" : US"";
  (void)error(10, s);                  /* General message */
  (void)fprintf(stderr, "** Start of unclosed element%s:\n", s);
  while (nest_stackptr > 0)
    {
    item *i = nest_stack[--nest_stackptr];
    (void)fprintf(stderr, "  Line %5d: <%s>\n", i->linenumber, i->name);
    }
  return FALSE;
  }

return TRUE;
}



/*************************************************
*              Read main input file              *
*************************************************/

/* This is also a wrapper function; it is used for the main input file so that
we can output suitable debugging stuff and also note that we have the main data
in memory - this affects the wording of subsequent error messages.

Argument:    file name, or NULL for stdin
Returns:     TRUE if all went well
*/

BOOL
read_main_file(uschar *filename)
{
BOOL yield;
read_linenumber = 0;
yield = read_file(filename, main_item_list);
DEBUG(D_read) debug_print_item_list(main_item_list,
  "at end of reading main input");
read_done = TRUE;
return yield;
}



/*************************************************
*           Read included files                  *
*************************************************/

/* Called after reading an input file. It processes elements of the form <?sdop
include="<path>"?> and inserts the file's contents instead of that element.
We keep a stack of "open includes" in order to detect and diagnose recursion.

Arguments:
  i               the first item of the list to scan
  main_filename   the main file name (to avoid recursion)

Returns:          TRUE if all went well
*/

BOOL
read_includes(item *i, uschar *main_filename)
{
BOOL yield = TRUE;
BOOL included = FALSE;
incfile *open_includes = NULL;

for (; i != NULL; i = i->next)
  {
  paramstr *p;
  item *rest;
  incfile *ff;

  if (open_includes != NULL && i == open_includes->end)
    open_includes = open_includes->next;

  if (Ustrcmp(i->name, "?sdop") != 0 ||
      (p = misc_param_find(i, US"include")) == NULL)
    continue;

  if (Ustrcmp(p->value, main_filename) == 0)
    (void)error(54, p->value);   /* Hard */

  for (ff = open_includes; ff != NULL; ff = ff->next)
    {
    if (Ustrcmp(p->value, ff->name) == 0)
      (void)error(54, p->value);   /* Hard */
    }

  ff = misc_malloc(sizeof(incfile) + Ustrlen(p->value));
  ff->next = open_includes;
  ff->end = i->next;
  open_includes = ff;
  Ustrcpy(ff->name, p->value);

  included = TRUE;
  rest = i->next;
  read_linenumber = i->linenumber;   /* For error on open failure */
  yield = read_file(p->value, i);
  if (!yield) break;
  read_addto->next = rest;
  rest->prev = read_addto;
  }

if (included)
  {
  DEBUG(D_read) debug_print_item_list(main_item_list,
    "at end of reading included files");
  }
else
  {
  DEBUG(D_read) debug_printf("No included files\n");
  }

return yield;
}

/* End of read.c */
