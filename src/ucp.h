/*************************************************
*          Unicode Property Table Data           *
*************************************************/

#ifndef _UCP_H
#define _UCP_H

/* This file contains definitions of the property values that are stored in
records in the table in ucd.c. The only use made of this is testing for letters
when hyphenating. */

/* These are the general character categories. */

enum {
  ucp_C,     /* Other */
  ucp_L,     /* Letter */
  ucp_M,     /* Mark */
  ucp_N,     /* Number */
  ucp_P,     /* Punctuation */
  ucp_S,     /* Symbol */
  ucp_Z      /* Separator */
};

/* These are the particular character types. */

enum {
  ucp_Cc,    /* Control */
  ucp_Cf,    /* Format */
  ucp_Cn,    /* Unassigned */
  ucp_Co,    /* Private use */
  ucp_Cs,    /* Surrogate */
  ucp_Ll,    /* Lower case letter */
  ucp_Lm,    /* Modifier letter */
  ucp_Lo,    /* Other letter */
  ucp_Lt,    /* Title case letter */
  ucp_Lu,    /* Upper case letter */
  ucp_Mc,    /* Spacing mark */
  ucp_Me,    /* Enclosing mark */
  ucp_Mn,    /* Non-spacing mark */
  ucp_Nd,    /* Decimal number */
  ucp_Nl,    /* Letter number */
  ucp_No,    /* Other number */
  ucp_Pc,    /* Connector punctuation */
  ucp_Pd,    /* Dash punctuation */
  ucp_Pe,    /* Close punctuation */
  ucp_Pf,    /* Final punctuation */
  ucp_Pi,    /* Initial punctuation */
  ucp_Po,    /* Other punctuation */
  ucp_Ps,    /* Open punctuation */
  ucp_Sc,    /* Currency symbol */
  ucp_Sk,    /* Modifier symbol */
  ucp_Sm,    /* Mathematical symbol */
  ucp_So,    /* Other symbol */
  ucp_Zl,    /* Line separator */
  ucp_Zp,    /* Paragraph separator */
  ucp_Zs     /* Space separator */
};

#endif

/* End of ucp.h */
