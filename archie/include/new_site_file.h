/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#ifndef _NEW_SITE_FILE_H
#define _NEW_SITE_FILE_H

#include <limits.h>
#include "typedef.h"
#include "ansi_compat.h"

#define SITE_FLAG_FILE 0
#define SITE_FLAG_DIR 1
#define SITE_FLAG_DIR_FILE 2

/*
    Each site file consists of a series of fixed length records, each having
    the form:

	<site_entry> := <core_information><indices>

    where:

	<core_information> := <file_size><date_time><parent_pointer>\
	                      <child_pointer><permissions><flags>

	<file_size> := unsigned.32



            # <date_time> is specified in seconds, UTC/ANSI C.
            # The conversion should be relative to the time zone of the
	    # site in question.

	<date_time> := unsigned.32

            # <parent_pointer> is the absolute record number of the entry
	    # corresponding to the parent directory of the current entry.
	    # It is 0 for an entry corresponding to a root directory.
	    # For directory entries <child_pointer> is the record number of
	    # the first entry contained in the directory.  It is unused for
	    # non-directory entries.


	<parent_pointer> := unsigned.32
	<child_pointer> := unsigned.32


            # <permissions> is not interpreted by the server.
	    # <is_a_dir> is 0 for non-directories and 1 for directories.

	<permissions> := unsigned.16

	<flags> := <pad-1><is_a_dir>

	<pad-1> := unsigned.15          # RFU
	<is_a_dir> := boolean.1



	<indices> := <string_pointer><forward_pointer><backward_pointer>

	<string_pointer> := unsigned.32

	<forward_pointer> := <ip_address><site_file_index>
	<ip_address> := unsigned.32
	<site_file_index> := unsigned.32

	<backward_pointer> := <ip_address><site_file_index>
	<ip_address> := unsigned.32
	<site_file_index> := unsigned.32


    In general, <core_information> requires 20 bytes of storage, while
    <site_entry> requires 20 bytes.

    All integer quantities (e.g. unsigned) are stored with the most
    significant bit first (i.e. big-endian/network byte order).
*/


typedef struct /* Site information, as it exists in the database */
{
    flags_t flags;    /* =0 file     =2 file-dir      =1 dir */
    index_t strt_1 ;  /* dir  strt1= parent-dir	      strt2= location-in-prnt-list */
			
    union {    
          struct {
		file_size_t size ;
		date_time_t date ;
		perms_t perms ;
	  } entry; /*file*/

          struct {
		double weight;
	  } kwrd;

          struct {
		index_t strt_2;
	  } prnt_entry;  /*dir*/
    } core;
} new_site_entry_t ;

#endif
