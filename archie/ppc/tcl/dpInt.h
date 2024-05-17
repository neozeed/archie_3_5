/*
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#include "dp.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#if HAVE_SYS_TIME_H
#   include <sys/time.h>
#else
#   include <time.h>
#endif

#if HAVE_STDLIB_H
#    include <stdlib.h>
#else
#    include <compat/stdlib.h>
#endif

#if HAVE_MALLOC_H
#   include <malloc.h>
#else
#    include <compat/malloc.h>
#endif

#if !HAVE_HTONL
    extern unsigned short htons	_ANSI_ARGS_((unsigned short));
    extern unsigned short ntohs	_ANSI_ARGS_((unsigned short));
    extern unsigned int htonl	_ANSI_ARGS_((unsigned int));
    extern unsigned int ntohl	_ANSI_ARGS_((unsigned int));
#endif

#if HAVE_SYS_SELECT_H
#   include <sys/select.h>
#endif

#ifndef NO_FD_SET
#   define SELECT_MASK fd_set
#else
#   ifndef _AIX
	typedef long fd_mask;
#   endif
#   if defined(_IBMR2)
#       define SELECT_MASK void
#   else
#       define SELECT_MASK int
#   endif
#endif

#ifdef NO_STRING_H
#   include <compat/string.h>
#else
#   include <string.h>
#endif

#ifndef NO_WRITEV
#   include <sys/uio.h>
#endif

#if !HAVE_TOLOWER
int	tolower		_ANSI_ARGS_((int c));
#endif

#define DP_VERSION "3.2"

/*
 * Maximum number of allowable open files.  This sets limits on various
 * internal table sizes.
 */
 
#define MAX_OPEN_FILES  1024

/*
 * This define enables a bunch of hacks enabling this code to run
 * correctly on a Cray.  Specifically, there are several places where are
 * char* has to be converted to a long, preserving the byte order.  The
 * problem is that these char* addresses are only 4 bytes long, but
 * shorts, ints, longs, etc are all 64 bits.  This causes the address to
 * be stored in the upper 4 bytes when it should be in the lower 4, or
 * vice versa.  It's enabled when CRAY_HACKS is defined rather than just
 * CRAY so it can be used on other systems if need be.  The NEC SX-3 will
 * be no problem since it has int=long=32 bits, pointer=64.  But the DEC
 * Alpha uses int=32, long=pointer=64, and either: (a) some longs will
 * likely need to be changed to ints for the existing code to work, or (b)
 * keep the longs and define CRAY_HACKS.  Don't have any idea about other
 * emerging 64 bit chips (such as the MIPS R4000/R4400).
 *
 * Note also that Cray uses a bit field for the s_addr field of the
 * sockaddr_in struct, which cannot be used in a memcpy since the address
 * of it is undefined.  
 */

#ifdef CRAY
#define CRAY_HACKS
#endif
