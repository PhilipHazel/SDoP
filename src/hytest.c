/*************************************************
*                  Son-of-GCAL                   *
*                Cloned for sdop                 *
*************************************************/

/* Copyright (c) Philip Hazel 2020            */
/* Created by Philip Hazel, October 1989      */
/* Last modified: August 2003                 */
/* Converted for sdop: April 2005             */
/* New ucp functions: February 2006           */
/* New ucp functions again: October 2008      */

/* Free-standing program to test hyphenation functions. */

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* Include the bits of code we are going to test. */

#include "ucp.h"
#include "utf8tables.c"
#include "ucd.c"
#include "hyphen.c"


FILE *main_hyphenfile = NULL;


/************************************************
*        Dummy misc_malloc() function           *
************************************************/

void *
misc_malloc(int size)
{
return malloc(size);
}


/************************************************
*              Main Program                     *
************************************************/

int main(int argc, char **argv)
{
int arg = 1;
FILE *infile;
FILE *outfile;

if (argc - arg < 1 || Ustrcmp(argv[1], "--help") == 0)
  {
  (void)fprintf(stderr,
    "Usage: hytest <hyhpendata file> [<input>] [<output>]\n");
  exit (EXIT_FAILURE);
  }

main_hyphenfile = fopen(argv[arg++], "r");
if (main_hyphenfile == NULL)
  {
  (void)fprintf(stderr, "*** Failed to open %s: %s\n", argv[--arg],
    strerror(errno));
  exit(EXIT_FAILURE);
  }

/* If files given, use them; else use stdin & stdout */

if (argc - arg > 0)
  {
  infile = fopen(argv[arg++], "r");
  if (infile == NULL)
    {
    (void)fprintf(stderr, "** Failed to open %s: %s\n", argv[--arg],
      strerror(errno));
    exit(EXIT_FAILURE);
    }
  }
else infile = stdin;

if (argc - arg > 0)
  {
  outfile = fopen(argv[arg++], "w");
  if (outfile == NULL)
    {
    (void)fprintf(stderr, "** Failed to open %s: %s\n", argv[--arg],
      strerror(errno));
    exit(EXIT_FAILURE);
    }
  }
else outfile = stdout;

/* Initialize hyphenation world */

Hyphen_Init();

/* Read each line and hyphenate it */

for (;;)
  {
  uschar line[256];
  uschar word[256];
  uschar singular[256];
  int offset = 0;

  if (Ufgets(line, 256, infile) == NULL) break;

  /* Split into words and hyphenate them */

  for (;;)
    {
    int i;
    int p = 0;
    int psave[10];
    int pptr = 0;
    int len, plural;

    if (sscanf(CS line+offset, "%s%n", word, &len) <= 0) break;
    offset += len;

    /* Remove non-letters at the start and end of the string, and
    perform other preparations (removal of 's); if this results in
    a null string, skip to next word. */

    if (Hyphen_Prepare(word) >= len) continue;

    /* Apply de-pluralling code and remember if it did anything */

    plural = Hyphen_DePlural(word, singular);

    /* Find all the hyphenation points and save them (they come out
    in reverse numerical order. */

    while (p >= 0)
      {
      if ((p = Hyphen_Next(singular, p)) > 0) psave[pptr++] = p;
      }

    /* If no hyphenation points were found and the word was de-
    pluralled, try again with the original form. */

    if (!pptr && plural)
      {
      p = 0;
      while (p >= 0)
        {
        if ((p = Hyphen_Next(word, p)) > 0) psave[pptr++] = p;
        }
      }

    /* Print hyphenated word: omit if no hyphens and not interactive,
    unless -all was requested. */

    pptr--;
    for (i = 0; i < Ustrlen(word); i++)
      {
      if (pptr >= 0 && i == psave[pptr])
        {
        fputc('-', outfile);
        pptr--;
        }
      fputc(word[i], outfile);
      }
    fputc('\n', outfile);
    }
  }

if (infile == stdin) printf("\n");
(void)fclose(outfile);
(void)fclose(infile);

return 0;
}

/* End of hytest.c */
