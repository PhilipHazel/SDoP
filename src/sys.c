/*************************************************
*          sdop - Simple DocBook Processor       *
*************************************************/

/* Copyright (c) Philip Hazel, 2023 */
/* Created in 2006; last modified: March 2023 */

/* This module contains functions that are system-dependent. At the moment,
the only supported system is a Unix-like environment. */

#include "sdop.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


/*************************************************
*        Check for the existence of a file       *
*************************************************/

BOOL
sys_exists(uschar *name)
{
struct stat statbuf;
return stat(CCS name, &statbuf) == 0;
}

/* End of sys.c */
