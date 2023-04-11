/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* This file created in 2023; last modified: April 2023 */

/* This is a freestanding program that tests the SVG-handling functions. Given
the name of an SVG file, it generates a PostScript representation. */

#include "sdop.h"

BOOL svg_listignored = TRUE;


/*************************************************
*              Main program                      *
*************************************************/

int main(int argc, char **argv)
{
FILE *f = fopen(argv[1], "rb");

if (f == NULL)
  {
  fprintf(stderr, "** Failed to open %s: %s\n", argv[1], strerror(errno));
  return 1;
  }

/* Write PostScript boilerplate that is included in SDoP's PSHeader file. */

printf("/Mt/moveto load def\n"
  "/RMt/rmoveto load def\n"
  "/RLt/rlineto load def\n"
  "/Ct/curveto load def\n"
  "/S/show load def\n"
  "/Slw/setlinewidth load def\n"
  "/St/stroke load def\n\n");

printf("/SM { dup stringwidth pop 2 div neg 0 RMt show } bind def\n");
printf("/SE { dup stringwidth pop neg 0 RMt show } bind def\n\n");

/* This PostScript function tests a number fonts in order until it finds one
that exists. If none are found, Times-Roman is defaulted. */

printf("/multifindfont{\n"
  "{dup /Font resourcestatus {pop pop findfont exit}{pop}ifelse}forall\n"
  "}bind def\n\n");
  
/* Now process the SVG file */ 

svg_write(f, stdout);
return 0;
}

/* End of svgtest.c */
