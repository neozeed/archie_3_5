/* Catalogs manipulation */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef ARCHIE_TIMING
#include <time.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include "pfs.h"
#include "typedef.h"
#include "db_files.h"
#include "db_ops.h"
#include "gindexdb_ops.h"
#include "error.h"
#include "master.h"
#include "files.h"
#include "archie_strings.h"
#include "archie_catalogs.h"
#include "prwais_do_search.h"

#include "protos.h"

static int      catalogs_time = 0;

/*
 * read_catalogs_list: read the catalogs file (by default
 * ~archie/etc/catalogs.cf) into the internal system list.
 *
 */

status_t read_catalogs_list(catalogs_file, catalogs_list, max_cat, p_err_string)
  file_info_t    *catalogs_file;
  catalogs_list_t *catalogs_list;
  int             max_cat;
  char           *p_err_string;
{
  char input_buf[2048];
  char *iptr, *tmp_ptr;
  int lineno;
  char **aptr;
  pathname_t attributes;
  pathname_t typestring, dirstring, accesstr;
  catalogs_list_t *curr_cat;
  int count;
  int past_master_host = 0;
  pathname_t mdir, hdir;
  struct stat statbuf;
  int reread = 0;

  ptr_check(catalogs_file, file_info_t, "read_catalogs_list", ERROR);
  ptr_check(catalogs_list, catalogs_list_t, "read_catalogs_list", ERROR);

  mdir[0] = hdir[0] = '\0';

  /* catalogs_list must be zeroed out before being passed */

  if (catalogs_file->filename[0] == '\0') {
    /* generate default catalogs filename */

    sprintf(catalogs_file->filename, "%s/%s/%s",
            get_archie_home(), DEFAULT_ETC_DIR, DEFAULT_CATALOGS_FILENAME);
    catalogs_time = 0;
  }

  /* check to see if catalogs.cf file exists */

  if (access(catalogs_file->filename, R_OK | F_OK) == -1) {
    catalogs_list->cat_name[0] = '\0';
    return (ERROR);
  }


  /* Check to see if we need to re-read the catalogs file */

  if (stat(catalogs_file->filename, &statbuf) == -1) {
    error(A_ERR, "read_catalogs_list", "Can't stat() catalogs file %s", catalogs_file->filename);
    return (ERROR);
  }

  if (catalogs_time != 0) {
    if (statbuf.st_mtime > catalogs_time) {
      catalogs_time = statbuf.st_mtime;
      /*	 plog(L_DB_INFO, NOREQ, "Catalogs file %s modified. Reinitializing databases", catalogs_file->filename); */
      reread = 1;
    }
    else
      return (A_OK);
  }
  else
    catalogs_time = statbuf.st_mtime;

  /* It does, then open it */

  if (open_file(catalogs_file, O_RDONLY) == ERROR) {
    error(A_ERR, "read_catalogs_list", "Can't open catalogs file %s", catalogs_file->filename);
    return (ERROR);
  }

  count = 0;

  for (iptr = input_buf, lineno = 1; fgets(iptr, input_buf + sizeof(input_buf) - iptr, catalogs_file->fp_or_dbm.fp) != (char *) NULL; lineno++) {
    if ((tmp_ptr = strstr(input_buf, CONTINUATION_LINE)) != NULL) {
      *tmp_ptr = '\0';
      iptr = tmp_ptr;
      continue;
    }

    /* Recognise comments */

    if ((tmp_ptr = strchr(input_buf, COMMENT_CHAR)) != NULL) *tmp_ptr = '\0';

    if (iptr[0] == '\0') continue;

    curr_cat = &catalogs_list[count];

    curr_cat->initialized = 0;

    attributes[0] = '\0';

    if (sscanf(input_buf, "%s %s %s %s", curr_cat->cat_name, typestring, accesstr, dirstring) < 2) {
      error(A_ERR, "read_catalogs_list",
            "Error in catalogs file %s, line %d. Insufficient arguments",
            catalogs_file->filename, lineno);
      error(A_ERR, "read_catalogs_list", "Line: '%s'", input_buf);
      continue;
    }

    if (strcasecmp(curr_cat->cat_name, MASTER_DIR_LINE) == 0) {
      if (past_master_host) {
        error(A_ERR, "read_catalogs_list",
              "Master directory specification must occur before standard databases");
        return (ERROR);
      }

      /* Master database directory */

      if (typestring[0] != '/') {
        error(A_ERR, "read_catalogs_list",
              "Full pathname not specified for master directory '%s'", typestring);
        return (ERROR);
      }

      strcpy(curr_cat->cat_db, typestring);
      strcpy(mdir, typestring);

      continue;
    }

    if (strcasecmp(curr_cat->cat_name, HOST_DIR_LINE) == 0) {
      /* Host database directory */

      if (past_master_host) {
        error(A_ERR, "read_catalogs_list",
              "Host database directory specification must occur before standard databases");
        return (ERROR);
      }

      if (typestring[0] != '/') {
        error(A_ERR, "read_catalogs_list",
              "Full pathname not specified for host database directory '%s'", typestring);
        return (ERROR);
      }

      strcpy(curr_cat->cat_db, typestring);
      strcpy(hdir, typestring);

      continue;
    }

    if (!past_master_host++) {
      if (set_master_db_dir(mdir) == (char *) NULL) {
        error(A_ERR, "Can't set archie master db directory");
        return (ERROR);
      }

      if (set_host_db_dir(hdir) == (char *) NULL) {
        error(A_ERR, "Can't set archie host db directory");
        return (ERROR);
      }
    }

    /* Check if it is an anonftp database */

    if (strncasecmp(typestring, CATS_ARCHIE, strlen(CATS_ARCHIE)) == 0) {
      curr_cat->cat_type = CAT_ARCHIE;

      aptr = str_sep(typestring, ',');

      if (aptr[1] != (char *) NULL) {
        char **p;

        curr_cat->cat_ainfo.archie.search_vector = 0;

        for (p = aptr + 1; *p != (char *) NULL; p++) {
          if (strcasecmp(*p, ARS_NO_REGEX) == 0) {
            /* disable regex */
            curr_cat->cat_ainfo.archie.search_vector |= AR_NO_REGEX;
          }
          else if (strcasecmp(*p, ARS_NO_SUB) == 0) {
            /* Disable case instensitive substring */
            curr_cat->cat_ainfo.archie.search_vector |= AR_NO_SUB;
          }
          else if (strcasecmp(*p, ARS_NO_SUBCASE) == 0) {
            /* Disable case sensitive substring */
            curr_cat->cat_ainfo.archie.search_vector |= AR_NO_SUBCASE;
          }
          else if (strcasecmp(*p, ARS_NO_EXACT) == 0) {
            /* Disable exact */
            curr_cat->cat_ainfo.archie.search_vector |= AR_NO_EXACT;
          }
        }
      }

      if (aptr) free_opts(aptr);

      if (reread) {
/*        if (close_files_db(curr_cat->cat_ainfo.archie.strings_idx,
                           curr_cat->cat_ainfo.archie.strings,
                           curr_cat->cat_ainfo.archie.strings_hash) == ERROR) {
          error(A_ERR, "read_catalogs_list", "Can't open anonftp database");
          return (ERROR);
        }
        */
      }
      else {
/*        curr_cat->cat_ainfo.archie.strings_idx = create_finfo();
        curr_cat->cat_ainfo.archie.strings = create_finfo();
        curr_cat->cat_ainfo.archie.strings_hash = create_finfo();
*/
      }

      if (strncasecmp(accesstr, CATAS_ANONFTP, strlen(CATAS_ANONFTP)) == 0) {
        curr_cat->cat_access = CATA_ANONFTP;

        if (dirstring[0] != '\0') {
          if (dirstring[0] == '/') strcpy(curr_cat->cat_db, dirstring);
          else strcpy(curr_cat->cat_db, master_db_filename(dirstring));
        }
        else
        sprintf(curr_cat->cat_db, "%s/%s", get_master_db_dir(), dirstring);
      }

#ifdef GOPHERINDEX_SUPPORT
      else if (strncasecmp(accesstr, CATAS_GOPHERINDEX, strlen(CATAS_GOPHERINDEX)) == 0) {
        curr_cat->cat_access = CATA_GOPHERINDEX;

        if (reread) {
          if (close_gfiles_db((file_info_t *) NULL,
                              (file_info_t *) NULL,
                              (file_info_t *) NULL,
                              (file_info_t *) NULL,
                              curr_cat->cat_ainfo.archie.host_finfo,
                              curr_cat->cat_ainfo.archie.sel_finfo,
                              (file_info_t *) NULL) == ERROR) {
            error(A_ERR, "read_catalogs_list", "Can't close archie gopherindex database");
            return (ERROR);
          }
        }
        else {
          curr_cat->cat_ainfo.archie.sel_finfo = create_finfo();
          curr_cat->cat_ainfo.archie.host_finfo = create_finfo();
        }

        if (dirstring[0] != '\0') {
          if (dirstring[0] == '/') strcpy(curr_cat->cat_db, dirstring);
          else strcpy(curr_cat->cat_db, master_db_filename(dirstring));
        }
        else
        sprintf(curr_cat->cat_db, "%s/%s", get_master_db_dir(), dirstring);
      }
#endif
      else if (strncasecmp(accesstr, CATAS_WEBINDEX, strlen(CATAS_WEBINDEX)) == 0) {
        curr_cat->cat_access = CATA_WEBINDEX;

        if (reread) {
          if (close_wfiles_db((file_info_t *) NULL,
                              (file_info_t *) NULL,
                              (file_info_t *) NULL,
                              (file_info_t *) NULL,
                              curr_cat->cat_ainfo.archie.host_finfo,
                              curr_cat->cat_ainfo.archie.sel_finfo,
                              (file_info_t *) NULL) == ERROR) {
            error(A_ERR, "read_catalogs_list", "Can't close archie webindex database");
            return (ERROR);
          }
        }
        else {
          curr_cat->cat_ainfo.archie.sel_finfo = create_finfo();
          curr_cat->cat_ainfo.archie.host_finfo = create_finfo();
        }

        if (dirstring[0] != '\0') {
          if (dirstring[0] == '/') strcpy(curr_cat->cat_db, dirstring);
          else strcpy(curr_cat->cat_db, master_db_filename(dirstring));
        }
        else
          sprintf(curr_cat->cat_db, "%s/%s", get_master_db_dir(), dirstring);
      }

      else
      error(A_ERR, "read_catalogs_list",
            "Unknown access method '%s' for '%s' database", accesstr, CATS_ARCHIE);
    }
    else if (strncasecmp(typestring, CATS_TEMPLATE, strlen(CATS_TEMPLATE)) == 0) {
      curr_cat->cat_type = CAT_TEMPLATE;

      if (dirstring[0] != '\0') {
        if (dirstring[0] == '/') strcpy(curr_cat->cat_db, dirstring);
        else strcpy(curr_cat->cat_db, master_db_filename(dirstring));
      }
      else
        sprintf(curr_cat->cat_db, "%s/%s", get_master_db_dir(), dirstring);

      aptr = str_sep(typestring, ',');

      if (aptr[1]) {
        if (*aptr[1] == '/') sprintf(curr_cat->cat_ainfo.template.ctemplate_aux, "%s.aux",
                                     aptr[1]);
        else sprintf(curr_cat->cat_ainfo.template.ctemplate_aux, "%s/%s/%s.aux",
                     get_archie_home(), DEFAULT_ETC_DIR, aptr[1]);
      }
      else
        sprintf(curr_cat->cat_ainfo.template.ctemplate_aux, "%s/%s/%s.aux",
                get_archie_home(), DEFAULT_ETC_DIR, curr_cat->cat_name);

#ifdef BUNYIP_AUTHENTICATION
      curr_cat->restricted = 0;
      if (aptr[2]) { 
        if ( strncmp(aptr[2],"restricted",strlen("restricted") ) == 0 )  {
          curr_cat->restricted = 1;

          if (*(aptr[2]+strlen("restricted")) == '(' && curr_cat->classes == NULL ) {
            int index;
            int size;

            /* Figure out the lenght of the string of all classes */
            size = strlen(aptr[2]+strlen("restricted")+1);
            for ( index = 3; aptr[index] != NULL ; index++ ) {
              size += strlen(aptr[index]);
              if ( *(aptr[index]+strlen(aptr[index])-1) == ')' )
              break;		     
            }
            /* Note that last one has an extra ')' character .. */
            /* Now size hold the total number of characters + ')' */
            /* index - 2 == number of classes */
            if ( aptr[index] == NULL ) aptr[index-1][strlen(aptr[index-1])-1] = '\0';
            else aptr[index][strlen(aptr[index])-1] = '\0';
            curr_cat->classes = (char*)malloc((size+index-3)*sizeof(char));

            if ( curr_cat->classes == NULL ) {
              /*		     plog(L_DB_INFO, NOREQ, "Can't allocate memory!"); */
              return PFAILURE;
            }

            strcpy(curr_cat->classes,aptr[2]+strlen("restricted")+1);
            for ( index = 3; aptr[index] != NULL; index++ ) {
              strcat(curr_cat->classes,",");
              strcat(curr_cat->classes,aptr[index]);
              if ( *(aptr[index]+strlen(aptr[index])-1) == ')' ) break;
            }
          }
          else 
            curr_cat->classes = NULL;
        }
      }
#endif

      if (aptr) free_opts(aptr);

      aptr = str_sep(accesstr, ',');

      if (strcasecmp(aptr[0], CATAS_WAIS) == 0) {
        int i;

        curr_cat->cat_access = CATA_WAIS;
        for (i = 1; aptr[i]; i++) {
          if (strcasecmp(aptr[i], "expand") == 0) curr_cat->cat_ainfo.template.expand = 1;
          else error(A_ERR, "read_catalogs_list",
                     "Unknown option '%s' for database '%s'", aptr[i], curr_cat->cat_name);
        }
      }

      if (aptr) free_opts(aptr);
    }
    else if (strncasecmp(typestring, CATS_FREETEXT, strlen(CATS_TEMPLATE)) == 0) {
      curr_cat->cat_type = CAT_FREETEXT;

      if (dirstring[0] != '\0') {
        if (dirstring[0] == '/') strcpy(curr_cat->cat_db, dirstring);
        else strcpy(curr_cat->cat_db, master_db_filename(dirstring));
      }
      else
        sprintf(curr_cat->cat_db, "%s/%s", get_master_db_dir(), dirstring);

      aptr = str_sep(accesstr, ',');

      if (strcasecmp(aptr[0], CATAS_WAIS) == 0) {
        int i;

        curr_cat->cat_access = CATA_WAIS;
        for (i = 1; aptr[i]; i++) {
          if (strcasecmp(aptr[i], "expand") == 0) curr_cat->cat_ainfo.template.expand = 1;
          else error(A_ERR, "read_catalogs_list",
                     "Unknown option '%s' for database '%s'", aptr[i], curr_cat->cat_name);
        }
      }

      if (aptr)
      free_opts(aptr);
    }

    count++;
  }

  catalogs_list[count].cat_name[0] = '\0';

  close_file(catalogs_file);

  if (initialize_databases(catalogs_list, p_err_string) == ERROR)
    return (ERROR);

  return (A_OK);
}


catalogs_list_t *find_in_catalogs(dbname, catalog_list)
  char           *dbname;
  catalogs_list_t *catalog_list;
{
   extern status_t reinitialize_template PROTO((catalogs_list_t *));

   struct stat     statbuf;
   catalogs_list_t *cat;

   ptr_check(dbname, char, "find_in_catalogs", (catalogs_list_t *) NULL);
   ptr_check(catalog_list, catalogs_list_t, "find_in_catalogs", (catalogs_list_t *) NULL);

   for (cat = catalog_list; cat->cat_name[0] != '\0'; cat++) {

      if (strcasecmp(dbname, cat->cat_name) == 0) {

	 if (cat->cat_type == CAT_TEMPLATE) {

	    if (stat(cat->cat_ainfo.template.ctemplate_aux, &statbuf) == -1)
	       return (cat);

	    if (statbuf.st_mtime > cat->cat_ainfo.template.lmodtime) {
	       if (reinitialize_template(cat) == ERROR)
		  return ((catalogs_list_t *) NULL);
	       error(A_INFO, "find_in_catalogs", "Auxiliary file changed. Reinitializing %s template.", cat->cat_name);
	    }
	 }

	 return (cat);
      }

   }

   return ((catalogs_list_t *) NULL);

}


status_t        initialize_databases(catalogs_list, errst)
   catalogs_list_t catalogs_list[];
   char           *errst;
{
   int             count;
   catalogs_list_t *curr_cat;


   curr_cat = &catalogs_list[0];

   for (count = 0; curr_cat->cat_name[0] != '\0'; curr_cat++) {

      if (curr_cat->cat_type == CAT_ARCHIE) {

	 if (curr_cat->cat_access == CATA_ANONFTP) {
/*
	    file_info_t    *strings_idx;
	    file_info_t    *strings;
	    file_info_t    *strings_hash;

	    strings_idx = curr_cat->cat_ainfo.archie.strings_idx;
	    strings = curr_cat->cat_ainfo.archie.strings;
	    strings_hash = curr_cat->cat_ainfo.archie.strings_hash;
*/
	    if (set_files_db_dir(curr_cat->cat_db) == (char *) NULL) {
	       error(A_ERR, "read_catalogs_list", "Can't set archie files db directory");
	       return (PFAILURE);
	    }

	    error(A_INFO, "read_catalogs_list", "Prospero server opened anonftp database");

/*	    if (open_files_db(strings_idx, strings, strings_hash, O_RDONLY) != A_OK) {
*	       plog(L_DB_ERROR,NOREQ,"Can't open anonftp database"); *
	       return (ERROR);
	    }
      */
	 }

#ifdef GOPHERINDEX_SUPPORT
	 else if (curr_cat->cat_access == CATA_GOPHERINDEX) {
	    file_info_t    *gstrings_idx;
	    file_info_t    *gstrings;
	    file_info_t    *gstrings_hash;
	    file_info_t    *sel_finfo;
	    file_info_t    *host_finfo;

	    gstrings_idx = curr_cat->cat_ainfo.archie.strings_idx;
	    gstrings = curr_cat->cat_ainfo.archie.strings;
	    gstrings_hash = curr_cat->cat_ainfo.archie.strings_hash;
	    host_finfo = curr_cat->cat_ainfo.archie.host_finfo;
	    sel_finfo = curr_cat->cat_ainfo.archie.sel_finfo;

	    if (set_gfiles_db_dir(curr_cat->cat_db) == (char *) NULL) {
	       error(A_ERR, "read_catalogs_list", "Can't set gopherindex files db directory");
	       return (ERROR);
	    }

	    error(A_INFO, "read_catalogs_list", "Opened gopherindex database");

	    if (open_gfiles_db(gstrings_idx, gstrings, gstrings_hash, (file_info_t *) NULL, host_finfo, sel_finfo, (file_info_t *) NULL, O_RDONLY) == ERROR) {
	       error(A_ERR, "Can't open archie gopherindex database");
	       return (ERROR);
	    }
	 }
#endif
      }
      else if (curr_cat->cat_type == CAT_TEMPLATE) {

	 initialize_template(curr_cat);
      }
   }

   return (A_OK);
}


int aux_rec_sort(a, b)
  template_aux_rec *a, *b;
{
  if (a->isheader > b->isheader)
    return (1);
  else if (a->isheader < b->isheader)
    return (-1);
  else
    return (0);
}

status_t        initialize_template(curr_cat)
   catalogs_list_t *curr_cat;
{

   int             args;
   int             i;
   template_aux_rec *tarec;
   int             hnum = 0;
   file_info_t    *aux_file = create_finfo();
   char            inbuf[MAX_PATH_LEN];
   char            attrname[2 * MAX_ATTRIBUTE_LENGTH];
   char            userstr[2 * MAX_USERSTRING];
   struct stat     statbuf;

   strcpy(aux_file->filename, curr_cat->cat_ainfo.template.ctemplate_aux);

   if (access(aux_file->filename, R_OK | F_OK) == -1)
      return (ERROR);

   if (open_file(aux_file, O_RDONLY) == ERROR)
      return (ERROR);

   if (fstat(fileno(aux_file->fp_or_dbm.fp), &statbuf) == -1)
      return (ERROR);

   tarec = &curr_cat->cat_ainfo.template.recs[0];

   curr_cat->cat_ainfo.template.lmodtime = statbuf.st_mtime;


   for (i = 1; fgets(inbuf, sizeof(inbuf), aux_file->fp_or_dbm.fp) == inbuf;
      tarec++, i++) {

      attrname[0] = '\0';
      userstr[0] = '\0';
      memset(tarec, '\0', sizeof(*tarec));

      if ((args = sscanf(inbuf, "%s%d%d \"%[^\"]*\"", attrname, &tarec->tindex, &hnum, userstr)) == 4) {
	 tarec->isheader = hnum;

      }
      else if ((args = sscanf(inbuf, "%s%d%d%s", attrname, &tarec->tindex, &hnum, userstr)) == 4) {
	 tarec->isheader = hnum;

      }
      else if ((args = sscanf(inbuf, "%s%d%d", attrname, &tarec->tindex, &hnum)) == 3) {
	 tarec->isheader = hnum;

      }
      else if ((args = sscanf(inbuf, "%s%d \"%[^\"]*\"", attrname, &tarec->tindex, userstr)) == 3) {
	 tarec->isheader = INT_MAX;

      }
      else if ((args = sscanf(inbuf, "%s%d %[A-Za-z \t]", attrname, &tarec->tindex, userstr)) == 3) {
	 tarec->isheader = INT_MAX;

      }
      else if (args == 2) {
	 tarec->isheader = INT_MAX;
      }
      else {
	 error(A_ERR, "initialize_template", "Error line %d in auxiliary file %s. ", i, aux_file->filename);
	 return (ERROR);
      }

      if (strlen(attrname) > MAX_ATTRIBUTE_LENGTH) {
	 error(A_ERR, "initialize_template", "Error line %d in auxiliary file %s. Attribute name too long %s", i, aux_file->filename, attrname);
	 return (ERROR);
      }
      else
	 tarec->name = strdup(attrname);

      if (tarec -> tindex == 0){
	 error(A_ERR, "initialize_template", "Error line %d in auxiliary file %s. Index number for %s must not be 0", i, aux_file->filename, attrname);
      }

      if (strlen(userstr) > MAX_USERSTRING) {
	 error(A_ERR, "initialize_template", "Error line %d in auxiliary file %s. User string too long %s", i, aux_file->filename, userstr);
	 return (ERROR);
      }
      else
	 tarec->userstring = strdup(userstr);
   }

   tarec -> name = strdup(BUNYIP_SEQ_STR);
   tarec -> tindex = 0;
   tarec->isheader = INT_MAX;   
   tarec->userstring = strdup("");

   tarec++;

   tarec->name = strdup("");

   close_file(aux_file);

   qsort((char *) &(curr_cat->cat_ainfo.template.recs), i - 1, sizeof(template_aux_rec), aux_rec_sort);

   return (A_OK);
}



status_t        reinitialize_template(curr_cat)
   catalogs_list_t *curr_cat;

{
   template_aux_rec *tarec;

   for (tarec = &curr_cat->cat_ainfo.template.recs[0]; tarec != NULL && tarec->name[0] != '\0'; tarec++) {

      if (tarec->name)
	 free(tarec->name);

      if (tarec->userstring)
	 free(tarec->userstring);

   }

   return (initialize_template(curr_cat));
}
