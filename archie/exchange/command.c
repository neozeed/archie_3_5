#include "protonet.h"

command_t command_set[] = {
	   {"ERROR", C_ERROR},
	   {"QUIT",  C_QUIT},
	   {"LISTSITES" , C_LISTSITES},
	   {"SENDHEADER",   C_SENDHEADER},
	   {"UPDATELIST", C_UPDATELIST},
	   {"TUPLELIST", C_TUPLELIST},
	   {"SITEFILE", C_SITEFILE},
	   {"SENDSITE", C_SENDSITE},
	   {"SITELIST", C_SITELIST},
	   {"HEADER", C_HEADER},
	   {"DUMPCONFIG", C_DUMPCONFIG},
	   {"ENDDUMP", C_ENDDUMP},
	   {"VERSION", C_VERSION},
	   {"AUTH_ERR", C_AUTH_ERR},
     {"SENDEXCERPT", C_SENDEXCERPT},
	   {"", 0}
};	   
