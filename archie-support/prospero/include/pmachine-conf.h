/*
 * Machine types - Supported values
 *
 *   VAX, SUN, HP9000_S300, HP9000_S700, IBM_RTPC, ENCORE_NS32K,
 *   ENCORE_S93, ENCORE_S91, APOLLO, IBM_RS6000, IBM_PC
 * 
 *   MIPS_BE - MIPS Chip (Big Endian Byte Order)
 *   MIPS_LE - MIPS Chip (Little Endian Byte Order)
 *
 * Add others as needed.  
 *
 * Files that check these defintions:
 *   include/pmachine.h
 */
#ifdef AIX
#define P_MACHINE_TYPE 		"IBM_RS6000"
#define IBM_RS6000
#endif

#ifdef SUNOS
#define P_MACHINE_TYPE 		"SUN"
#define SUN
#endif

#ifdef SOLARIS
#define P_MACHINE_TYPE 		"SUN"
#define SUN
#endif

/*
 * Operating system - Supported values
 * 
 * ULTRIX, BSD43, SUNOS (version 4), SUNOS_V3 (SunOS version 3), HPUX, SYSV,
 * MACH, DOMAINOS, AIX, SOLARIS (a.k.a. SunOS version 5), SCOUNIX
 *
 * Add others as needed.  
 *
 * Files that check these defintions:
 *   include/pmachine.h, lib/pcompat/opendir.c, lib/pcompat/readdir.c
 */
#ifdef AIX
#define P_OS_TYPE		"AIX"
#endif

#ifdef SUNOS
#define P_OS_TYPE		"SUNOS"
#endif

#ifdef SOLARIS
#define P_OS_TYPE		"SOLARIS"
#endif

