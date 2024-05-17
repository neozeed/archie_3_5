#ifndef PENCODE_H
#define PENCODE_H

#include "ansi_compat.h"


extern int pDecodeCh proto_((int c, char **hosttype, char **hsonametype));
extern int pEncodeCh proto_((const char *hosttype, const char *hsonametype));

#endif
