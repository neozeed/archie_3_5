/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */



#ifndef _AR_ATTRIB_H_
#define _AR_ATTRIB_H_

typedef enum{

   AR_H_IP_ADDR = 01,

#define	SET_AR_H_IP_ADDR(x)	((x) |= AR_H_IP_ADDR)
#define	UNSET_AR_H_IP_ADDR(x)	((x) &= ~AR_H_IP_ADDR)
#define	GET_AR_H_IP_ADDR(x)	((x) & AR_H_IP_ADDR)

#define	NAME_AR_H_IP_ADDR  "AR_H_IP_ADDR"

   /* Usually something like "BSD Unix" or "VMS". I'm not going to try to
      enumerate the various values. */

   AR_H_OS_TYPE = 02,

#define	 SET_AR_H_OS_TYPE(x)		((x) |= AR_H_OS_TYPE)
#define	 UNSET_AR_H_OS_TYPE(x)		((x) &= ~AR_H_OS_TYPE)
#define	 GET_AR_H_OS_TYPE(x)		((x) & AR_H_OS_TYPE)

#define	 NAME_AR_H_OS_TYPE	  "AR_H_OS_TYPE"

   /*
    * Given in seconds offset from UTC. Really needs to be signed (negative
    * for west, positive for east). Is explicit with either  "+" or "-"
    * Given as YYYYMMDDHHMMSS"Z"
    */

   AR_H_TIMEZ = 04,

#define	 SET_AR_H_TIMEZ(x)		 ((x) |= AR_H_TIMEZ)
#define	 UNSET_AR_H_TIMEZ(x)		 ((x) &= ~AR_H_TIMEZ)
#define	 GET_AR_H_TIMEZ(x)		 ((x) & AR_H_TIMEZ)

#define	 NAME_AR_H_TIMEZ	"AR_H_TIMEZ"

   /* This is stored but currently I don't pay much attention to it. The
      idea is to keep track of the archie host which is responsible for
      retrieving and assimilating the raw data from this site into the
      archie system. It is meant to be used internally to provide for certain
      info distribution optimizations, but may be of use to the user. */

   AUTHORITY = 010,

#define	 SET_AUTHORITY(x)	       ((x) |= AUTHORITY)
#define	 UNSET_AUTHORITY(x)	       ((x) &= ~AUTHORITY)
#define	 GET_AUTHORITY(x)	       ((x) & AUTHORITY)

#define	 NAME_AUTHORITY				"AR_AUTHORITY"

   /* The system keeps track of the number of "records" for this site/access
      method pair (a site may have several access methods). This is defined
      on a method-by-method basis. For some methods, it might not make much
      sense */

   
   AR_RECNO = 020,

#define	 SET_AR_RECNO(x)		     ((x) |= AR_RECNO)
#define	 UNSET_AR_RECNO(x)	  ((x) &= ~AR_RECNO)
#define	 GET_AR_RECNO(x)		     ((x) & AR_RECNO)

#define	 NAME_AR_RECNO		  "AR_RECNO"


/* last modification time of this link */  

   LK_LAST_MOD = 040,

#define	 SET_LK_LAST_MOD(x)	     ((x) |= LK_LAST_MOD)
#define	 UNSET_LK_LAST_MOD(x)	     ((x) &= ~LK_LAST_MOD)
#define	 GET_LK_LAST_MOD(x)	     ((x) & LK_LAST_MOD)

#define	 NAME_LK_LAST_MOD			"LAST-MODIFIED"  /* backwards compatible */


   /* As defined for the anonymous ftp database, this is the number of
      children of a directory (files & subdirectories). I'm not going to
      implement this right at the moment */

   LINK_COUNT = 0100,

#define	 SET_LINK_COUNT(x)		  ((x) |= LINK_COUNT)
#define	 UNSET_LINK_COUNT(x)	       ((x) &= ~LINK_COUNT)
#define	 GET_LINK_COUNT(x)		  ((x) & LINK_COUNT)

#define	 NAME_LINK_COUNT	    "LINK-COUNT"

   /* Size of given link */

   LINK_SIZE = 0200,

#define	 SET_LINK_SIZE(x)	       ((x) |= LINK_SIZE)
#define	 UNSET_LINK_SIZE(x)	       ((x) &= ~LINK_SIZE)
#define	 GET_LINK_SIZE(x)	       ((x) & LINK_SIZE)

#define	 NAME_LINK_SIZE		    "SIZE"	/* Backwards compatible */

   /* This is the permissions associated with the link. It is given as
      a "bit sequence" (translated into ascii digits) and is OS specific,
      that is, you have to know the OS that you're working with in order
      to interpret the bits in the correct order */

   NATIVE_MODES = 0400,

#define	 SET_NATIVE_MODES(x)	       ((x) |= NATIVE_MODES)
#define	 UNSET_NATIVE_MODES(x)	       ((x) &= ~NATIVE_MODES)
#define	 GET_NATIVE_MODES(x)	       ((x) & NATIVE_MODES)

#define	 NAME_NATIVE_MODES	       "NATIVE-MODES"


/* Last update time of the host */

   AR_H_LAST_MOD = 01000,

#define  SET_AR_H_LAST_MOD(x)		 ((x) |= AR_H_LAST_MOD)
#define  UNSET_AR_H_LAST_MOD(x)	      ((x) &= ~AR_H_LAST_MOD)
#define  GET_AR_H_LAST_MOD(x)		 ((x) & AR_H_LAST_MOD)
   
#define	 NAME_AR_H_LAST_MOD	  "AR_H_LAST_MOD"

/* Current status of database in archie system */


   AR_DB_STAT = 02000,

#define  SET_AR_DB_STAT(x) 	 ((x) |= AR_DB_STAT)
#define  UNSET_AR_DB_STAT(x)	 ((x) &= ~AR_DB_STAT)
#define  GET_AR_DB_STAT(x)		 ((x) & AR_DB_STAT)


#define	 NAME_AR_DB_STAT  "AR_DB_STAT"



   LK_UNIX_MODES = 04000,

#define	 SET_LK_UNIX_MODES(x)	       ((x) |= LK_UNIX_MODES)
#define	 UNSET_LK_UNIX_MODES(x)	       ((x) &= ~LK_UNIX_MODES)
#define	 GET_LK_UNIX_MODES(x)	       ((x) & LK_UNIX_MODES)

#define	 NAME_LK_UNIX_MODES	       "UNIX-MODES"



} attr_defs;


typedef attr_defs attrib_list_t;

#define GET_ANY_ATTRIB(x)	       ((x) & 0x00ffffff)
#define	SET_ALL_ATTRIB(x)	       ((x) |= 0x00ffffff)

typedef enum{

   ARCHIE_DIR = 001000000

#define	 SET_ARCHIE_DIR(x)	       ((x) |= ARCHIE_DIR)
#define	 UNSET_ARCHIE_DIR(x)	       ((x) &= ~ARCHIE_DIR)
#define	 GET_ARCHIE_DIR(x)	       ((x) & ARCHIE_DIR)

} prosp_flags_t;



#endif

