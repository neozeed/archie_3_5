#include "macros.h"


StrVal sort_types[] =
{
  { "filename",  SORT_FILENAME },
  { "hostname",  SORT_HOSTNAME },
  { "nothing",   SORT_DBORDER },
  { "rfilename", SORT_R_FILENAME },
  { "rhostname", SORT_R_HOSTNAME },
  { "rnothing",  SORT_DBORDER },
  { "rsize",     SORT_R_FILESIZE },
  { "rtime",     SORT_R_MODTIME },
  { "size",      SORT_FILESIZE },
  { "time",      SORT_MODTIME },

  { FRENCH("nom_du_fichier"),     SORT_FILENAME },
  { FRENCH("nom_d'hote"),         SORT_HOSTNAME },
  { FRENCH("aucun"),              SORT_DBORDER },
  { FRENCH("inv_nom_du_fichier"), SORT_R_FILENAME },
  { FRENCH("inv_nom_d'hote"),     SORT_R_HOSTNAME },
  { FRENCH("inv_aucun"),          SORT_DBORDER },
  { FRENCH("inv_taille"),         SORT_R_FILESIZE },
  { FRENCH("inv_temps"),          SORT_R_MODTIME },
  { FRENCH("taille"),             SORT_FILESIZE },
  { FRENCH("temps"),              SORT_MODTIME },

  { (const char *)0, 0 }
};


StrVal search_types[] =
{
  { "exact", '=' },
  { "regex", 'R' },
  { "sub", 'S' },
  { "subcase", 'C' },
  { "exact_regex", 'r' },
  { "exact_sub", 's' },
  { "exact_subcase", 'c' },
  { (const char *)0, 0 }
};
