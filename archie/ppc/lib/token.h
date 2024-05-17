#ifndef TOKEN_H
#define TOKEN_H

#include "ansi_compat.h"
#include "prosp.h"


extern char **tkarray proto_((TOKEN tok));
extern int tkmatches proto_((TOKEN tok, ...));

#endif
