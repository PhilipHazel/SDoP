/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains the code for doing hyphenation by reference to a
dictionary. The original implementation was part of SGCAL. */


#include "sdop.h"


/*************************************************
*              Data for hyphenation              *
*************************************************/

typedef uint32_t Key;

#define keyof(s) ((((Key)(s[0]))<<24)|(((Key)(s[1]))<<16)|(s[2]<<8)|s[3])

static struct hyindex {
  Key key;    /* four bytes as a comparable integer */
  long pos;   /* seek position */
} *hy_index = NULL;

static unsigned int hy_indexsize;     /* size of index */



/*************************************************
*            Initialize hyphenation              *
*************************************************/

/* This function reads the index to the hyphenation dictionary if one is
present. If not, no hyphenation can happen.

Arguments:  none
Returns:    nothing
*/

void
Hyphen_Init(void)
{
uschar line[20];
long hy_indexoffset;

if (main_hyphenfile == NULL) return;

/* In newer versions of gcc, with FORTIFY_SOURCE set (the default in some
environments), a warning is issued if the value of fgets() is ignored.
Casting to (void) does not suppress the warning. To get round this, we use a
fudge. */

if (Ufgets(line, 20, main_hyphenfile)) {};
hy_indexsize = atoi(CS line);
hy_index = misc_malloc(hy_indexsize * sizeof(struct hyindex));
hy_index[0].key = 0;
hy_index[0].pos = 0;

for (unsigned int i = 1; i < hy_indexsize; i++)
  {
  if (Ufgets(line, 20, main_hyphenfile)) {};  /* FORTIFY_SOURCE fudge */
  hy_index[i].key = keyof(line);
  hy_index[i].pos = atol(CS line+4);
  }

hy_indexoffset = ftell(main_hyphenfile);
for (unsigned int i = 0; i < hy_indexsize; i++)
  hy_index[i].pos += hy_indexoffset;
}



/*************************************************
*           Prepare word for hyphenation         *
*************************************************/

/* This function accepts a string which was delimited by spaces -- it removes
punctuation from either end, and also the suffix 's, so that the word can be
hyphenated. The yield is the number of non-letters removed from the start of
the word. This value must be added to the yields from Hyphen_Next() to get the
correct hyphenation points.

Argument:  points to the string, in memory that can be modified
Returns:   number of bytes removed from the start
           -1 if no letters are found
*/

int
Hyphen_Prepare(uschar *word)
{
uschar *ptr;
uschar *lptr;
int len = Ustrlen(word);
int yield, c;

/* Remove leading non-letters, quitting if reached end of string */

for (ptr = word;;)
  {
  lptr = ptr;
  GETCHARINC(c, ptr);
  if (c == 0) return -1;
  if (UCD_CATEGORY(c) == ucp_L) break;
  }

/* If any non-letters found, slide the word left */

yield = lptr - word;
if (yield > 0)
  {
  len -= yield;
  memmove(word, word + yield, len + 1);
  }

/* Remove non-letters from the end of the word. We know there is at least one
letter at the start. */

ptr = word + len;
for (;;)
  {
  lptr = ptr;
  ptr--;
  BACKCHAR(ptr);
  GETCHAR(c, ptr);
  if (UCD_CATEGORY(c) == ucp_L) break;
  }
len = lptr - word;

/* Check for the ending 's and remove it */

if (len >= 3 && word[len-1] == 's' && word[len-2] == '\'') len -= 2;

/* Terminate the modified word and return the initial byte count */

word[len] = 0;
return yield;
}



/*************************************************
*           De-plural for hyphenation            *
*************************************************/

/* This routine accepts a word and returns, in another vector, a de-pluralled
form.

Arguments:
  plural      the original word
  singlular   where to put the singular form

Returns:      0 if no letters were removed
             !0 if some letters were removed
*/

int
Hyphen_DePlural(uschar *plural, uschar *singular)
{
int plen = Ustrlen(plural);
int slen = plen;
Ustrcpy(singular, plural);

/* Test for a word longer than four characters (anything shorter
is not relevant), and for a terminating 's'. */

if (plen >= 4 && plural[plen-1] == 's')
  {
  uschar *suffix = plural + plen - 4;  /* the last four bytes */

  /* If the word ends in "es", then if the ending is "shes", "ches",
  "sses", or "oes", remove "es"; otherwise, unless the ending is
  "ices", "eses", or "ies" remove "s". */

  if (suffix[2] == 'e')
    {
    if (((suffix[0] == 's' || suffix[0] == 'c') && suffix[1] == 'h') ||
        (suffix[0] == 's' && suffix[1] == 's') ||
        (suffix[1] == 'o')) slen -= 2;

    else if ((suffix[0] != 'i' || suffix[1] != 'c') &&
             (suffix[0] != 'e' || suffix[1] != 's') &&
             (suffix[1] != 'i')) slen -= 1;
    }

  /* The word does not end in "es". Remove "s" if the word does not end in
  "ss", "as", "is", "os", "us" or "ys", or if one of those endings other than
  "ss" is preceded by another vowel. */

  else
    {
    if (strchr("saiouy", suffix[2]) == NULL ||
        (suffix[2] != 's' && strchr("aeiouy", suffix[1]) != NULL))
      slen--;
    }

  /* Set new length for the singular form */

  singular[slen] = 0;
  }

/* Return non-zero if a change has been made */

return slen != plen;
}



/*************************************************
*           Find next hyphenation                *
*************************************************/

/* Given a word and a byte offset, this procedure returns the previous
hyphenation point. If the offset is zero, the last hyphenation point in the
word is returned. A negative value is yielded if there are no further possible
hyphenations. Only words with at least five characters can be hyphenated, and
then only if a hyphenation file has been set up.

Arguments:
  word      points to the word
  p         an offset value

Returns:    offset of previous hyphenation point
            negative if there are no more
*/

int
Hyphen_Next(uschar *word, int p)
{
static uschar splitword[64];    /* This data must remain in existence */
static int  splits;             /* between calls to this function. */
uschar plainword[64];

if ((int)Ustrlen(word) < 4 || hy_index == NULL) return -1;

/* A value of zero for p indicates a call to find the last hyphenation point in
a new word. We must scan the in-store index of first four letters (bytes) to
find the place at which to start reading the hyphenation dictionary. */

if (p == 0)
  {
  uschar *pp;
  Key key = keyof(word);
  int seekpoint = 0;
  int middle;
  int bottom = 0;
  int top = hy_indexsize;

  /* Binary chop search in index */

  while (bottom < top)
    {
    Key testkey;
    middle = (top + bottom)/2;
    testkey = hy_index[middle].key;
    if (key < testkey) top = middle; else
      {
      seekpoint = middle;
      if (key == testkey) break;
      bottom = middle + 1;
      }
    }

  /* Position the hyphenation dictionary to the right place */

  fseek(main_hyphenfile, hy_index[seekpoint].pos, SEEK_SET);

  /* Read the hyphenation dictionary until the word is found, or
  a word lexically greater is found, or we hit the end of the dictionary. */

  for (;;)
    {
    int c, compare;
    int i = 0, j = 0;
    if (Ufgets(splitword, 30, main_hyphenfile) == NULL) return -1;
    splitword[Ustrlen(splitword) - 1] = 0;
    while ((c = splitword[i++]) != 0) if (c != '-') plainword[j++] = c;
    plainword[j] = 0;
    if ((compare = Ustrcmp(word, plainword)) == 0) break;
    if (compare < 0) return -1;    /* No entry found, yield -1 */
    }

  /* Entry in dictionary found; compute number of hyphens in it */

  p = Ustrlen(splitword);  /* Current pointer past the end */
  splits = 0;
  pp = splitword;
  while (*pp) if (*pp++ == '-') splits++;
  }

/* If called with p ~= 0, we require the next most previous hyphen from the
last word hyphendated. We update p by the number of splits that precede it so
as to convert it into an offset in the split word. */

else p += splits;

/* Loop to scan back for previous hyphenation point. We return the
offset less the number of hyphens that precede it -- that is, the
offset in the unhyphenated word. */

while (--p >= 0) if (splitword[p] == '-') return p - (--splits);

/* No more hyphens available for this word */

return -1;
}

/* End of hyphen.c */
