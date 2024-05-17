#ifndef _HEADER_DEF_H_
#define _HEADER_DEF_H_

#include "sub_header.h"

/*
 * This structure holds the information on what header fields look like.
 * It also holds the information to check if that field has been set or not
 */

static ascii_header_fields_t sub_header_fields[] = {
	{ SUB_HEADER_BEGIN_I, SUB_HEADER_BEGIN, 0},
	{ LOCAL_URL_ENTRY_I, LOCAL_URL_ENTRY, 0},
	{ STATE_ENTRY_I, STATE_ENTRY, 0},
	{ SIZE_ENTRY_I, SIZE_ENTRY, 0},
	{ PORT_ENTRY_I, PORT_ENTRY, 0},
  { SERVER_ENTRY_I, SERVER_ENTRY, 0},
  { TYPE_ENTRY_I, TYPE_ENTRY, 0},
  { RECNO_ENTRY_I, RECNO_ENTRY, 0},
  { DATE_ENTRY_I, DATE_ENTRY, 0},
	{ FORMAT_ENTRY_I, FORMAT_ENTRY, 0},
	{ FSIZE_ENTRY_I, FSIZE_ENTRY, 0},  
	{ SUB_HEADER_END_I, SUB_HEADER_END, 0}
};

#endif
