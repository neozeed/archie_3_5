/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>.
 */

#include <usc-copyr.h>

extern int gopherget(char *host, char *local, char *selector_string, int port,
                     int gophertype /* actually char, but make sure default
                                       promotions work. */ );
