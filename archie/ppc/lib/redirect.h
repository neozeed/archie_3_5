#ifndef REDIRECT_H
#define REDIRECT_H

#include <stdio.h>
#include "ansi_compat.h"


extern const char *redirection proto_((const char *src));
extern int load_redirection_file proto_((const char *redfile));
extern int load_redirection_fp proto_((FILE *rfp));

#endif
